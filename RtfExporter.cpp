#include "RtfExporter.h"
#include "PdfExporter.h"
#include "Main.h"
#include "ui_Main.h"

using namespace Words;

#include <QFileInfo>

QString RtfExporter::handleCover(const QString& cover) {
    QByteArray imgData;
    Main::ref().loadImageBytesFromUrl(cover, [&](QByteArray data){ imgData = data; });
    QImage img(imgData);
    QBuffer buffer(&imgData);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "BMP");
    QByteArray dib = imgData.mid(14);
    QString hex;
    hex.reserve(dib.size() * 2);
    for (auto& c: dib) hex += QString("%1").arg(int(c), 2, 16, QLatin1Char('0'));
    return "{\\pict\\dibitmap0\n" + hex + "\n}\n";
}

QString RtfExporter::handleCover(Item& item) {
    QString rtf;
    // for each image in item: handleCover(image url)
    return rtf;
}

QString RtfExporter::escapeText(const QString& str) {
    QString out;

    for (auto c: str) {
        char ch = c.toLatin1();
        if (ch == '\\') out += "\\\\";
        else if (ch == '{') out += "\\{}";
        else if (ch == '}')  out += "\\}";
        else {
            if ((ch & 0x80) == 0) out += c;
            else out += "\\u" + QString::number((int16_t) ch) + "?";
        }
    }

    return out;
}

QString RtfExporter::itemsToRtf(const QString& cover) {
    QString rtf = "{\\rtf1\\ansi\\deff0\n"
                  "{\\fonttbl{\\f0 " + Main::ref().ui()->textEdit->font().family() + ";}}\n";
    mMargins *= PointsPerInch * TwipsPerPoint;
    rtf += QString("\\margl%1\\margr%2\\margt%3\\margb%4\n")
               .arg(mMargins.left())
               .arg(mMargins.right())
               .arg(mMargins.top())
               .arg(mMargins.bottom());
    bool first = true;
    if (!cover.isEmpty()) {
        handleCover(cover);
        rtf += "\\page\n";
    }
    int dpi = Main::ref().ui()->textEdit->viewport()->logicalDpiX();
    for (auto& id: mItemIds) {
        Item& item = mNovel.findItem(id);
        if (!item.hasTag(mChapterTag) &&
            !item.hasTag(mSceneTag) &&
            !item.hasTag(mCoverTag)) continue;
        if (item.hasTag(mCoverTag)) {
            if (!first) rtf += "\\page\n";
            rtf += handleCover(item);
        }
        else if (item.hasTag(mChapterTag)) {
            if (!first) rtf += "\\page\n";
        }
        first = false;
        QFontMetrics metrics(Main::ref().ui()->textEdit->font());
        auto indent = metrics.size(Qt::TextSingleLine, " ").width() / dpi * PointsPerInch * TwipsPerPoint * 4;
        StringList paragraphs = Printer::createParagraphs(item, Printer::WithAlignment);
        Tags current = Tags::None;
        for (auto& para: paragraphs) {
            rtf += "\\pard\\fi" + QString::number(indent);
            QString justification = para.left(1);
            QString paragraph = para.mid(1);
            if (justification == "L")      rtf += "\\ql";
            else if (justification == "R") rtf += "\\qr ";
            else if (justification == "C") rtf += "\\qc ";
            else if (justification == "J") rtf += "\\qj ";
            List<Word> words = Printer::paragraphWords(paragraph);
            for (auto i = 0; i < words.count(); ++i) {
                // fix image insert here
                Word word = words[i];
                Tags on = word.tags() & ~current;
                Tags off = current & ~word.tags();
                if ((off & Tags::Bold) != Tags::None)      rtf += " \\b0 ";
                if ((off & Tags::Italic) != Tags::None)    rtf += " \\i0 ";
                if ((off & Tags::Underline) != Tags::None) rtf += " \\ul0 ";
                current = word.tags();
                if ((on & Tags::Bold) != Tags::None)      rtf += " \\b ";
                if ((on & Tags::Italic) != Tags::None)    rtf += " \\i ";
                if ((on & Tags::Underline) != Tags::None) rtf += " \\ul ";
                rtf += escapeText(word.str());
                if (!word.isPartial()) rtf += " ";
            }
        }
        rtf += "\\par";
    }
    return rtf + "}";
}

bool RtfExporter::convert() {
    if (mFilename.isEmpty()) return false;

    const auto& defaults = collectMetadataDefaults();

    QString    cover = fetchValue(Cover, defaults, "cover");
    StringList inputMargins { fetchValue(Margins, defaults, "margins").split(",") };
    mMargins = PdfExporter::parseMargins(inputMargins);

    QFileInfo info(mFilename);
    QString path = info.absolutePath();
    Main::ref().setDocDir(path);

    QString rtf = itemsToRtf(cover);

    QFile file(mFilename);
    if (file.open(QIODeviceBase::WriteOnly)) return file.write(rtf.toUtf8()) != -1;
    return false;
}
