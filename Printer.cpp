#include "Main.h"
#include "Printer.h"

#include <QTextBlock>
#include <QTextDocumentFragment>

void Printer::printNovel() {
    setFullPage(true);
    QSizeF size = pageSize(QPrinter::Point);
    QPainter* paint = painter();
    outputNovel(mIds, mPrefs->chapterTag(), mPrefs->sceneTag(), mPrefs->coverTag(), paint, size,
                [this, &paint]() {
                    header(paint);
                    footer(paint);
                    newPage();
                });
    delete paint;
}

void Printer::printParagraphs(QPainter* painter,
                              QSizeF &pageSize,
                              std::function<void()> &pager,
                              QMarginsF &margins,
                              QFont &font,
                              qreal &at,
                              bool &startingPage,
                              StringList &paragraphs) {
    QFontMetricsF metrics(font, paintdevice());
    qreal lineWidth = pageSize.width() - margins.left() - margins.right();
    qreal lineHeight = metrics.height() / mYFactor;
    qreal pageHeight = pageSize.height() - margins.bottom() - margins.top();
    qreal bottom = pageSize.height() - margins.bottom();
    qreal ascent = metrics.ascent() / mYFactor;
    qreal space = metrics.horizontalAdvance("N") / mXFactor;
    Words::Tags current = Words::Tags::None;
    auto indentDistance = 4 * metrics.horizontalAdvance("M") / mXFactor;
    for (const auto& paragraph: paragraphs) {
        int indent = indentDistance;
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
        List<QString> imgs;
        while (!words.isEmpty()) {
            auto width = extra;
            if (first) width += indent;
            List<Words::Word> line;
            qreal workingLineHeight = lineHeight;
            while (!words.isEmpty()) {
                qreal imageHeight = 0.0;
                QString imgUrl;
                Words::Word word = words.first();
                if (line.size() != 0 &&  (current & Words::Tags::Partial) != Words::Tags::None) width += space;
                qreal withWord = 0.0;

                if (word.hasTag(Words::Tags::Image)) {
                    QString str = word.str();
                    auto [x, y, img] = clampToPage(str, lineWidth, pageHeight);
                    if (x == 0.0 || y == 0.0) {
                        words.takeFirst();
                        continue;
                    }
                    withWord = width + x;
                    workingLineHeight = y;
                    str = "\x1F" + QString::number(imgs.count());
                    words[0].setStr(str);
                } else withWord = width + metrics.horizontalAdvance(word.str()) / mXFactor;

                if (withWord < lineWidth) {
                    if (word.hasTag(Words::Tags::Image)) {
                        imgs.append(imgUrl);
                    }
                    auto word = words.takeFirst();
                    line.append(word);
                    width = withWord;
                    if (!word.isPartial()) width += space;
                } else break;
            }

            if (at + workingLineHeight > bottom) {
                newPage(painter, pager);
                ++mPageNo;
                at = margins.top() + ascent;
                startingPage = true;
            }

            qreal fill = 0.0;
            qreal x = margins.left() + indent + extra;
            if (!line.empty() && align == "J") {
                if (line.count() > 1 && !words.isEmpty()) fill = width / (line.count() - 1);
            } else {
                if (align == "R") x = pageSize.width() / mXFactor - margins.right() - width;
                else if (align == "C") x = pageSize.width() / 2.0 - width / 2.0;
            }

            printLine(painter, line, imgs, font, current, x, at, fill);
            at += lineHeight;
            startingPage = false;
            indent = 0;
            imgs.clear();
        }
    }
}

QList<Words::Word> Printer::paragraphWords(const QString &paragraph) {
    QList<Words::Word> words;
    StringList wordList { paragraph.split(" ", Qt::SkipEmptyParts) };
    for (const auto& item: wordList) {
        StringList parts { item.split("<", Qt::SkipEmptyParts) };
        // now parts has (for ex. b>text, /b> or other text formatting
        //                     or img ... src=\"...\" ... />) at a minimum
        Words::Word word;
        Words::Word img;
        bool inImage = false;
        for (auto&& [i, part]: enumerate(parts)) {
            QString fragment = part;
            if (i != 0 && inImage) {
                if (fragment.startsWith("/>")) {
                    inImage = false;
                    words.append(img);
                    img.clear();
                } else img.setStr(img.str() + " " + fragment);
                continue;
            } else if (i != 0 && fragment.startsWith("img ")) {
                img += Words::Tags::Image;
                inImage = true;
                continue;
            }
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

void Printer::footer(QPainter* painter) {
    auto marginals = parseMarginal(mPrefs->footer());
    qreal bottom = mPrefs->margins()[Preferences::Bottom] * PointsPerInch * mYFactor;
    qreal y = mPrinter->pageRect(QPrinter::Point).height() * mYFactor - bottom;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    QFontMetricsF metrics(font, paintdevice());
    y -= metrics.descent();
    y += metrics.lineSpacing();
    for (const auto& marginal: marginals) {
        double line = y + marginal.line() * metrics.lineSpacing();
        printMarginal(painter, line, marginal);
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

void Printer::printCover(Item &item, bool startingPage, QSizeF &pageSize, QMarginsF &margins, QPainter *painter, std::function<void()> printer) {
    QPointF at = QPointF(margins.top(), margins.left());
    if (mPageNo != 1) {
        if (!startingPage) {
            newPage(painter, printer, true);
            ++mPageNo;
            if (mPageNo % 2 != 0) {
                newPage(painter, printer, true);
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
            auto width = (pageSize.width() - margins.right() - margins.left()) * mXFactor;
            auto height = (pageSize.height() - margins.top() - margins.bottom()) * mYFactor;
            if (image.width() < width) scaledImage = scaledImage.scaledToWidth(width, Qt::SmoothTransformation);
            if (scaledImage.height() > height) scaledImage = scaledImage.scaledToHeight(height, Qt::SmoothTransformation);
            if (at.y() * mYFactor + scaledImage.height() > (pageSize.height() - margins.bottom()) * mYFactor) {
                newPage(painter, printer);
                ++mPageNo;
                at = QPointF(margins.left(), margins.top());
            }
            auto size = scaledImage.size();
            QRectF imageRect(at.x() * mXFactor, at.y() * mYFactor, size.width(), size.height());
            painter->drawImage(imageRect, scaledImage);
            at.setY(at.y() + scaledImage.height());
        }
    }
}

void Printer::header(QPainter* painter) {
    auto marginals = parseMarginal(mPrefs->header());
    if (marginals.size() < 1) return;

    qreal y = mPrefs->margins(Preferences::Units::Points)[Preferences::Top] * mYFactor;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    QFontMetricsF metrics(font, paintdevice());
    y += metrics.descent();
    y -= metrics.lineSpacing();
    y -= marginals.last().line() * metrics.lineSpacing();
    for (const auto& marginal: marginals) {
        double line = y + marginal.line() * metrics.lineSpacing();
        printMarginal(painter, line, marginal);
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

void Printer::newPage(QPainter* painter,
                      std::function<void()> printer,
                      bool marginals) {
    if (marginals) {
        header(painter);
        footer(painter);
    }
    printer();
}

void Printer::outputNovel(List<qlonglong> ids,
                          const QString &chapterTag,
                          const QString &sceneTag,
                          const QString &coverTag,
                          QPainter* painter,
                          QSizeF pageSize,
                          std::function<void()> pager) {
    if (mPrinter == nullptr) return;
    auto pointMargins = mPrefs->margins(Preferences::Units::Points);
    QMarginsF margins(pointMargins[Preferences::Left], pointMargins[Preferences::Top], pointMargins[Preferences::Right], pointMargins[Preferences::Bottom]);
    mXFactor = physicalDpiX() / PointsPerInch;
    mYFactor = physicalDpiX() / PointsPerInch;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    font.setPixelSize(mYFactor * font.pixelSize());
    QFontMetricsF metrics(font, paintdevice());
    qreal ascent = metrics.ascent() / mYFactor;
    qreal lineHeight = metrics.height() / mYFactor;
    qreal at = margins.top() + ascent;
    qreal bottom = pageSize.height() - margins.bottom();
    painter->setFont(font);

    mPageNo = 1;
    bool startingPage = true;
    mIds = ids;
    for (const auto& id : ids) {
        mId = id;
        Item& item = Main::ref().novel().findItem(id);
        if (!item.hasTag(chapterTag) && !item.hasTag(sceneTag) && !item.hasTag(coverTag)) continue;
        if (item.hasTag(coverTag)) {
            printCover(item, startingPage, pageSize, margins, painter, pager);
            newPage(painter, pager, false);
            at = margins.top() + ascent;
            startingPage = true;
            continue;
        } else if (item.hasTag(chapterTag)) {
            if (!startingPage && mPageNo != 1) {
                newPage(painter, pager);
                ++mPageNo;
                at = margins.top() + ascent;
                startingPage = true;
            }
        }

        StringList paragraphs = createParagraphs(item, WithAlignment);
        printParagraphs(painter, pageSize, pager, margins, font, at, startingPage, paragraphs);
        if (item.hasTag(chapterTag)) {
            at += lineHeight;
            if (at > bottom) {
                newPage(painter, pager);
                ++mPageNo;
                at = margins.top() + ascent;
                startingPage = true;
            }
        }
    }

    if (!startingPage) newPage(painter, pager);
}

void Printer::printLine(QPainter* painter, List<Words::Word> &line, List<QString>& imgs, QFont &font, Words::Tags &current, qreal x, qreal at, qreal fill) {
    QFontMetricsF metrics(painter->fontMetrics());
    at *= mYFactor;
    fill *= mXFactor;
    x *= mXFactor;
    int space = metrics.horizontalAdvance("N");

    bool first = false;
    while (!line.empty()) {
        Words::Word word = line.takeFirst();
        if (word.str().left(1) == "\x1F") {
            auto str = word.str();
            auto temp = str.mid(1);
            auto imgIdx = temp.toInt();
            QImage image(imgs[imgIdx]);
            auto size = image.size();
            size.setWidth(size.width() * mXFactor);
            size.setHeight(size.height() * mYFactor);
            QRectF imageRect(x, at, size.width(), size.height());
            painter->drawImage(imageRect, image);
            x += size.width();
        } else {
            current = (current & word.tags()) | word.tags();
            QFont wFont = font;
            if ((current & Words::Tags::Bold) != Words::Tags::None)      wFont.setWeight(QFont::Bold);
            if ((current & Words::Tags::Italic) != Words::Tags::None)    wFont.setItalic(true);
            if ((current & Words::Tags::Underline) != Words::Tags::None) wFont.setUnderline(true);
            painter->setFont(wFont);
            painter->drawText(QPoint(x + 0.5, at + 0.5), word.str());
            x += fill + metrics.horizontalAdvance(word.str());
            if (!word.isPartial()) x += space;
        }
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
    for (auto&& [i, str]: enumerate(lcr)) {
        StringList lines { str.split("\n", Qt::KeepEmptyParts) };
        Marginal::Justify justify = Marginal::Justify::Left;
        switch (i) {
        case 1: justify = Marginal::Justify::Center; break;
        case 2: justify = Marginal::Justify::Right;  break;
        }
        for (auto&& [lineNum, line]: enumerate(lines)) marginals.append({ justify, lineNum, line });
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

std::tuple<qreal, qreal, QString> Printer::clampToPage(QString& img, const qreal width, const qreal height) {
    qreal newWidth = 0.0;
    qreal newHeight = 0.0;
    QString imgUrl;

    StringList imgParts = { img. split("=\"") };
    if (imgParts.size() >= 4) {
        auto imgStr = imgParts[1];
        auto widthStr = imgParts[2];
        auto heightStr = imgParts[3];
        auto loc = widthStr.indexOf("\"");
        if (loc != -1) {
            QString temp = widthStr.left(loc);
            newWidth = temp.toInt();
            if (newWidth < 1.0) newWidth = 0;
            else {
                loc = heightStr.indexOf("\"");
                if (loc == -1) newWidth = 0.0;
                else {
                    temp = heightStr.left(loc);
                    newHeight = temp.toInt();
                    if (newHeight < 1.0) newHeight = newWidth = 0.0;
                    else {
                        if (newWidth > width) {
                            double scale = width / newWidth;
                            newWidth = width;
                            newHeight = height * scale;
                        }
                        if (newHeight > height) {
                            double scale = height / newHeight;
                            newHeight = height;
                            newWidth = width * scale;
                        }
                    }
                }
            }
        }

        loc = imgStr.indexOf("\"");
        if (loc != -1) imgUrl = imgStr.left(loc);
        else newWidth = newHeight = 0.0;

        img = imgParts[0] + "=" + imgParts[1] + "\"" + QString::number(newWidth) + "\" height`=\"" + QString::number(newHeight) + "\" />";
    }
    return { newWidth, newHeight, imgUrl };
}

void Printer::printMarginal(QPainter* painter, qlonglong y, const Marginal& object) {
    qreal left = mPrefs->margins()[Preferences::Left] * PointsPerInch * mXFactor;
    qreal right = (mPrinter->pageRect(QPrinter::Point).width() -  mPrefs->margins()[Preferences::Right] * PointsPerInch) * mXFactor;
    qreal center = mPrinter->pageRect(QPrinter::Point).width() * mXFactor / 2;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    QFontMetricsF metrics(painter->fontMetrics());
    QString text = object.text();
    auto width = metrics.horizontalAdvance(text);
    auto x = left;

    switch (object.justify()) {
    case Marginal::Justify::Center: x = center - width / 2; break;
    case Marginal::Justify::Left:                           break;
    case Marginal::Justify::Right:  x = right - width;      break;
    }

    painter->drawText(QPoint({ int(x + 0.5), int(y + 0.5) }), text);
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
    for (auto&& [i, para]: enumerate(bodyless)) {
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

const QSizeF Printer::pageSize(QPrinter::Unit unit) const {
    return paperRect(unit).size();
}
