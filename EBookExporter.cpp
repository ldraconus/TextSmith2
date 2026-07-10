#include "EBookExporter.h"

#include <QDate>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QPrinter>

#include <zip.h>

#include "Main.h"
#include "ui_Main.h"
#include "TextEdit.h"

EBookExporter::~EBookExporter() {
    if (mZip) zip_close(mZip);
    mZip = nullptr;
}

bool EBookExporter::convert() {
    if (mZip) return true;

    int error = 0;
    QByteArray path = mFilename.toUtf8();
    mZip = zip_open(path.constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!mZip) {
        qWarning() << "Failed to open EPUB for writing:" << mFilename << "error:" << error;
        return false;
    }

    novelToBook();

    auto defaults = collectMetadataDefaults();
    mTitle =     fetchValue(0, defaults, "title");
    mAuthor =    fetchValue(1, defaults, "author");
    mCover =     fetchValue(2, defaults, "cover");
    mRights =    fetchValue(3, defaults, "title");
    mLanguage =  fetchValue(4, defaults, "language");
    mId =        fetchValue(5, defaults, "id");
    mPublisher = fetchValue(6, defaults, "title");
    mYear =      fetchValue(6, defaults, "year");
    if (mId.isEmpty()) mId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (mYear.isEmpty()) {
        QDate date;
        mYear = QString::asprintf("%d", date.currentDate().year());
    }

    // create the zip file from the book
    writeMimetype();
    writeContainerXml();
    if (!mCover.isEmpty()) writeCover();
    writePageTemplate();
    writeStylesheet();
    writeContentOpf();
    writeToc();

    int len = chapterNumWidth();
    TextEdit* edit = Main::ref().ui()->textEdit;
    auto images = edit->internalImages();
    auto urls = edit->externalImageUrls();
    QMap<QString, QString> jpgs;
    QList<QUrl> urlList = urls.values();
    for (auto i = 0; i < urlList.size(); ++i) {
        auto url = urlList[i];
        auto& imgData = newData({ });
        Main::ref().loadImageBytesFromUrl(url, [&](QByteArray data){ QImage img(data); images[url] = img; });
    }
    auto keys = images.keys();
    for (auto i = keys.begin(); i != keys.end(); ++i) {
        auto& imgData = newData({ });
        QImage image = images[*i];
        QString name = i->fileName();
        Main::ref().loadImageBytes(image, "JPG", [&](QByteArray data) { imgData = data; });
        StringList fileParts { name.split(".") };
        fileParts.takeLast();
        name = fileParts.join(".") + ".jpg";
        jpgs[i->toString()] = name;
        addEntry(QString("OEBPS/%1").arg(name), imgData);
    }
    int i = 1;
    for (auto& entry: mBook) {
        QString html = fixImages(jpgs, entry.second);
        addEntry(QString("OEBPS/chap%1.xhtml").arg(i, len, 10, QChar('0')), fixImages(jpgs, html));
        ++i;
    }
    zip_close(mZip);
    mZip = nullptr;
    mData.clear();
    return true;
}

bool EBookExporter::addEntry(const QString& name, const QString& value, bool compressed) {
    if (!mZip) return false;

    qDebug().noquote().nospace() << name + ": \n" + value;
    auto& bytes = newData(value.toUtf8());
    zip_source_t* src = zip_source_buffer(mZip, bytes.constData(), mData.last().size(), 0);   // 0 = do NOT free the buffer when done

    if (!src) return false;

    // Add to the zip under the given internal path
    auto& path = newData(name.toUtf8());
    zip_int64_t idx = zip_file_add(mZip, path.constData(), src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);

    if (idx < 0) {
        zip_source_free(src);
        return false;
    }

    if (!compressed && zip_set_file_compression(mZip, idx, ZIP_CM_STORE, 0)) {
        zip_source_free(src);
        return false;
    }

    return true;
}

bool EBookExporter::addEntry(const QString& name, const QByteArray& value, bool compressed) {
    if (!mZip) return false;

    qDebug().noquote().nospace() << name + ": [BINARY]";
    mData.append(value);
    zip_source_t* src = zip_source_buffer(mZip, mData.last().constData(), mData.last().size(), 0);   // 0 = do NOT free the buffer when done

    if (!src) return false;

    // Add to the zip under the given internal path
    mData.append(name.toUtf8());
    zip_int64_t idx = zip_file_add(mZip, mData.last().constData(), src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);

    if (idx < 0) {
        zip_source_free(src);
        return false;
    }

    if (!compressed && zip_set_file_compression(mZip, idx, ZIP_CM_STORE, 0)) {
        zip_source_free(src);
        return false;
    }

    return true;

}

QString EBookExporter::chapterManifest() {
    QString manifest = "";
    int len = chapterNumWidth();
    for (size_t i = 1; i <= mBook.size(); ++i) {
        manifest += QString("        <item id=\"chapter%1\" href=\"chap%1.xhtml\" media-type=\"application/xhtml+xml\" />\n")
                        .arg(i, len);
    }
    return manifest;
}

int EBookExporter::chapterNumWidth() {
    int start = mCover.isEmpty() ? 1 : 2;
    return (int) QString::number(mBook.size() + start).length();
}

QString EBookExporter::close() {
    return "</" + mTags.pop() + ">\n";
}

QString EBookExporter::convertHTML(const QString& qHtml) {
    QString html = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" + qHtml;
    html = replace(html, "<!DOCTYPE", ">", "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">");
    html = replace(html, "<html", ">", "<html xmlns=\"http://www.w3.org/1999/xhtml\">");
    html = replace(html, "<style ", ">", "");
    return replace(html, "<head", ">","<head><title>" + mTitle + "</title>");
}

QString EBookExporter::fixImages(QMap<QString, QString>& jpgs, const QString& qHtml) {
    QString html = qHtml;
    StringList keys { jpgs.keys() };
    for (auto& key: keys) {
        html = replace(html, "\"" + key, "\"", jpgs[key]);
    }
    return html;
}

QString EBookExporter::navPoints() {
    QString nav;
    int len = chapterNumWidth();
    int start = mCover.isEmpty() ? 1 : 2;
    int i = start;
    for (auto& entry: mBook) {
        Item& item = mNovel.findItem(entry.first);
        nav += QString("      <navPoint id=\"chapter%1\" playOrder=\"%2\">\n"
                       "          " + open("navLabel") +
                       "              " + open("text") + "%3" + close() +
                       "          " + close() +
                       "          <content src=\"chap%1.xhtml\"/>\n"
                       "      </navPoint>\n").arg(i - (start - 1), len, 10, QChar(0)).arg(i).arg(item.name());
        ++i;
    }
    return nav;
}

QByteArray &EBookExporter::newData(const QByteArray& from) {
    mData.append(from);
    return mData.last();
}

void EBookExporter::novelToBook() {
    TextEdit build;
    Main::ref().setupHtml(build);
    qlonglong currentId = -1;
    bool firstScene = true;
    auto& prefs = Main::ref().prefs();
    for (int i = 0; i < mItemIds.count(); ++i) {
        Item& item = mNovel.findItem(mItemIds[i]);
        if (item.hasTag(mChapterTag)) {
            if (currentId != -1) mBook[currentId] = convertHTML(build.toHtml());
            currentId = item.id();
            build.setHtml(item.html());
            firstScene = true;
        } else if (currentId != -1) {
            if (item.hasTag(mCoverTag) || item.hasTag(mSceneTag)) {
                auto cursor = QTextCursor(build.document());
                cursor.movePosition(QTextCursor::End);
                if (firstScene) firstScene = false;
                else if (prefs.useSeparator()) cursor.insertHtml("<br/><center>" + prefs.separator() + "</center><br/>");
                cursor.insertHtml(item.html());
            }
        }
    }
    if (currentId != -1) mHtml[currentId] = convertHTML(build.toHtml());
}

QString EBookExporter::open(const QString& tag) {
    mTags.push(tag);
    return "<" + tag + ">\n";
}

QString EBookExporter::replace(const QString& qHtml, const QString& front, const QString& back, const QString& with) {
    QString html = qHtml;
    int pos = html.indexOf(front);
    if (pos == -1) return qHtml;
    QString left = html.left(pos);
    html = html.mid(pos);
    pos = html.indexOf(back);
    if (pos == -1) return qHtml;
    QString right = html.mid(pos + back.length());
    return left + with + right;
}

QString EBookExporter::spineTOC() {
    QString spine;
    int len = chapterNumWidth();
    for (size_t i = 1; i <= mBook.size(); ++i) {
        spine += QString("        <itemref idref=\"chapter%1\" />\n").arg(i, len);
    }
    return spine;
}

bool EBookExporter::writeContentOpf() {
    bool hasCover = !mCover.isEmpty();
    return addEntry("OEBPS/content.opf",
                   ("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
                    "<package xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"BookID\" version=\"2.0\" >\n"
                    "    <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:opf=\"http://www.idpf.org/2007/opf\">\n"
                    "        " + open("dc:title") + mTitle + close() +
                    "        <dc:creator opf:role=\"aut\">" + mAuthor + "</dc:creator>\n"
                    "        " + open("dc:language") + mLanguage + close() +
                    "        " + open("dc:rights") + mRights + close() +
                    "        " + open("dc:publisher") + mPublisher + close() +
                     "        " + open("dc:date") + mYear + close() +
                    "        <dc:identifier id=\"BookID\" opf:scheme=\"UUID\">" + mId +"</dc:identifier>\n" +
                   (hasCover ? "        <meta name=\"cover\" content=\"images/Cover.jpg\" />\n" : "") +
                    "    </metadata>\n"
                    "    " + open("manifest") +
                   (hasCover ? "        <item id=\"cover_jpg\" href=\"images/Cover.jpg\" media-type=\"image/jpeg\" />" : "") +
                    "        <item id=\"ncx\" href=\"toc.ncx\" media-type=\"application/x-dtbncx+xml\" />\n"
                    "        <item id=\"style\" href=\"stylesheet.css\" media-type=\"text/css\" />\n"
                    "        <item id=\"pagetemplate\" href=\"page-template.xpgt\" media-type=\"application/vnd.adobe-page-template+xml\" />\n" +
                   (hasCover ? "        <item id=\"cover_html\" href=\"Cover.xhtml\" media-type=\"application/xhtml+xml\" />\n" : "") +
                    chapterManifest() + "\n"
                    "    " + close() +
                    "    <spine toc=\"ncx\">\n" +
                   (hasCover ? "        <itemref idref=\"cover_html\" />\n" : "") +
                    spineTOC() +
                    "    </spine>\n"
                    "</package>"));
}

bool EBookExporter::writeContainerXml() {
    return addEntry("META-INF/container.xml", QString("") +
                    "<?xml version=\"1.0\"?>"
                    "<container xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\" version=\"1.0\">"
                    "  " + open("rootfiles") +
                    "    <rootfile media-type=\"application/oebps-package+xml\" full-path=\"OEBPS/content.opf\"/>"
                    "  " + close() +
                    "</container>");
}

bool EBookExporter::writeCover() {
    if (!mCover.isEmpty()) {
        auto& data = newData({ });
        Main::ref().loadImageBytesFromUrl({ mCover }, [&](QByteArray qBA) { data = qBA; });
        return addEntry("OEBPS/images/Cover.jpg", data) &&
               addEntry("OEBPS/Cover.xhtml", QString("") +
                        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                        + open("html") +
                        "   " + open("<body>") +
                        "     <img align=\"center\" height=\"100%\" src=\"images/Cover.jpg\"/><br/>&nbsp;\n"
                        "   " + close()
                        + close());
    }
    return true;
}

bool EBookExporter::writeMimetype() {
    return addEntry("mimetype", QString("application/epub+zip"), false);
}

bool EBookExporter::writePageTemplate() {
    return addEntry("OEBPS/page-template.xpgt", QString("") +
                    "<ade:template xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:ade=\"http://ns.adobe.com/2006/ade\"\n"
                    "         xmlns:fo=\"http://www.w3.org/1999/XSL/Format\">\n"
                    "  <fo:layout-master-set>\n"
                    "    <fo:simple-page-master master-name=\"single_column\">\n"
                    "        <fo:region-body margin-bottom=\"3pt\" margin-top=\"0.5em\" margin-left=\"3pt\" margin-right=\"3pt\"/>\n"
                    "    </fo:simple-page-master>\n"
                    "    <fo:simple-page-master master-name=\"single_column_head\">\n"
                    "        <fo:region-before extent=\"8.3em\"/>\n"
                    "        <fo:region-body margin-bottom=\"3pt\" margin-top=\"6em\" margin-left=\"3pt\" margin-right=\"3pt\"/>\n"
                    "    </fo:simple-page-master>\n"
                    "    <fo:simple-page-master master-name=\"two_column\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\">\n"
                    "        <fo:region-body column-count=\"2\" column-gap=\"10pt\"/>\n"
                    "    </fo:simple-page-master>\n"
                    "    <fo:simple-page-master master-name=\"two_column_head\" margin-bottom=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\">\n"
                    "        <fo:region-before extent=\"8.3em\"/>\n"
                    "        <fo:region-body column-count=\"2\" margin-top=\"6em\" column-gap=\"10pt\"/>\n"
                    "    </fo:simple-page-master>\n"
                    "    <fo:simple-page-master master-name=\"three_column\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\">\n"
                    "        <fo:region-body column-count=\"3\" column-gap=\"10pt\"/>\n"
                    "    </fo:simple-page-master>\n"
                    "    <fo:simple-page-master master-name=\"three_column_head\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\">\n"
                    "        <fo:region-before extent=\"8.3em\"/>\n"
                    "        <fo:region-body column-count=\"3\" margin-top=\"6em\" column-gap=\"10pt\"/>\n"
                    "    </fo:simple-page-master>\n"
                    "    <fo:page-sequence-master>\n"
                    "        <fo:repeatable-page-master-alternatives>\n"
                    "            <fo:conditional-page-master-reference master-reference=\"three_column_head\" page-position=\"first\" ade:min-page-width=\"80em\"/>\n"
                    "            <fo:conditional-page-master-reference master-reference=\"three_column\" ade:min-page-width=\"80em\"/>\n"
                    "            <fo:conditional-page-master-reference master-reference=\"two_column_head\" page-position=\"first\" ade:min-page-width=\"50em\"/>\n"
                    "            <fo:conditional-page-master-reference master-reference=\"two_column\" ade:min-page-width=\"50em\"/>\n"
                    "            <fo:conditional-page-master-reference master-reference=\"single_column_head\" page-position=\"first\" />\n"
                    "            <fo:conditional-page-master-reference master-reference=\"single_column\"/>\n"
                    "        </fo:repeatable-page-master-alternatives>\n"
                    "    </fo:page-sequence-master>\n"
                    "  </fo:layout-master-set>\n"
                    "  <ade:style>\n"
                    "    <ade:styling-rule selector=\".title_box\" display=\"adobe-other-region\" adobe-region=\"xsl-region-before\"/>\n"
                    "  </ade:style>\n"
                    "</ade:template>\n");
}

bool EBookExporter::writeStylesheet() {
    return addEntry("OEBPS/stylesheet.css", QString("") +
                    "/* Style Sheet */\n"
                    "/* This defines styles and classes used in the book */\n"
                    "body { margin-left: 5%; margin-right: 5%; margin-top: 5%; margin-bottom: 5%; text-align: justify; }\n"
                    "pre { font-size: x-small; }\n"
                    "h1 { text-align: center; }\n"
                    "h2 { text-align: center; }\n"
                    "h3 { text-align: center; }\n"
                    "h4 { text-align: center; }\n"
                    "h5 { text-align: center; }\n"
                    "h6 { text-align: center; }\n"
                    ".CI {\n"
                    "    text-align:center;\n"
                    "    margin-top:0px;\n"
                    "    margin-bottom:0px;\n"
                    "    padding:0px;\n"
                    "    }\n"
                    ".center   {text-align: center;}\n"
                    ".smcap    {font-variant: small-caps;}\n"
                    ".u        {text-decoration: underline;}\n"
                    ".bold     {font-weight: bold;}\n");
}

bool EBookExporter::writeToc() {
    return addEntry("OEBPS/toc.ncx",
                   ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<ncx xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\">\n"
                     "   " + open("head") +
                    "       <meta name=\"dtb:uid\" content=\"" + mId + "\"/>\n"
                    "       <meta name=\"dtb:depth\" content=\"1\"/>\n"
                    "       <meta name=\"dtb:totalPageCount\" content=\"0\"/>\n"
                    "       <meta name=\"dtb:maxPageNumber\" content=\"0\"/>\n"
                    "   " + close() +
                    "   " + open("docTitle") +
                    "       " + open("text") + mTitle + close() +
                    "   " + close() +
                    "   " + open("navMap") +
                   (!mCover.isEmpty() ?
                    "      <navPoint id=\"cover\" playOrder=\"1\">\n"
                    "          " + open("navLabel") +
                    "              " + open("text") + "Cover" + close() +
                    "          " + close() +
                    "          <content src=\"Cover.xhtml\"/>\n"
                    "      </navPoint>\n" : "") +
                    navPoints() +
                    "   " + close() +
                    "</ncx>"));
}
