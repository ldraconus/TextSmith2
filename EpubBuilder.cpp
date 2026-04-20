#include "EpubBuilder.h"

#include <zip.h>
#include <QDateTime>
#include <QDebug>

EpubBuilder::~EpubBuilder() {
    if (mZip) {
        zip_close(mZip);
    }
}

bool EpubBuilder::open() {
    if (mZip) return true;

    int error = 0;
    QByteArray path = mOutputPath.toUtf8();
    mZip = zip_open(path.constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!mZip) {
        qWarning() << "Failed to open EPUB for writing:" << mOutputPath << "error:" << error;
        return false;
    }

    if (!writeMimetype())      return false;
    if (!writeContainerXml())  return false;

    return true;
}

bool EpubBuilder::close() {
    if (!mZip) return true;

    if (!writeContentOpf()) {
        qWarning() << "Failed to write content.opf";
        zip_close(mZip);
        mZip = nullptr;
        return false;
    }

    if (zip_close(mZip) != 0) {
        qWarning() << "Failed to close EPUB:" << mOutputPath;
        mZip = nullptr;
        return false;
    }

    mZip = nullptr;
    return true;
}

bool EpubBuilder::writeMimetype() {
    // Spec: must be first file, uncompressed, stored at root, exact content
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

    // Force store (no compression)
    // zip_file_set_compression(m_zip, idx, ZIP_CM_STORE, 0);
    return true;
}

bool EpubBuilder::writeContainerXml() {
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

bool EpubBuilder::addFileFromData(const QString& filenameInEpub, const QByteArray& data) {
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

bool EpubBuilder::addXhtml(const QString& filenameInEpub, const QString& xhtmlContent) {
    if (!mZip && !open()) return false;

    QByteArray data = xhtmlContent.toUtf8();
    if (!addFileFromData("OEBPS/" + filenameInEpub, data)) return false;
    return true;
}

bool EpubBuilder::addImage(const QString& filenameInEpub, const QByteArray& imageData) {
    if (!mZip && !open()) return false;

    if (!addFileFromData("OEBPS/" + filenameInEpub, imageData)) return false;
    return true;
}

bool EpubBuilder::writeContentOpf() {
    // Simple, minimal OPF. You can enrich this later.
    QString opf;
    opf += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    opf += "<package version=\"2.0\" xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"BookId\">\n";

    opf += "  <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n";
    opf += "    <dc:title>" + mTitle.toHtmlEscaped() + "</dc:title>\n";
    opf += "    <dc:creator>" + mAuthor.toHtmlEscaped() + "</dc:creator>\n";
    opf += "    <dc:identifier id=\"BookId\">" + mIdentifier.toHtmlEscaped() + "</dc:identifier>\n";
    opf += "    <dc:language>en</dc:language>\n";
    opf += "  </metadata>\n";

    opf += "  <manifest>\n";
    for (auto [i, id]: enumerate(mItemIds)) {
        opf += "    <item id=\"" + QString::number(id)
            + "\" href=\"chap" + QString::number(id) + ".xhtml\""
            + "\" media-type=\"application/xhtml+xml" + "\" />\n";
    }
    opf += "  </manifest>\n";

    opf += "  <spine>\n";
    for (auto [i, id]: enumerate(mSpine)) opf += "    <itemref idref=\"" + id.toHtmlEscaped() + "\"/>\n";
    opf += "  </spine>\n";

    opf += "</package>\n";

    QByteArray data = opf.toUtf8();
    return addFileFromData("OEBPS/content.opf", data);
}
