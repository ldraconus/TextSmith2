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
        QByteArray imgData;
        loadImageBytesFromUrl(url, [&](QByteArray data){ QImage img(data); images[url] = img; });
    }
    auto keys = images.keys();
    QByteArray imgData;
    for (auto i = keys.begin(); i != keys.end(); ++i) {
        QImage image = images[*i];
        QString name = i->fileName();
        loadImageBytes(image, [&](QByteArray data) { imgData = data; });
        StringList fileParts { name.split(".") };
        fileParts.takeLast();
        name = fileParts.join(".") + ".jpg";
        jpgs[i->toString()] = name;
        addEntry(QString("OEBPS/%1").arg(name), imgData);
    }
    int i = 1;
    for (auto& entry: mBook) {
        QString html = fixImages(jpgs, entry.second);
        addEntry(QString("OEBPS/chap%1.xhtml").arg(i, len), fixImages(jpgs, html));
        ++i;
    }
    zip_close(mZip);
    mZip = nullptr;
    return true;
}

bool EBookExporter::addEntry(const QString& name, const QString& value) {
    if (!mZip) return false;

    QByteArray bytes(value.toUtf8());
    QByteArray path(name.toUtf8());

    zip_source_t* src = zip_source_buffer(mZip, bytes.constData(), bytes.size(), 0);   // 0 = do NOT free the buffer when done

    if (!src) return false;

    // Add to the zip under the given internal path
    zip_int64_t idx = zip_file_add(mZip, path.constData(), src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);

    if (idx < 0) {
        zip_source_free(src);
        return false;
    }

    return true;
}

bool EBookExporter::addEntry(const QString& name, const QByteArray& value) {
    if (!mZip) return false;

    QByteArray path(name.toUtf8());

    zip_source_t* src = zip_source_buffer(mZip, value.constData(), value.size(), 0);   // 0 = do NOT free the buffer when done

    if (!src) return false;

    // Add to the zip under the given internal path
    zip_int64_t idx = zip_file_add(mZip, path.constData(), src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);

    if (idx < 0) {
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

void EBookExporter::loadImageBytes(const QImage& img, std::function<void(QByteArray)> callback) {
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "JPG");
    callback(data);
}

void EBookExporter::loadImageBytesFromUrl(const QUrl& url, std::function<void(QByteArray)> callback) {
    if (url.isLocalFile() || url.scheme() == "qrc" || url.scheme().isEmpty()) {
        // Local file or resource
        QImage img(url.toLocalFile());
        loadImageBytes(img, callback);
        return;
    }

    // Remote URL
    auto* nam = new QNetworkAccessManager();
    QNetworkReply* reply = nam->get(QNetworkRequest(url));

    QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();
        QImage img(data);
        loadImageBytes(img, callback);
    });
}

QString EBookExporter::navPoints() {
    QString nav;
    int len = chapterNumWidth();
    int start = mCover.isEmpty() ? 1 : 2;
    int i = start;
    for (auto& entry: mBook) {
        Item& item = mNovel.findItem(entry.first);
        nav += QString("      <navPoint id=\"chapter%1\" playOrder=\"%2\">\n"
                       "          <navLabel>\n"
                       "              <text>%3</text>\n"
                       "          </navLabel>\n"
                       "          <content src=\"chap%1.xhtml\"/>\n"
                       "      </navPoint>\n").arg(i - (start - 1), len).arg(i).arg(item.name());
        ++i;
    }
    return nav;
}

void EBookExporter::novelToBook() {
    TextEdit build;
    Main::ref().setupHtml(build);
    qlonglong currentId = -1;
    for (int i = 0; i < mItemIds.count(); ++i) {
        Item& item = mNovel.findItem(mItemIds[i]);
        if (item.hasTag(mChapterTag)) {
            if (currentId != -1) mBook[currentId] = convertHTML(build.toHtml());
            currentId = item.id();
            build.setHtml(item.html());
        } else if (currentId != -1) {
            if (item.hasTag(mCoverTag) && item.hasTag(mSceneTag)) {
                auto cursor = QTextCursor(build.document());
                cursor.movePosition(QTextCursor::End);
                cursor.insertHtml(item.html());
            }
        }
    }
    if (currentId != -1) mHtml[currentId] = convertHTML(build.toHtml());
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
                    "        <dc:title>" + mTitle + "</dc:title>\n"
                    "        <dc:creator opf:role=\"aut\">" + mAuthor + "</dc:creator>\n"
                    "        <dc:language>" + mLanguage + "</dc:language>\n"
                    "        <dc:rights>" + mRights + "</dc:rights>\n"
                    "        <dc:publisher>" + mPublisher + "</dc:publisher>\n"
                    "        <dc:date>" + mYear + "</dc:data>\n"
                    "        <dc:identifier id=\"BookID\" opf:scheme=\"UUID\">" + mId +"</dc:identifier>\n" +
                   (hasCover ? "        <meta name=\"cover\" content=\"images/Cover.jpg\" />\n" : "") +
                    "    </metadata>\n"
                    "    <manifest>\n" +
                   (hasCover ? "        <item id=\"cover_jpg\" href=\"images/Cover.jpg\" media-type=\"image/jpeg\" />" : "") +
                    "        <item id=\"ncx\" href=\"toc.ncx\" media-type=\"application/x-dtbncx+xml\" />\n"
                    "        <item id=\"style\" href=\"stylesheet.css\" media-type=\"text/css\" />\n"
                    "        <item id=\"pagetemplate\" href=\"page-template.xpgt\" media-type=\"application/vnd.adobe-page-template+xml\" />\n" +
                   (hasCover ? "        <item id=\"cover_html\" href=\"Cover.xhtml\" media-type=\"application/xhtml+xml\" />\n" : "") +
                    chapterManifest() + "\n"
                    "    </manifest>\n"
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
                    "  <rootfiles>"
                    "    <rootfile media-type=\"application/oebps-package+xml\" full-path=\"OEBPS/content.opf\"/>"
                    "  </rootfiles>"
                    "</container>");
}

bool EBookExporter::writeCover() {
    if (!mCover.isEmpty()) {
        QByteArray data;
        loadImageBytesFromUrl({ mCover }, [&](QByteArray qBA) { data = qBA; });
        return addEntry("OEBPS/images/Cover.jpg", data) &&
               addEntry("OEBPS/Cover.xhtml", QString("") +
                        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                        "<html>\n"
                        "   <body>\n"
                        "     <img align=\"center\" height=\"100%\" src=\"images/Cover.jpg\"/><br/>&nbsp;\n"
                        "   </body>\n"
                        "</html>\n");
    }
    return true;
}

bool EBookExporter::writeMimetype() {
    return addEntry("mimetype", QString("application/epub+zip"));
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
                    "   <head>\n"
                    "       <meta name=\"dtb:uid\" content=\"" + mId + "\"/>\n"
                    "       <meta name=\"dtb:depth\" content=\"1\"/>\n"
                    "       <meta name=\"dtb:totalPageCount\" content=\"0\"/>\n"
                    "       <meta name=\"dtb:maxPageNumber\" content=\"0\"/>\n"
                    "   </head>\n"
                    "   <docTitle>\n"
                    "       <text>" + mTitle + "</text>\n"
                    "   </docTitle>\n"
                    "   <navMap>\n" +
                   (!mCover.isEmpty() ?
                    "      <navPoint id=\"cover_html\" playOrder=\"1\">\n"
                    "          <navLabel>\n"
                    "              <text>Cover</text>\n"
                    "          </navLabel>\n"
                    "          <content src=\"Cover.xhtml\"/>\n"
                    "      </navPoint>\n" : "") +
                    navPoints() +
                    "   </navMap>\n"
                    "</ncx>"));
}

#ifdef NOTDEF
bool EBookExporter::addFileFromData(const QString &filenameInEpub, const QByteArray &data) {
    QByteArray fname = filenameInEpub.toUtf8();
    zip_source* src = zip_source_buffer(mZip, data.constData(), data.size(), 0);
    if (!src) {
        qWarning() << "zip_source_buffer failed for" << filenameInEpub;
        return false;
    }

    zip_int64_t idx = zip_file_add(mZip, fname.constData(), src, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);
    if (idx < 0) {
        qWarning() << "zip_file_add failed for" << filenameInEpub;
        zip_source_free(src);
        return false;
    }

    return true;
}

bool EBookExporter::writeContentOpf()
{
    // Simple, minimal OPF. You can enrich this later.
    QString opf;
    opf += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    opf += "<package version=\"2.0\" xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"BookId\">\n";

    opf += "  <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n";
    opf += "    <dc:title>" + mTitle.toHtmlEscaped() + "</dc:title>\n";
    opf += "    <dc:creator opf:role=\"aut\">" + mAuthor.toHtmlEscaped() + "</dc:creator>\n";
    opf += "    <dc:rights>" + mRights + "</dc:rights>\n";
    opf += "    <dc:publisher>" + mPublisher + "</dc:publisher>\n";
    opf += "    <dc:date>" + mYear + "</dc:data>\n";
    opf += "    <dc:identifier id=\"BookId\">" + mId.toHtmlEscaped() + "</dc:identifier>\n";
    opf += "    <dc:language>" + mLanguage + "</dc:language>\n";
    opf += "  </metadata>\n";

    opf += "  <manifest>\n";
    int i = 0;
    for (int j = 0; j < mItemIds.count(); ++j) {
        auto id = mItemIds[j];
        Item& item = mNovel.findItem(id);
        if (!item.hasTag(mChapterTag)) continue;
        opf += "    <item id=\"chap" + QString::number(id) +
                            "\" href=\"chapter" + QString::number(++i) + ".xhtml"
                            "\" media-type=\"application/xhtml+xml\"/>\n";
    }
    opf += "  </manifest>\n";

    opf += "  <spine>\n";
    i = 0;
    for (int j = 0; j < mItemIds.count(); ++j) {
        auto id = mItemIds[j];
        Item& item = mNovel.findItem(id);
        if (!item.hasTag(mChapterTag)) continue;
        opf += "    <itemref idref=\"chapter" + QString::number(id) + "\"/>\n";
        ++i;
    }
    opf += "  </spine>\n";
    opf += "</package>\n";

    QByteArray data = opf.toUtf8();
    return addFileFromData("OEBPS/content.opf", data);
}

bool EBookExporter::writeContainerXml() {
    static const char containerXml[] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n"
        "  <rootfiles>\n"
        "    <rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n"
        "  </rootfiles>\n"
        "</container>\n";

    QByteArray data(containerXml);
    return addFileFromData("META-INF/container.xml", data);
}

bool EBookExporter::writeMimetype() {
    static const char mimetype[] = "application/epub+zip";

    zip_source* src = zip_source_buffer(mZip, mimetype, sizeof(mimetype) - 1, 0);
    if (!src) {
        qWarning() << "zip_source_buffer failed for mimetype";
        return false;
    }

    zip_int64_t idx = zip_file_add(mZip, "mimetype", src, ZIP_FL_ENC_UTF_8);
    if (idx < 0) {
        qWarning() << "zip_file_add failed for mimetype";
        zip_source_free(src);
        return false;
    }

    return true;
}

EBookExporter::~EBookExporter() {
    if (mZip) zip_close(mZip);
    mZip = nullptr;
}

QString EBookExporter::convertHTML(const QString& qHtml) {
    QString html = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" + qHtml;
    html = replace(html, "<!DOCTYPE", ">", "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">");
    html = replace(html, "<html", ">", "<html xmlns=\"http://www.w3.org/1999/xhtml\">");
    html = replace(html, "<style ", ">", "");
    return replace(html, "<head", ">","<head><title>" + mTitle + "</title>");
}

void EBookExporter::loadImageBytesFromUrl(const QUrl& url, std::function<void(QByteArray)> callback) {
    if (url.isLocalFile() || url.scheme() == "qrc" || url.scheme().isEmpty()) {
        // Local file or resource
        QImage img(url.toLocalFile());
        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");
        callback(data);
        return;
    }

    // Remote URL
    auto* nam = new QNetworkAccessManager();
    QNetworkReply* reply = nam->get(QNetworkRequest(url));

    QObject::connect(reply, &QNetworkReply::finished, [reply, callback]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();
        callback(data);
    });
}

bool EBookExporter::convert() {
    if (mFilename.isEmpty()) return false;

    int error = 0;
    QByteArray path = mFilename.toUtf8();
    mZip = zip_open(path.constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!mZip) {
        qWarning() << "Failed to open EPUB for writing: " << mFilename << ", error:" << error;
        return false;
    }

    if (!writeMimetype())     return false;
    if (!writeContainerXml()) return false;

    if (!writeContentOpf()) {
        qWarning() << "Failed to write content.opf";
        zip_close(mZip);
        mZip = nullptr;
        return false;
    }

    TextEdit build;
    build.setAcceptRichText(true);

    qlonglong currentId = -1;
    for (int i = 0; i < mItemIds.count(); ++i) {
        Item& item = mNovel.findItem(mItemIds[i]);
        if (item.hasTag(mChapterTag)) {
            if (currentId != -1) mHtml[currentId] = convertHTML(build.toHtml());
            currentId = item.id();
            build.setHtml(item.html());
        } else if (currentId != -1) {
            auto cursor = QTextCursor(build.document());
            cursor.movePosition(QTextCursor::End);
            cursor.insertHtml(item.html());
        }
    }
    if (currentId != -1) mHtml[currentId] = convertHTML(build.toHtml());

    TextEdit* text = Main::ref().ui()->textEdit;
    auto internalImagesMap = text->internalImages();
    auto externalImagesSet = text->externalImageUrls();
    QMap<QString, QByteArray> imageData;
    auto keys = internalImagesMap.keys();
    for (auto i = 0; i < keys.count(); ++i) {
        QUrl key = keys[i];
        QByteArray imgData;
        loadImageBytesFromUrl(key, [&](QByteArray data) { imgData = data; });
        QString newName = "images/image" + QString::number(i + 1);
        for (auto& html: mHtml) html.second = replace(html.second, "\"" + key.toString(), "\"", newName);
        addImage("images/image" + newName, imgData);
    }
    auto values = externalImagesSet.values();
    for (auto i = 0; i < values.count(); ++i) {
        QByteArray imgData;
        QUrl key(values[i]);
        loadImageBytesFromUrl(key, [&](QByteArray data) { imgData = data; });
        QString newName = "images/image" + QString::number(i + keys.count());
        for (auto& html: mHtml) html.second = replace(html.second, "\"" + key.toString(), "\"", newName);
        addImage("images/image" + newName, imgData);
    }

    int i = 0;
    for (int j = 0; j < mItemIds.count(); ) {
        Item& item = mNovel.findItem(mItemIds[j++]);
        if (!item.hasTag(mChapterTag)) continue;
        QString html = mHtml[item.id()];
        addXhtml("chapter" + QString::number(++i) + ".xhtml", html);
    }

    QByteArray coverBytes; /* your PNG/JPEG data */;
    addImage("images/cover.jpg", coverBytes);

    if (zip_close(mZip) != 0) {
        qWarning() << "Failed to close EPUB: " << mFilename;
        mZip = nullptr;
        return false;
    }

    mZip = nullptr;

    return true;
}

bool EBookExporter::addXhtml(const QString& filenameInEpub, const QString& xhtmlContent) {
    QByteArray data = xhtmlContent.toUtf8();
    return addFileFromData("OEBPS/" + filenameInEpub, data);
}

bool EBookExporter::addImage(const QString& filenameInEpub, const QByteArray& imageData) {
    return addFileFromData("OEBPS/" + filenameInEpub, imageData);
}
#endif
