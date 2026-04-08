#include "TextExporter.h"

#include <QDir>
#include <QFileInfo>

#include "Main.h"

bool TextExporter::convert() {
    if (mFilename.isEmpty()) return false;

    const auto& defaults = collectMetadataDefaults();
    QFileInfo info(mFilename);
    QString path = info.absolutePath();
    QString base = info.baseName();
    QString ext =  "." + (info.completeSuffix().isEmpty() ? Extension() : info.completeSuffix());

    auto pointMargins = Main::ref().prefs().margins(Preferences::Units::Points);
    QSize pageSize(8.5 * 72, 11 * 72);
    int pointWidth = pageSize.width() - pointMargins[Preferences::Left] - pointMargins[Preferences::Right];
    int pointHeight = pageSize.height() - pointMargins[Preferences::Top] - pointMargins[Preferences::Bottom];

    QFont courier("Courier", 12);
    QFontMetrics metrics(courier);

    auto charWidth = metrics.averageCharWidth();
    qreal dpiX = 1;
    int fontHeight = 12;
    int fontWidth = charWidth / dpiX;

    int linesPerPage = pointHeight / fontHeight;
    int charactersPerLine = pointWidth / fontWidth;

    QString dir = path.isEmpty() ? Main::ref().docDir() : path;
    Main::ref().setDocDir(dir);

    QString document;
    int line = 0;
    bool firstLineEver = true;
    auto& prefs = Main::ref().prefs();
    StringList chapterTags { prefs.chapterTag().split(Preferences::sep) };
    StringList coverTags   { prefs.coverTag().split(Preferences::sep) };
    StringList sceneTags   { prefs.sceneTag().split(Preferences::sep) };
    bool useSep = prefs.useSeparator();
    QString sep = prefs.separator();
    bool firstScene = true;
    StringList bold      { "*", "*" };
    StringList italic    { "/", "/" };
    StringList underline { "_", "_" };
    for (auto&& id: mItemIds) {
        Item& item = Main::ref().novel().findItem(id);
        if (item.hasTag(coverTags)) continue;
        if (item.hasTag(chapterTags)) {
            if (firstLineEver) firstLineEver = false;
            else {
                document += QChar(12);
                line = 0;
            }
            firstScene = true;
        }
        if (item.hasTag(sceneTags)) {
            if (firstScene) firstScene = false;
            if (useSep) {
                int leading = (charactersPerLine - sep.length()) / 2;
                document += "\n";
                for (int i = 0; i < leading; ++i) document += " ";
                document += sep + "\n";
            }
        }

        auto* doc = item.doc();
        for (auto it = doc->begin(); it != doc->end(); it = it.next()) {
            StringList lines;
            QTextBlock block = it;
            auto blockFmt = block.blockFormat();
            int width = 4;
            QString text = "    ";
            for (auto it2 = block.begin(); it2 != block.end(); it2++) {
                bool firstWord = true;
                QTextFragment frag = it2.fragment();
                if (!frag.isValid()) continue;

                QTextCharFormat fmt = frag.charFormat();
                if (fmt.isImageFormat()) continue;

                QTextCharFormat format = frag.charFormat();
                if (format.fontWeight() > QFont::Normal) text += bold[0];
                if (format.fontItalic())                 text += italic[0];
                if (format.fontUnderline())              text += underline[0];

                StringList words { frag.text().split(" ") };
                for (const auto& word: words) {
                    if (word.length() + 1 + width >= charactersPerLine) {
                        if (line == linesPerPage) {
                            text += QChar(12);
                            line = 0;
                        }
                        lines.append(text);
                        text = word;
                        width = word.length();
                    } else {
                        if (firstWord) firstWord = false;
                        else text += " ";
                        text += word;
                        width += word.length();
                    }
                }
                if (format.fontUnderline())              text += underline[1];
                if (format.fontItalic())                 text += italic[1];
                if (format.fontWeight() > QFont::Normal) text += bold[1];
            }
            if (!text.isEmpty()) lines.append(text);

            for (const auto& line: lines) {
                int spaces = 0;
                switch (blockFmt.alignment()) {
                case Qt::AlignHCenter: spaces = (charactersPerLine - width) / 2; break;
                case Qt::AlignRight:   spaces = charactersPerLine - width;       break;
                case Qt::AlignJustify:
                default:                                                         break;
                }
                for (int i = 0; i < spaces; ++i) document += " ";
                document += line + "\n";
            }
        }
    }

    QFile file(dir + "/" + base + ext);
    if (file.open(QIODeviceBase::WriteOnly)) return file.write(document.toUtf8()) != -1;
    return false;
}
