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
    Main::ref().setDocDir(path);

    auto pointMargins = Main::ref().prefs().margins(Preferences::Units::Points);
    QSize pageSize(8.5 * 72, 11 * 72);
    int pointWidth = pageSize.width() - pointMargins[Preferences::Left] - pointMargins[Preferences::Right];
    int pointHeight = pageSize.height() - pointMargins[Preferences::Top] - pointMargins[Preferences::Bottom];

    QFont courier("Courier", 12);
    QFontMetrics metrics(courier);

    auto charWidth = metrics.averageCharWidth();
    QScreen* widgetScreen = Main::ptr()->screen();
    qreal dpiX = 1;
    if (widgetScreen)  dpiX = widgetScreen->logicalDotsPerInchX();
    int fontHeight = 12;
    int fontWidth = charWidth / dpiX;

    int linesPerPage = pointHeight / fontHeight;
    int charactersPerLine = pointWidth / fontWidth;

    QString dir = path;

    QString text;
    int line = 0;
    bool firstLineEver = true;
    StringList chapterTags { Main::ref().prefs().chapterTag().split(Preferences::sep) };
    StringList coverTags { Main::ref().prefs().coverTag().split(Preferences::sep) };
    StringList sceneTags { Main::ref().prefs().sceneTag().split(Preferences::sep) };
    for (auto&& id: mItemIds) {
        Item& item = Main::ref().novel().findItem(id);
        StringList paragraphs { item.doc()->toPlainText().split("\n") };
        if (item.hasTag(coverTags)) continue;
        if (item.hasTag(Main::ref().prefs().chapterTag())) {
            if (firstLineEver) firstLineEver = false;
            else {
                text += QChar(12);
                line = 0;
            }
        }
        for (const auto& paragraph: paragraphs) {
            text += "    ";
            int width = 4;
            StringList words { paragraph.split(" ") };
            for (const auto& word: words) {
                if (word.length() + 1 + width >= charactersPerLine) {
                    text += "\n";
                    ++line;
                    if (line == linesPerPage) {
                        text += QChar(12);
                        line = 0;
                    }
                    text += word;
                    width = word.length();
                } else {
                    text += " " + word;
                    width += word.length();
                }
            }
        }
    }

    QFile file(dir + "/" + base + ext);
    if (file.open(QIODeviceBase::WriteOnly)) return file.write(text.toUtf8()) != -1;
    return false;
}
