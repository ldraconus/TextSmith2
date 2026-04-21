#include "TextExporter.h"

#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>

#include "Main.h"

static constexpr auto Bold      = 1;
static constexpr auto Italic    = 2;
static constexpr auto Underline = 3;

QString TextExporter::outputFooter(int pageNo, int lineWidth, qlonglong id) {
    auto& prefs = Main::ref().prefs();
    auto marginals = Printer::parseMarginal(prefs.footer(), pageNo, mItemIds, id);
    /*
    // xx xx xx
    // xx    xx
    // xx
    //
    // for each left, center, and right columns in the marginal:
    //   columnHeight[which] = maxlines in column
    //   columns[which] = lines in column
    // max = max of columnHeights
    // for line 0 to max:
    //   if line < columnStart[left]: print left marginal line (column[left, columnStart[left]]
    //   if line < columnStart[center]: print center marginal line (column[center, columnStart[center]]
    //   if line < coumnStart[right]: print center marginal line (column[right, columnStart[right]]
    //   increment all columnStarts
    */
    constexpr auto left = 0;
    constexpr auto center = 1;
    constexpr auto right =2;
    int columnHeight[3] { 0, 0, 0 };
    StringList columnLeft;
    StringList columnCenter;
    StringList columnRight;
    for (const auto& marginal: marginals) {
        if (marginal.justify() == Printer::Marginal::Justify::Left) {
            columnHeight[left] = std::max<int>(columnHeight[left], marginal.line());
            columnLeft.append(marginal.text());
        }
        else if (marginal.justify() == Printer::Marginal::Justify::Center) {
            columnHeight[center] = std::max<int>(columnHeight[center], marginal.line());
            columnCenter.append(marginal.text());
        }
        else if (marginal.justify() == Printer::Marginal::Justify::Right) {
            columnHeight[right] = std::max<int>(columnHeight[right], marginal.line());
            columnRight.append(marginal.text());
        }
    }
    int max = std::max<int>({ columnHeight[left], columnHeight[center], columnHeight[right] });
    int columnStart[3];
    QString result;
    for (int ln = 0; ln < max; ++ln) {
        QString line;
        if (ln < columnHeight[left]) { line = columnLeft[ln]; }
        if (ln < columnHeight[center]) {
            QString text = columnCenter[ln];
            int spaces = text.length() / 2 - line.length();
            if (spaces > 0) for (int i = 0; i < spaces; ++i) line += " ";
            line += text;
        }
        if (ln < columnHeight[right]) {
            QString text = columnRight[ln];
            int spaces = lineWidth - text.length() - line.length();
            if (spaces > 0) for (int i = 0; i < spaces; ++i) line += " ";
            line += text;
        }
        line += QChar::LineFeed;
        result += line;
    }
    return result;
}

QString TextExporter::outputHeader(int pageNo, int lineWidth, qlonglong id) {
    auto& prefs = Main::ref().prefs();
    auto marginals = Printer::parseMarginal(prefs.footer(), pageNo, mItemIds, id);
    /*
    // xx xx xx
    // xx    xx
    // xx
    //
    // for each left, center, and right columns in the marginal:
    //   columnHeight[which] = maxlines in column
    //   columns[which] = lines in column
    // max = max of columnHeights
    // for line 0 to max:
    //   if line < columnStart[left]: print left marginal line (column[left, columnStart[left]]
    //   if line < columnStart[center]: print center marginal line (column[center, columnStart[center]]
    //   if line < coumnStart[right]: print center marginal line (column[right, columnStart[right]]
    //   increment all columnStarts
    */
    constexpr auto left = 0;
    constexpr auto center = 1;
    constexpr auto right =2;
    int columnHeight[3] { 0, 0, 0 };
    int columnStart[3];
    QStringList columns[3];
    for (const auto& marginal: marginals) {
        if (marginal.justify() == Printer::Marginal::Justify::Left) {
            columnHeight[left] = std::max<int>(columnHeight[left], marginal.line());
            columns[left].append(marginal.text());
        }
        else if (marginal.justify() == Printer::Marginal::Justify::Center) {
            columnHeight[center] = std::max<int>(columnHeight[center], marginal.line());
            columns[center].append(marginal.text());
        }
        else if (marginal.justify() == Printer::Marginal::Justify::Right) {
            columnHeight[right] = std::max<int>(columnHeight[right], marginal.line());
            columns[right].append(marginal.text());
        }
    }
    int max = std::max<int>({ columnHeight[left], columnHeight[center], columnHeight[right] });
    columnStart[left] = columnHeight[left] - max;
    columnStart[center] = columnHeight[center] - max;
    columnStart[right] = columnHeight[right] - max;
    QString result;
    for (int ln = 0; ln < max; ++ln) {
        QString line;
        if (columnStart[left] >= 0) { line = columns[left][columnStart[left]]; }
        if (columnStart[center] >= 0) {
            QString text = columns[center][columnStart[center]];
            int spaces = text.length() / 2 - line.length();
            if (spaces > 0) for (int i = 0; i < spaces; ++i) line += " ";
            line += text;
        }
        if (columnStart[right] >= 0) {
            QString text = columns[right][columnStart[right]];
            int spaces = lineWidth - text.length() - line.length();
            if (spaces > 0) for (int i = 0; i < spaces; ++i) line += " ";
            line += text;
        }
        line += QChar::LineFeed;
        result += line;
    }
    return result;
}

bool TextExporter::convert() {
    static constexpr auto PageHeight = 11.0;
    static constexpr auto PointSize  = 72;
    static constexpr auto PageWidth  = 8.5;

    if (mFilename.isEmpty()) return false;

    const auto& defaults = collectMetadataDefaults();

    QFileInfo info(mFilename);
    QString path = info.absolutePath();
    QString base = info.baseName();
    QString ext =  "." + (info.completeSuffix().isEmpty() ? Extension() : info.completeSuffix());

    auto pointMargins = Main::ref().prefs().margins(Preferences::Units::Points);
    QSize pageSize(PageWidth * PointSize, PageHeight * PointSize);
    int pointWidth = pageSize.width() - pointMargins[Preferences::Left] - pointMargins[Preferences::Right];
    int pointHeight = pageSize.height() - pointMargins[Preferences::Top] - pointMargins[Preferences::Bottom];

    QFont courier("Courier", 10);
    QFontMetrics metrics(courier);

    QChar space = QChar::Space;
    auto spaceWidth = metrics.horizontalAdvance(space);
    int fontHeight = 12;

    int linesPerPage = pointHeight / fontHeight;
    int lineWidth = pointWidth;

    QString dir = path.isEmpty() ? Main::ref().docDir() : path;
    Main::ref().setDocDir(dir);

    auto& prefs = Main::ref().prefs();
    StringList chapterTags { prefs.chapterTag().split(Preferences::sep) };
    StringList coverTags   { prefs.coverTag().split(Preferences::sep) };
    StringList sceneTags   { prefs.sceneTag().split(Preferences::sep) };
    bool useSep = prefs.useSeparator();
    QString sep = prefs.separator();
    bool firstScene = true;
    StringList bold = fetchValue(Bold, defaults, prefs.bold()).split(",");
    if (bold.count() == 0) bold.append("*");
    if (bold[0].isEmpty()) bold[0] = "*";
    if (bold.count() == 1) bold.append(bold[0]);
    if (bold[1].isEmpty()) bold[1] = bold[0];
    StringList italic = fetchValue(Italic, defaults, prefs.italic()).split(",");
    if (italic.count() == 0) italic.append("/");
    if (italic[0].isEmpty()) italic[0] = "/";
    if (italic.count() == 1) italic.append(italic[0]);
    if (italic[1].isEmpty()) italic[1] = italic[0];
    StringList underline = fetchValue(Underline, defaults, prefs.underline()).split(",");
    if (underline.count() == 0) underline.append("_");
    if (underline[0].isEmpty()) underline[0] = "_";
    if (underline.count() == 1) underline.append(underline[0]);
    if (underline[1].isEmpty()) underline[1] = underline[0];

    QString document;
    int lineNo = 0;
    bool firstLineEver = true;
    int pageNo = 1;
    for (auto&& id: mItemIds) {
        Item& item = Main::ref().novel().findItem(id);
        if (item.hasTag(coverTags)) continue;
        if (item.hasTag(chapterTags)) {
            if (firstLineEver) firstLineEver = false;
            else {
                for (; lineNo < linesPerPage; ++lineNo) document += QChar::LineFeed;
                outputFooter(pageNo, lineWidth, id);
                document += QChar::FormFeed;
                lineNo = 0;
                outputHeader(++pageNo, lineWidth, id);
            }
            firstScene = true;
        }
        if (item.hasTag(sceneTags)) {
            if (firstScene) firstScene = false;
            else if (useSep) {
                int leading = (lineWidth - metrics.horizontalAdvance(sep)) / (2 * spaceWidth);
                document += QChar::LineFeed;
                lineNo++;
                for (int i = 0; i < leading; ++i) document += space;
                document += sep + QChar::LineFeed + QChar::LineFeed;
                lineNo += 2;
            }
        }

        auto* doc = item.doc();
        for (auto it = doc->begin(); it != doc->end(); it = it.next()) {
            StringList lines;
            QTextBlock block = it;
            auto blockFmt = block.blockFormat();
            QString text;
            for (auto i = 0; i < 4; ++i) text += space;
            int indent = blockFmt.indent();
            for (auto i = 0; i < indent * 4; ++i) text += space;
            int width = metrics.horizontalAdvance(text);
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
                    int wordLen = metrics.horizontalAdvance(word);
                    if (wordLen + (firstWord ? 0 : spaceWidth) + width >= lineWidth) {
                        if (lineNo == linesPerPage) {
                            text += outputFooter(pageNo, lineWidth, id);
                            text += QChar::FormFeed;
                            lineNo = 0;
                            text += outputHeader(++pageNo, lineWidth, id);
                        }
                        lines.append(text);
                        text.clear();
                        for (auto i = 0; i < indent * 4; ++i) text += space;
                        text += word;
                        width = metrics.horizontalAdvance(text);
                    } else {
                        if (firstWord) firstWord = false;
                        else {
                            text += space;
                            width += spaceWidth;
                        }
                        text += word;
                        width += wordLen;
                    }
                }
                if (format.fontUnderline())              text += underline[1];
                if (format.fontItalic())                 text += italic[1];
                if (format.fontWeight() > QFont::Normal) text += bold[1];
            }
            if (!text.isEmpty()) lines.append(text);

            for (auto line: lines) {
                int spaces = 0;
                switch (blockFmt.alignment()) {
                case Qt::AlignVCenter:
                case Qt::AlignCenter:
                case Qt::AlignHCenter: spaces = (lineWidth - metrics.horizontalAdvance(line)) / (2 * spaceWidth); break;
                case Qt::AlignRight:   spaces = (lineWidth - metrics.horizontalAdvance(line)) / spaceWidth;       break;
                case Qt::AlignJustify:
                case Qt::AlignAbsolute:
                    spaces = (lineWidth - metrics.horizontalAdvance(line)) / spaceWidth;
                    {
                        StringList words { line.split(space) };
                        if (words.count() > 1) {
                            int eachWord = spaces / (words.count() - 1);
                            int extra = spaces % (words.count() - 1);
                            for (int i = 1; i < eachWord; ++i) words[i] = " " + words[i];
                            QList<int> wordsAvailable;
                            for (auto i = 1; i < words.count(); ++i) wordsAvailable.append(i);
                            while (extra) {
                                int pick = QRandomGenerator::global()->bounded(extra);
                                int which = wordsAvailable[pick];
                                words[which] = " " + words[which];
                                wordsAvailable.remove(pick);
                                --extra;
                            }
                        }
                        line = words.join(" ");
                    }
                    spaces = 0;
                    break;
                case Qt::AlignBaseline:
                case Qt::AlignTop:
                case Qt::AlignBottom:
                case Qt::AlignHorizontal_Mask:
                case Qt::AlignLeft:                                                                               break;
                }
                for (int i = 0; i < spaces; ++i) document += space;
                document += line + QChar::LineFeed;
            }
        }
    }
    for (; lineNo != linesPerPage; ++lineNo) document += QChar::LineFeed;
    document += outputFooter(pageNo, lineWidth, mItemIds.last());

    QFile file(dir + "/" + base + ext);
    if (file.open(QIODeviceBase::WriteOnly)) return file.write(document.toUtf8()) != -1;
    return false;
}
