#include "Main.h"
#include "Printer.h"

#include <QTextBlock>
#include <QTextDocumentFragment>

void Printer::printNovel() {
    setFullPage(true);
    QSizeF size = pageSize(QPrinter::Point);
    QPainter paint = painter();
    outputNovel(mIds, mPrefs->chapterTag(), mPrefs->sceneTag(), mPrefs->coverTag(), paint, size,
                [this, &paint]() {
                    qreal xFactor = physicalDpiX() / PointsPerInch;
                    qreal yFactor = physicalDpiX() / PointsPerInch;
                    header(paint, xFactor, yFactor);
                    footer(paint, xFactor, yFactor);
                    newPage();
                });
}

void Printer::printParagraphs(QPainter &painter,
                              QSizeF &pageSize,
                              std::function<void()> &pager,
                              QMarginsF &margins,
                              QFont &font,
                              qreal &xFactor,
                              qreal &yFactor,
                              qreal &at,
                              bool &startingPage,
                              StringList &paragraphs) {
    QFontMetricsF metrics(font, paintdevice());
    int lineHeight = metrics.height() / yFactor;
    int baseline = metrics.ascent() / yFactor;
    int space = metrics.horizontalAdvance("N") / xFactor;
    Words::Tags current = Words::Tags::None;
    for (const auto& paragraph: paragraphs) {
        int indent = 4 * metrics.horizontalAdvance("M") / xFactor;
        QString align = paragraph.left(1);
        QList<Words::Word> words;
        double extra = 0.0;

        if (!align.isEmpty()) {
            if (align[0].isDigit()) {
                int i = 0;
                for (; i < paragraph.length(); ++i) {
                    if (!paragraph[i].isDigit()) break;
                }
                QString num = paragraph.left(i);
                extra = num.toInt() * indent;
                align = "I";
                words = paragraphWords(paragraph.mid(i + 1));
            } else words = paragraphWords(paragraph.mid(1));
        } else align = "L";

        bool first = true;
        if (words.isEmpty()) at += lineHeight;
        while (!words.isEmpty()) {
            auto width = extra;
            if (first) width += indent;
            List<Words::Word> line;
            while (!words.isEmpty()) {
                Words::Word word = words.first();
                if (line.size() != 0 && (current & Words::Tags::Partial) != Words::Tags::None) width += space;
                auto withWord = metrics.horizontalAdvance(word.str()) / xFactor + width;
                if (withWord < pageSize.width() / xFactor) {
                    auto word = words.takeFirst();
                    line.append(word);
                    width = withWord;
                    if (!word.isPartial()) width += space;
                } else break;
            }

            if (at + lineHeight > pageSize.height() - margins.bottom()) {
                newPage(painter, pager, xFactor, yFactor);
                ++mPageNo;
                at = margins.top() + baseline;
                startingPage = true;
            }

            qreal fill = 0.0;
            qreal x = margins.left() + indent + extra;
            if (!line.empty() && align == "J") {
                if (line.count() > 1 && !words.isEmpty()) fill = width / (line.count() - 1);
            } else {
                if (align == "R") x = pageSize.width() / xFactor - margins.right() - width;
                else if (align == "C") x = pageSize.width() / 2.0 - width / 2.0;
            }

            printLine(painter, line, font, current, x, at, fill, xFactor, yFactor);

            startingPage = false;
            at += lineHeight;
            indent = 0;
        }
    }
}

QList<Words::Word> Printer::paragraphWords(const QString &paragraph) {
    QList<Words::Word> words;
    StringList wordList { paragraph.split(" ", Qt::SkipEmptyParts) };
    for (const auto& item: wordList) {
        StringList parts { item.split("<", Qt::SkipEmptyParts) };
        // now parts has (for ex. b>text, /b>)
        Words::Word word;
        for (auto [i, part]: enumerate(parts)) {
            QString fragment = part;
            if (i != 0) word.setTags(words[i - 1].tags());
            bool found = true;
            while(found) {
                found = true;
                if (fragment.startsWith("b>")) {
                    word += Words::Tags::Bold;
                    fragment = fragment.mid(2);
                    continue;
                } else if (fragment.startsWith("i>")) {
                    word += Words::Tags::Italic;
                    fragment = fragment.mid(2);
                    continue;
                } else if (fragment.startsWith("u>")) {
                    word += Words::Tags::Underline;
                    fragment = fragment.mid(2);
                    continue;
                } else if (fragment.startsWith("/b>")) {
                    word -= Words::Tags::Bold;
                    fragment = fragment.mid(3);
                    continue;
                } else if (fragment.startsWith("/i>")) {
                    word -= Words::Tags::Italic;
                    fragment = fragment.mid(3);
                    continue;
                } else if (fragment.startsWith("/u>")) {
                    word -= Words::Tags::Underline;
                    fragment = fragment.mid(3);
                    continue;
                }
                found = false;
            }
            word.setStr(fragment);
            words.append(word);
            word.clear();
            if (i != 0) words[i - 1] += Words::Tags::Partial;
        }
    }
    return words;
}

void Printer::footer(QPainter &painter, const qreal &xFactor, const qreal &yFactor) {
    auto marginals = parseMarginal(mPrefs->footer());
    qreal bottom = mPrefs->margins()[Preferences::Bottom] * PointsPerInch * yFactor;
    qreal y = mPrinter->pageRect(QPrinter::Point).height() * yFactor - bottom;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    QFontMetricsF metrics(font, paintdevice());
    y -= metrics.descent();
    y += metrics.lineSpacing();
    for (const auto& marginal: marginals) {
        double line = y + marginal.line() * metrics.lineSpacing();
        printMarginal(painter, line, marginal, xFactor, yFactor);
    }
}

QString Printer::fromHtml(const QString &html) {
    static std::unordered_map<QString, QString> conversions {
        { "&quot;",    { "\"" } }, { "&apos;",    { "'" } }, { "&nbsp;",    { " " } }, { "&emsp;",    { " " } }, { "&ensp;",    { " " } },
        { "&thinsp;",  { " " } },  { "&ndash;",   { "-" } }, { "&mdash;",   { "-" } }, { "&lsquo;",   { "‘" } }, { "&rsquo;",   { "’" } },
        { "&ldquo;",   { "“" } },  { "&rdquo;",   { "”" } }, { "&hellip;",  { "…" } }, { "&copy;",    { "©" } }, { "&reg;",     { "®" } },
        { "&trade;",   { "™" } },  { "&euro;",    { "€" } }, { "&pound;",   { "£" } }, { "&yen;",     { "¥" } }, { "&rarr;",    { "→" } },
        { "&larr;",    { "←" } },  { "&uarr;",    { "↑" } }, { "&darr;",    { "↓" } }
    };

    QString work = html;
    for (const auto& pair: conversions) work = work.replace(pair.first, pair.second, Qt::CaseInsensitive);
    return work;
}

void Printer::handleCover(Item &item,
                          bool startingPage,
                          QSizeF &pageSize,
                          QMarginsF &margins,
                          QPainter &painter,
                          qreal xFactor,
                          qreal yFactor,
                          std::function<void()> printer) {
    QPointF at = QPointF(margins.top(), margins.left());
    if (mPageNo != 1) {
        if (!startingPage) {
            newPage(painter, printer, xFactor, yFactor, true);
            ++mPageNo;
            if (mPageNo % 2 != 0) {
                newPage(painter, printer, xFactor, yFactor, true);
                ++mPageNo;
            }
        }
    }
    QString html = item.html();
    StringList imgParts { html.split("<img src=\"") };
    for (auto i = 1; i < imgParts.count(); ++i) {
        StringList name { imgParts[i].split("\"")};
        if (name.count() >= 2) {
            QUrl url(name[0]);
            const QImage& image = mImages[url];
            QImage scaledImage = image;
            auto width = (pageSize.width() - margins.right() - margins.left()) * xFactor;
            auto height = (pageSize.height() - margins.top() - margins.bottom()) * yFactor;
            if (image.width() > width) scaledImage = image.scaledToWidth(width, Qt::SmoothTransformation);
            if (scaledImage.height() > height) scaledImage = image.scaledToHeight(height, Qt::SmoothTransformation);
            if (at.y() * yFactor + scaledImage.height() > (pageSize.height() - margins.bottom()) * yFactor) {
                newPage(painter, printer, xFactor, yFactor);
                ++mPageNo;
                at = QPointF(margins.left(), margins.top());
            }
            auto size = scaledImage.size();
            QRectF imageRect(at.x() * xFactor, at.y() * yFactor, size.width(), size.height());
            painter.drawImage(imageRect, scaledImage);
            at.setY(at.y() + scaledImage.height());
        }
    }
}

void Printer::header(QPainter &painter, const qreal &xFactor, const qreal &yFactor) {
    auto marginals = parseMarginal(mPrefs->header());
    if (marginals.size() < 1) return;

    qreal y = mPrefs->margins()[Preferences::Top] * PointsPerInch * yFactor;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    QFontMetricsF metrics(font, paintdevice());
    y += metrics.descent();
    y -= metrics.lineSpacing();
    y -= marginals.last().line() * metrics.lineSpacing();
    for (const auto& marginal: marginals) {
        double line = y + marginal.line() * metrics.lineSpacing();
        printMarginal(painter, line, marginal, xFactor, yFactor);
    }
}

StringList Printer::mergeWordFragments(QList<Words::Word> &words) {
    StringList wordList;
    QString w;
    for (const auto& word: words) {
        w += word.str();
        if (!word.isPartial()) {
            wordList += w;
            w.clear();
        }
    }
    return wordList;
}

void Printer::newPage(QPainter &painter,
                      std::function<void()> printer,
                      const qreal &xFactor,
                      const qreal &yFactor,
                      bool marginals) {
    if (marginals) {
        header(painter, xFactor, yFactor);
        footer(painter, xFactor, yFactor);
    }
    printer();
}

void Printer::outputNovel(List<qlonglong> ids,
                          const QString &chapterTag,
                          const QString &sceneTag,
                          const QString &coverTag,
                          QPainter &painter,
                          QSizeF pageSize,
                          std::function<void()> pager) {
    if (mPrinter == nullptr) return;
    QMarginsF margins(mPrefs->margins()[Preferences::Left] * PointsPerInch,
                      mPrefs->margins()[Preferences::Top] * PointsPerInch,
                      mPrefs->margins()[Preferences::Right] * PointsPerInch,
                      mPrefs->margins()[Preferences::Bottom] * PointsPerInch);
    qreal xFactor = physicalDpiX() / PointsPerInch;
    qreal yFactor = physicalDpiX() / PointsPerInch;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    font.setPixelSize(yFactor * font.pixelSize());
    QFontMetricsF metrics(font, paintdevice());
    int baseline = metrics.ascent() / yFactor;
    qreal at = margins.top() + baseline;
    painter.setFont(font);

    mPageNo = 1;
    bool startingPage = true;
    mIds = ids;
    for (const auto& id : ids) {
        mId = id;
        Item& item = Main::ref().novel().findItem(id);
        if (!item.hasTag(chapterTag) && !item.hasTag(sceneTag) && !item.hasTag(coverTag)) continue;
        if (item.hasTag(coverTag)) {
            handleCover(item, startingPage, pageSize, margins, painter, xFactor, yFactor, pager);
            newPage(painter, pager, xFactor, yFactor, false);
            at = margins.top() + baseline;
            startingPage = true;
            continue;
        } else if (item.hasTag(chapterTag)) {
            if (!startingPage && mPageNo != 1) {
                newPage(painter, pager, xFactor, yFactor);
                ++mPageNo;
                at = margins.top() + baseline;
                startingPage = true;
            }
        }

        StringList paragraphs = createParagraphs(item, WithAlignment);
        printParagraphs(painter, pageSize, pager, margins, font, xFactor, yFactor, at, startingPage, paragraphs);
    }

    if (!startingPage) newPage(painter, pager, xFactor, yFactor);
}

void Printer::printLine(QPainter &painter, List<Words::Word> &line, QFont &font, Words::Tags &current, qreal x, qreal at, qreal fill, qreal &xFactor,
                        qreal &yFactor) {
    QFontMetricsF metrics(painter.fontMetrics());
    at *= yFactor;
    fill *= xFactor;
    x *= xFactor;
    int space = metrics.horizontalAdvance("N");

    bool first = false;
    while (!line.empty()) {
        Words::Word word = line.takeFirst();
        current = (current & word.tags()) | word.tags();
        QFont wFont = font;
        if ((current & Words::Tags::Bold) != Words::Tags::None) wFont.setWeight(QFont::Bold);
        if ((current & Words::Tags::Italic) != Words::Tags::None) wFont.setItalic(true);
        if ((current & Words::Tags::Underline) != Words::Tags::None) wFont.setUnderline(true);
        painter.setFont(wFont);
        painter.drawText(QPoint(x + 0.5, at + 0.5), word.str());
        x += fill + metrics.horizontalAdvance(word.str());
        if (!word.isPartial()) x += space;
    }
}

List<Printer::Marginal> Printer::parseMarginal(const QString &marginal) {
    List<Marginal> marginals;
    StringList htmls { marginal.split(Preferences::sep, Qt::KeepEmptyParts) };
    if (htmls.size() != 3) return marginals;
    QTextEdit edit;
    StringList lcr;
    for (const auto& html: htmls) {
        edit.setHtml(html);
        lcr.append(edit.toPlainText());
    }
    for (auto [i, str]: enumerate(lcr)) {
        StringList lines { str.split("\n", Qt::KeepEmptyParts) };
        Marginal::Justify justify = Marginal::Justify::Left;
        switch (i) {
        case 1: justify = Marginal::Justify::Center; break;
        case 2: justify = Marginal::Justify::Right;  break;
        }
        for (auto [lineNum, line]: enumerate(lines)) marginals.append({ justify, lineNum, line });
    }

    // handle: \\ -      \
    //         \# -      page number
    //         \(tag) - look up tag for this id:value, output value
    //         every other \, remove.
    Item& item = Main::ref().novel().findItem(mId);
    for (auto& margin: marginals) {
        QString text = margin.text();
        text.replace("\\\\", QChar(0x001e));
        text.replace("\\#", QString::number(mPageNo));
        StringList tags { text.split("\\(") };
        text = "";
        bool first = true;
        for (const auto& tag: tags) {
            if (first) text = tag;
            else {
                StringList parts { tag.split(")") };
                QString value = "";
                if (parts.size() != 1) text += resolveTag(parts[0]) + parts[1];
            }
            first = false;
        }

        text.replace(QChar(0x001e), "\\");
        margin.setText(text);
    }

    return marginals;
}

void Printer::printMarginal(QPainter &painter, qlonglong y, const Marginal &object, const qreal &xFactor, const qreal &yFactor) {
    qreal left = mPrefs->margins()[Preferences::Left] * PointsPerInch * xFactor;
    qreal right = (mPrinter->pageRect(QPrinter::Point).width() -  mPrefs->margins()[Preferences::Right] * PointsPerInch) * xFactor;
    qreal center = mPrinter->pageRect(QPrinter::Point).width() * xFactor / 2;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    QFontMetricsF metrics(painter.fontMetrics());
    QString text = object.text();
    auto width = metrics.horizontalAdvance(text);
    auto x = left;

    switch (object.justify()) {
    case Marginal::Justify::Center: x = center - width / 2; break;
    case Marginal::Justify::Left:                           break;
    case Marginal::Justify::Right:  x = right - width;      break;
    }

    painter.drawText(QPoint({ int(x + 0.5), int(y + 0.5) }), text);
}

QString Printer::resolveTag(const QString &key) {
    QString value;
    for (const auto& id: mIds) {
        const Item& item = Main::ref().novel().findItem(id);
        const auto& tags = item.tags();
        for (const auto& tag: tags) {
            if (tag.startsWith(key + ":", Qt::CaseInsensitive)) return tag.mid(key.length() + 1);
        }
        if (mId == id) break;
    }
    return value;
}

QString Printer::createParagraph(const QString &html) {
    StringList bodyless { html.split("<body>") };
    if (bodyless.count() != 2) return "";
    bodyless = { bodyless[1].split("</body>") };
    if (bodyless.count() != 2) return "";
    bodyless = { bodyless[0].split("<p")} ;

    QString block;
    for (auto [i, para]: enumerate(bodyless)) {
        if (i == 0) continue;
        StringList end { para.split("</p>") };
        if (end.size() != 2) continue;

        StringList spanned { end[0].split("<span") };
        if (spanned.count() != 2) {
            spanned = { spanned[0].split("<br/>") };
            if (spanned.count() != 2) continue;
            block += "\n";
            continue;
        }

        spanned = { spanned[1].split(";\">") };
        if (spanned.count() != 2) continue;
        spanned = { spanned[1].split("</span>") };
        if (spanned.count() != 2) continue;
        block += spanned[0];
    }

    return fromHtml(block);
}

StringList Printer::createParagraphs(Item& item, bool align) {
    StringList paragraphs;
    QTextDocument doc;
    QTextDocument empty;
    doc.setHtml(item.html());
    for (QTextBlock block = doc.begin(); block != doc.end(); block = block.next()) {
        QTextCursor cursor(block);
        QString alignment;
        cursor.select(QTextCursor::BlockUnderCursor);
        QString html = cursor.selection().toHtml();
        if (!html.isEmpty()) {
            StringList paras { html.split("<p ") };
            for (const auto& para: paras) {
                if (para.contains("<body>"))                                   continue;
                if (align) {
                    if (para.startsWith("align=\"left"))                  alignment = "L";
                    else if (para.startsWith("align=\"right"))                 alignment = "R";
                    else if (para.startsWith("align=\"center"))                alignment = "C";
                    else if (para.startsWith("align=\"justify"))               alignment = "F";
                    else if (para.startsWith("style=\"-qt-paragraph-type:empty;"))     continue;
                    else if (para.indexOf(" -qt-block-indent:0; ") == -1) {
                        StringList indented { para.split("-qt-block-indent:") };
                        if (indented.size() < 2 || !indented[1].contains(">")) alignment = "L";
                        else {
                            StringList parts { indented[1].split(";") };
                            if (parts.size() < 2) alignment = "L";
                            else {
                                auto indent = parts[0].toInt();
                                alignment = QString::number(indent) + "I";
                            }
                        }
                    } else                                                     alignment = "L";
                }
                paragraphs.append(alignment + createParagraph(html));
            }
        } else paragraphs.append("");
    }

    return paragraphs;
}

const QSizeF Printer::pageSize(QPrinter::Unit unit) const
{
    const QRectF rect = paperRect(unit);

    Preferences* prefs = mPrefs ? mPrefs : &Main::ref().prefs();
    auto margins = prefs->margins();
    return QSizeF(rect.right() - (margins[Preferences::Left] + margins[Preferences::Right]),
                  rect.bottom() - (margins[Preferences::Top] - margins[Preferences::Bottom]));
}
