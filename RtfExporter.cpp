#include "RtfExporter.h"
#include "PdfExporter.h"
#include "Main.h"
#include "ui_Main.h"

using namespace Words;

#include <QFileInfo>
#include <QTextBlock>

QString RtfExporter::handleImage(const QString& cover) {
    QString rtf;

    QByteArray imgData;
    Main::ref().loadImageBytesFromUrl(cover, [&](QByteArray data){ imgData = data; });
    QImage img = QImage::fromData(imgData);
    if (img.isNull()) return rtf;
    QString hexData = imgData.toHex();

    int twipWidth = img.width() * 15;
    int twipHeight = img.height() * 15;

    rtf += "  {\\pict";
    rtf += "\\pngblip";
    rtf += "\\picw" + QString::number(img.width());   // Source width (pixels)
    rtf += "\\pich" + QString::number(img.height());  // Source height (pixels)
    rtf += "\\picwgoal" + QString::number(twipWidth); // Target display width (twips)
    rtf += "\\pichgoal" + QString::number(twipHeight);// Target display height (twips)

    rtf += + "    \n"; // <--- CRITICAL SPACE separating commands from data

    rtf += hexData;

    rtf += "  }\n";
    return rtf;
}

QString RtfExporter::itemsToRtf(const QString& cover) {
    QString rtf = "{\\rtf1\\ansi\\deff0\n"
                  "  {\\fonttbl{\\f0 " + Main::ref().ui()->textEdit->font().family() + ";}}\n";
    mMargins *= PointsPerInch * TwipsPerPoint;
    rtf += QString("  \\margl%1\\margr%2\\margt%3\\margb%4\n")
               .arg(mMargins.left())
               .arg(mMargins.right())
               .arg(mMargins.top())
               .arg(mMargins.bottom());
    bool first = true;
    int dpi = Main::ref().ui()->textEdit->viewport()->logicalDpiX();
    QFontMetrics metrics(Main::ref().ui()->textEdit->font());
    auto indent = metrics.size(Qt::TextSingleLine, " ").width() / dpi * PointsPerInch * TwipsPerPoint * 4;

    if (!cover.isEmpty()) handleImage(cover);

    for (auto& id: mItemIds) {
        Item& item = mNovel.findItem(id);
        if (!item.hasTag(mChapterTag) &&
            !item.hasTag(mSceneTag) &&
            !item.hasTag(mCoverTag)) continue;
        if (item.hasTag(mCoverTag) || item.hasTag(mChapterTag)) {
            if (!first) rtf += "  \\page\n";
        }
        first = false;

        QTextDocument doc;
        doc.setHtml(item.html());

        for (auto it = doc.begin(); it != doc.end(); it = it.next()) {
            QTextBlock block = it;
            auto blockFmt = block.blockFormat();
            rtf += "  \\pard\\fi" + QString::number(indent);
            switch (blockFmt.alignment()) {
            case Qt::AlignCenter:  rtf += "\\qc "; break;
            case Qt::AlignRight:   rtf += "\\qr "; break;
            case Qt::AlignJustify: rtf += "\\qj "; break;
            default:               rtf += "\\ql "; break;
            }

            for (auto it2 = block.begin(); it2 != block.end(); it2++) {
                QTextFragment frag = it2.fragment();
                if (!frag.isValid()) continue;

                QTextCharFormat fmt = frag.charFormat();
                if (fmt.isImageFormat()) rtf += handleImage(fmt.toImageFormat().name());
                else                     rtf += processText(frag);
            }
            rtf += "\n  \\par\n";
        }
    }
    return rtf + "}";
}

QString RtfExporter::escapeRtfText(const QString &text) {
    QString rtf;
    // Reserve space to avoid frequent reallocations (approx 10% overhead est.)
    rtf.reserve(text.size() * 1.1);

    for (QChar c : text) {
        ushort val = c.unicode();

        if (val == '\\') rtf.append("\\\\");
        else if (val == '{') rtf.append("\\{");
        else if (val == '}') rtf.append("\\}");
        else if (val == '\t') rtf.append("\\tab ");
        else if (val == '\n' || val == '\r') rtf.append("\\line "); // Soft line break
        else if (val < 128) rtf.append(c);
        else {
            short signedVal = static_cast<short>(val);
            rtf.append(QString("\\u%1?").arg(signedVal));
        }
    }
    return rtf;
}

QString RtfExporter::processText(const QTextFragment& frag) {
    QString rtf;
    QTextCharFormat format = frag.charFormat();
    if (format.fontWeight() > QFont::Normal) rtf += "\\b";
    if (format.fontItalic())                 rtf += "\\i";
    if (format.fontUnderline())              rtf += "\\u";
    bool addSpace = !rtf.isEmpty();

    rtf += (addSpace ? " " : "") + escapeRtfText(frag.text());

    if (format.fontWeight() > QFont::Normal) rtf += "\\b0";
    if (format.fontItalic())                 rtf += "\\i0";
    if (format.fontUnderline())              rtf += "\\ulnone";

    return rtf + (addSpace ? " " : "");
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
