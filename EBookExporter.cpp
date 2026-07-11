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

const QString operator<<(const QString& a, const QString& b) { return a + b; }
const QString operator<<(const QString& a, const char* b)    { return a + b; }
const QString operator<<(const char* a, const QString& b)    { return a + b; }

EBookExporter::~EBookExporter() {
    if (mZip) zip_close(mZip);
    mZip = nullptr;
}

bool EBookExporter::convert() {
    if (mZip) return true;

    mHasCover = false;
    int error = 0;
    QByteArray path = mFilename.toUtf8();
    mZip = zip_open(path.constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!mZip) {
        qWarning() << "Failed to open EPUB for writing:" << mFilename << "error:" << error;
        return false;
    }

    auto defaults = collectMetadataDefaults();
    mTitle =     fetchValue(0, defaults, "title");
    mAuthor =    fetchValue(1, defaults, "author");
    mCover =     fetchValue(2, defaults, "cover");
    mRights =    fetchValue(3, defaults, "title");
    mLanguage =  fetchValue(4, defaults, "language");
    mId =        fetchValue(5, defaults, "id");
    mPublisher = fetchValue(6, defaults, "publisher");
    mYear =      fetchValue(7, defaults, "year");

    auto& prefs = Main::ref().prefs();
    QString ext = "." + fileExtension();
    prefs["title" + ext] =     mTitle;
    prefs["author" + ext] =    mAuthor;
    prefs["cover" + ext] =     mCover;
    prefs["rights" + ext] =    mRights;
    prefs["language" + ext] =  mLanguage;
    prefs["id" + ext] =        mId;
    prefs["publisher" + ext] = mPublisher;
    prefs["year" + ext] =      mYear;

    mCoverImage = "";
    if (!mCover.isEmpty()) {
        mCoverTag = "";
        if (mCover.endsWith(".png",  Qt::CaseInsensitive) ||
            mCover.endsWith(".jpg",  Qt::CaseInsensitive) ||
            mCover.endsWith(".jpeg", Qt::CaseInsensitive) ||
            mCover.endsWith(".jpe",  Qt::CaseInsensitive) ||
            mCover.endsWith(".bmp",  Qt::CaseInsensitive) ||
            mCover.endsWith(".gif",  Qt::CaseInsensitive) ||
            mCover.endsWith(".webp", Qt::CaseInsensitive) ||
            mCover.endsWith(".pgm",  Qt::CaseInsensitive) ||
            mCover.endsWith(".ppm",  Qt::CaseInsensitive) ||
            mCover.endsWith(".pbm",  Qt::CaseInsensitive) ||
            mCover.endsWith(".xpm",  Qt::CaseInsensitive) ||
            mCover.endsWith(".xbm",  Qt::CaseInsensitive)) mCoverImage = mCover;
        else mCoverTag = mCover;
    }
    novelToBook();

    if (mId.isEmpty()) mId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (mYear.isEmpty()) {
        QDate date;
        mYear = QString::asprintf("%d", date.currentDate().year());
    }

    // create the zip file from the book
    writeMimetype();
    writeContainerXml();
    writeCover();
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
        Main::ref().loadImageBytesFromUrl(url, [&](QByteArray data){ QImage img(data); images[url] = img; });
    }
    if (!mCoverImage.isEmpty()) Main::ref().loadImageBytesFromUrl(mCoverImage, [&](QByteArray data){ QImage img(data); images[mCoverImage] = img; });
    auto keys = images.keys();
    for (auto i = keys.begin(); i != keys.end(); ++i) {
        auto& imgData = newData({ });
        QImage image = images[*i];
        QString name = i->fileName();
        if (name.isEmpty()) name = i->host();
        Main::ref().loadImageBytes(image, "JPG", [&](QByteArray data) { imgData = data; });
        StringList fileParts { name.split("/") };
        name = fileParts.takeLast() + ".jpg";
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

//qDebug().noquote().nospace() << name + ":\n" + value + "\n";
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

//qDebug().noquote().nospace() << name + ":\n[BINARY]\n";
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
    for (size_t i = 1; i <= mBook.size(); ++i) manifest += QString("        " << openCloseIt("item", "id=\"chapter%1\" href=\"chap%1.xhtml\" media-type=\"application/xhtml+xml\"")).arg(i, len, 10, QChar('0'));
    return manifest;
}

int EBookExporter::chapterNumWidth() {
    int start = mCover.isEmpty() ? 1 : 2;
    return (int) QString::number(mBook.size() + start).length();
}

QString EBookExporter::closeIt() {
    return "</" + mTags.pop() + ">\n";
}

// This function is very sensitive to the qt version ... tread carefully!
QString EBookExporter::convertHTML(const QString& qHtml) {
    QString html = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" + qHtml;
    html = replace(html, "<!DOCTYPE", ">", "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">");
    html = replace(html, "<html", ">", "<html xmlns=\"http://www.w3.org/1999/xhtml\">");
    html = replace(html, "<style", "</style>", "");
    html = replace(html, "</head", ">", "");
    html = replace(html, "<head", ">","<head><title>" + mTitle + "</title></head>");
    html = replaceAll(html, "<meta", "/>","");
    html = replaceAll(html, " align=\"", "\" ", " ");
    return html;
}

QString EBookExporter::fixImages(QMap<QString, QString>& jpgs, const QString& qHtml) {
    QString html = qHtml;
    StringList keys { jpgs.keys() };
    for (auto& key: keys) {
        html = replace(html, "\"" + key, "\"", jpgs[key]);
    }
    return html;
}

bool EBookExporter::hasCover() {
    return !mCoverHtml.isEmpty() || !mCoverImage.isEmpty();
}

QString EBookExporter::jpegManifest() {
    QString manifest;
    TextEdit* edit = Main::ref().ui()->textEdit;
    auto images = edit->internalImages();
    auto keys = images.keys();
    auto i = keys.begin();
    for (; i != keys.end(); ++i) {
        QImage image = images[*i];
        QString name = i->fileName();
        if (name.isEmpty()) name = i->host();
        StringList fileParts { name.split("/") };
        name = fileParts.takeLast();
        manifest += QString("        " << openCloseIt("item", "id=\"%1_jpg\" href=\"%1.jpg\" media-type=\"image/jpeg\"")).arg(name);
    }
    if (!mCoverImage.isEmpty()) {
        QString name;
        QUrl img(mCoverImage);
        name = img.fileName();
        if (name.contains('.')) name = name.left(name.indexOf('.'));
        manifest += QString("        " << openCloseIt("item", "id=\"%1_jpg\" href=\"%1.jpg\" media-type=\"image/jpeg\"")).arg(name);
    }
    return manifest;
}

QString EBookExporter::navPoints() {
    QString nav;
    int len = chapterNumWidth();
    int start = (mCoverTag.isEmpty() && mCoverImage.isEmpty()) ? 1 : 2;
    int i = start;
    for (auto& entry: mBook) {
        Item& item = mNovel.findItem(entry.first);
        nav += QString("      " << openIt("navPoint", "id=\"chapter%1\" playOrder=\"%2\"", true) <<
                       "          " << openIt("navLabel", true) <<
                       "              " << openIt("text", false) << "%3" << closeIt() <<
                       "          " << closeIt() <<
                       "          " << openCloseIt("content", "src=\"chap%1.xhtml\"") <<
                       "      " << closeIt()).arg(i - (start - 1), len, 10, QChar('0')).arg(i).arg(item.name().trimmed());
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
                if (item.hasTag(mCoverTag)) {
                    mHasCover = true;
                    mCoverHtml = convertHTML(item.html());
                    continue;
                }
                auto cursor = QTextCursor(build.document());
                cursor.movePosition(QTextCursor::End);
                if (firstScene) firstScene = false;
                else if (prefs.useSeparator()) cursor.insertHtml("<br/><center>" + prefs.separator() + "</center><br/>");
                cursor.insertHtml(item.html());
            }
        } else if (item.hasTag(mCoverTag)) {
            mHasCover = true;
            mCoverHtml = convertHTML(item.html());
        }
    }
    if (currentId != -1) mHtml[currentId] = convertHTML(build.toHtml());
}

QString EBookExporter::openCloseIt(const QString& tag, const QString& args) {
    QString value = "<" + tag;
    if (!args.isEmpty()) value += " " + args + " ";
    return value + "/>\n";
}

QString EBookExporter::openIt(const QString& tag, const QString& args, bool nl) {
    mTags.push(tag);
    QString value = "<" + tag;
    if (!args.isEmpty()) value += " " + args;
    return value + ">" + (nl ? "\n" : "");
}

QString EBookExporter::openIt(const QString& tag, bool nl) {
    return openIt(tag, "", nl);
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

QString EBookExporter::replaceAll(const QString& qHtml, const QString& front, const QString& back, const QString& with) {
    QString html = qHtml;
    while (html.contains(front)) html = replace(html,front, back, with);
    return html;
}

QString EBookExporter::spineTOC() {
    QString spine;
    int len = chapterNumWidth();
    for (size_t i = 1; i <= mBook.size(); ++i) {
        spine += "        " + openCloseIt("itemref", QString("idref=\"chapter%1\"").arg(i, len, 10, QChar('0')));
    }
    return spine;
}

bool EBookExporter::writeContentOpf() {
    return addEntry("OEBPS/content.opf",
                    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" <<
                     openIt("package", "xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"BookID\" version=\"2.0\"", true) <<
                     "    " << openIt("metadata", "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:opf=\"http://www.idpf.org/2007/opf\"", true) <<
                     "        " << openIt("dc:title") << mTitle << closeIt() <<
                     "        " << openIt("dc:creator", "opf:role=\"aut\"", false) << mAuthor << closeIt() <<
                     "        " << openIt("dc:language") << mLanguage << closeIt() <<
                     "        " << openIt("dc:rights") << mRights << closeIt() <<
                     "        " << openIt("dc:publisher") << mPublisher << closeIt() <<
                     "        " << openIt("dc:date") << mYear << closeIt() <<
                     "        " << openIt("dc:identifier", "id=\"BookID\" opf:scheme=\"UUID\"", false) << mId << closeIt() <<
                    (hasCover() ? "        " << openCloseIt("meta", "name=\"cover\" content=\"images/Cover.jpg\"") : "") <<
                     "    " << closeIt() <<
                     "    " << openIt("manifest", true) <<
                    (hasCover() ? "        " << openCloseIt("item", "id=\"cover_jpg\" href=\"images/Cover.jpg\" media-type=\"image/jpeg\"") : "") <<
                     "        " << openCloseIt("item", "id=\"ncx\" href=\"toc.ncx\" media-type=\"application/x-dtbncx+xml\"") <<
                     "        " << openCloseIt("item", "id=\"style\" href=\"stylesheet.css\" media-type=\"text/css\"") <<
                     "        " << openCloseIt("item", "id=\"pagetemplate\" href=\"page-template.xpgt\" media-type=\"application/vnd.adobe-page-template+xml\"") <<
                     jpegManifest() <<
                    (hasCover() ? "        " << openCloseIt("item", "id=\"cover_html\" href=\"Cover.xhtml\" media-type=\"application/xhtml+xml\"") : "") <<
                     chapterManifest() << "\n" <<
                     "    " << closeIt() <<
                     "    " << openIt("spine", "toc=\"ncx\"", true) <<
                    (hasCover() ? "        " << openCloseIt("itemref", "idref=\"cover_html\"") : "") <<
                     spineTOC() <<
                     "    " << closeIt() <<
                     closeIt());
}

bool EBookExporter::writeContainerXml() {
    return addEntry("META-INF/container.xml", QString("") <<
                    "<?xml version=\"1.0\"?>" <<
                    openIt("container", "xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\" version=\"1.0\"", true) <<
                    "  " << openIt("rootfiles", true) <<
                    "    " << openCloseIt("rootfile", "media-type=\"application/oebps-package+xml\" full-path=\"OEBPS/content.opf\"") <<
                    "  " << closeIt() <<
                    closeIt());
}

bool EBookExporter::writeCover() {
    if (hasCover()) {
        TextEdit edit;
        auto& data = newData({ });
        if (mCoverHtml.isEmpty()) {
            QImage img;
            img.load(mCoverImage);
            auto size = img.size();
            if (size.width() > size.height()) img = img.scaledToWidth(1600, Qt::SmoothTransformation);
            else img = img.scaledToHeight(2560, Qt::SmoothTransformation);
            Main::ref().loadImageBytes(img, "JPG", [&](const QByteArray& from) { data = from; });
        }
        else {
            auto& images = Main::ref().ui()->textEdit->internalImages();
            edit.setInternalImages(images);
            edit.setHtml(mCoverHtml);
            QTextDocument *doc = edit.document();
            doc->setPageSize(QSizeF(2560, 1600));
            QImage img(2560, 1600, QImage::Format_ARGB32);
            img.setDotsPerMeterX(11811); // 300 DPI
            img.setDotsPerMeterY(11811);
            img.fill(Qt::transparent);
            QPainter p(&img);
            doc->drawContents(&p);
            p.end();
            Main::ref().loadImageBytes(img, "JPG", [&](const QByteArray& from) { data = from; });
        }
        return addEntry("OEBPS/Cover.jpg", data) &&
               addEntry("OEBPS/Cover.xhtml", QString("") <<
                        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                        << openIt("html", true) <<
                        "   " << openIt("<body>", true) <<
                        "     " << openCloseIt("img", "src=\"Cover.jpg\"") << "<br/>&nbsp;\n" <<
                        "   " << closeIt() <<
                        closeIt());
    }
    return true;
}

bool EBookExporter::writeMimetype() {
    return addEntry("mimetype", QString("application/epub+zip"), false);
}

bool EBookExporter::writePageTemplate() {
    return addEntry("OEBPS/page-template.xpgt", QString("") <<
                    openIt("ade:template", QString("xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:ade=\"http://ns.adobe.com/2006/ade\"\n") <<
                                                          "         xmlns:fo=\"http://www.w3.org/1999/XSL/Format\"") <<
                    "  " << openIt("fo:layout-master-set") <<
                    "    " << openIt("fo:simple-page-master", "master-name=\"single_column\"") <<
                    "      " << openCloseIt("fo:region-body", "margin-bottom=\"3pt\" margin-top=\"0.5em\" margin-left=\"3pt\" margin-right=\"3pt\"") <<
                    "    " << closeIt() <<
                    "    " << openIt("fo:simple-page-master", "master-name=\"single_column_head\"") <<
                    "      " << openCloseIt("fo:region-before", "extent=\"8.3em\"") <<
                    "      " << openCloseIt("fo:region-body", "margin-bottom=\"3pt\" margin-top=\"6em\" margin-left=\"3pt\" margin-right=\"3pt\"") <<
                    "    " << closeIt() <<
                    "    " << openIt("fo:simple-page-master", "master-name=\"two_column\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\"") <<
                    "      " << openCloseIt("fo:region-body", "column-count=\"2\" column-gap=\"10pt\"") <<
                    "    " << closeIt() <<
                    "    " << openIt("fo:simple-page-master", "master-name=\"two_column_head\" margin-bottom=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\"") <<
                    "      " << openCloseIt("fo:region-before", "extent=\"8.3em\"") <<
                    "      " << openCloseIt("fo:region-body", "column-count=\"2\" margin-top=\"6em\" column-gap=\"10pt\"") <<
                    "    " << closeIt() <<
                    "    " << openIt("fo:simple-page-master", "master-name=\"three_column\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\"") <<
                    "      " << openCloseIt("fo:region-body", "column-count=\"3\" column-gap=\"10pt\"") <<
                    "    " << closeIt() <<
                    "    " << openIt("fo:simple-page-master", "master-name=\"three_column_head\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\"") <<
                    "      " << openCloseIt("fo:region-before", "extent=\"8.3em\"") <<
                    "      " << openCloseIt("fo:region-body", "column-count=\"3\" margin-top=\"6em\" column-gap=\"10pt\"") <<
                    "    " << closeIt() <<
                    "    " << openIt("fo:page-sequence-master>") <<
                    "      " << openIt("fo:repeatable-page-master-alternatives>") <<
                    "        " << openCloseIt("fo:conditional-page-master-reference", "master-reference=\"three_column_head\" page-position=\"first\" ade:min-page-width=\"80em\"") <<
                    "        " << openCloseIt("fo:conditional-page-master-reference", "master-reference=\"three_column\" ade:min-page-width=\"80em\"") <<
                    "        " << openCloseIt("fo:conditional-page-master-reference", "master-reference=\"two_column_head\" page-position=\"first\" ade:min-page-width=\"50em\"") <<
                    "        " << openCloseIt("fo:conditional-page-master-reference", "aster-reference=\"two_column\" ade:min-page-width=\"50em\"") <<
                    "        " << openCloseIt("fo:conditional-page-master-reference", "master-reference=\"single_column_head\" page-position=\"first\"") <<
                    "        " << openCloseIt("fo:conditional-page-master-reference", "master-reference=\"single_column\"") <<
                    "      " << closeIt() <<
                    "    " << closeIt() <<
                    "  " << closeIt() <<
                    "  " << openIt("ade:style") <<
                    "    " << openCloseIt("ade:styling-rule", "selector=\".title_box\" display=\"adobe-other-region\" adobe-region=\"xsl-region-before\"") <<
                    "  " << closeIt() <<
                    closeIt());
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
                    "}\n"
                    ".center   { text-align: center; }\n"
                    ".smcap    { font-variant: small-caps; }\n"
                    ".u        { text-decoration: underline; }\n"
                    ".bold     { font-weight: bold; }\n");
}

bool EBookExporter::writeToc() {
    return addEntry("OEBPS/toc.ncx",
                    QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") <<
                    openIt("ncx", "xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\"", true) <<
                    "   " << openIt("head", true) <<
                    "      " << openCloseIt("meta", "name=\"dtb:uid\" content=\"" << mId << "\"") <<
                    "      " << openCloseIt("meta", "name=\"dtb:depth\" content=\"1\"") <<
                    "      " << openCloseIt("meta", "name=\"dtb:totalPageCount\" content=\"0\"") <<
                    "      " << openCloseIt("meta", "name=\"dtb:maxPageNumber\" content=\"0\"") <<
                    "   " << closeIt() <<
                    "   " << openIt("docTitle", true) <<
                    "      " << openIt("text") << mTitle << closeIt() <<
                    "   " << closeIt() <<
                    "   " << openIt("navMap", true) <<
                   (hasCover() ?
                    "      " << openIt("navPoint", "id=\"cover_html\" playOrder=\"1\"", true) <<
                    "         " << openIt("navLabel", true) <<
                    "            " << openIt("text") << "Cover" << closeIt() <<
                    "         " << closeIt() <<
                    "         " << openCloseIt("content", "src=\"Cover.xhtml\"") <<
                    "      " << closeIt() : "") <<
                    navPoints() <<
                    "   " << closeIt() <<
                    closeIt());
}
