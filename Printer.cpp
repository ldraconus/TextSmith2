#include "Main.h"
#include "ui_Main.h"
#include "Printer.h"

#include "TextEdit.h"

static constexpr auto InchesPerMeter = 39.3701;
static constexpr auto ScreenDPI = 96.0;

QList<QTextBlock> Printer::createParagraphs(Item& item, qreal scaleX, qreal scaleY, qreal lineWidth, qreal pageHeight) {
    mDoc.setHtml(item.html());

    QTextCursor cursor(&mDoc);
    cursor.movePosition(QTextCursor::Start);

    QTextBlockFormat format;
    format.setLineHeight(0, QTextBlockFormat::FixedHeight);
    format.setTopMargin(0);
    format.setBottomMargin(0);

    mParaImages.clear();

    for (QTextBlock block = mDoc.begin(); block != mDoc.end(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();

            if (fragment.charFormat().isImageFormat()) {
                QTextImageFormat imgFmt = fragment.charFormat().toImageFormat();

                QImage img;
                if (imgFmt.name().startsWith("internal://")) {
                    auto& images = Main::ref().ui()->textEdit->internalImages();
                    if (images.contains(imgFmt.name())) img = images[imgFmt.name()];
                } else img = Main::ref().ui()->textEdit->document()->resource(QTextDocument::ImageResource, QUrl(imgFmt.name())).value<QImage>();

                if (img.isNull()) continue;

                qreal origW = (imgFmt.width() > 0 ? imgFmt.width() : img.width()) * scaleX;
                qreal origH = (imgFmt.height() > 0 ? imgFmt.height() : img.height()) * scaleY;

                if (origW > lineWidth) {
                    qreal shrinkRatio = lineWidth / origW;
                    origW = lineWidth;
                    origH = origH * shrinkRatio;
                }

                if (origH > pageHeight) {
                    qreal shrinkRatio = pageHeight / origH;
                    origH = pageHeight;
                    origW = origW * shrinkRatio;
                }

                QImage image = img.scaled(origW, origH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                if (!mParaImages.contains(imgFmt.name())) mParaImages[imgFmt.name()] = image;

                mDoc.addResource(QTextDocument::ImageResource, QUrl(imgFmt.name()), image);

                imgFmt.setWidth(origW);
                imgFmt.setHeight(origH);

                QTextCursor cursor(&mDoc);
                cursor.setPosition(fragment.position());
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                cursor.setCharFormat(imgFmt);
            }
        }
    }

    QList<QTextBlock> paragraphs;
    for (QTextBlock block = mDoc.begin(); block != mDoc.end(); block = block.next()) {
        if (!block.isValid()) continue;
        paragraphs.append(block);
    }

    return paragraphs;
}

static constexpr int bold =      1 << 0;
static constexpr int italic =    1 << 1;
static constexpr int underline = 1 << 2;
static constexpr int image =     1 << 3;

static QMap<QString, QImage> lineImages;

void Printer::footer(QPainter* painter) {
    auto marginals = parseMarginal(mPrefs->footer(), mPageNo, mIds, mId);
    qreal bottom = mPrefs->margins()[Preferences::Bottom] * PointsPerInch * mYFactor;
    qreal y = pageRect(QPrinter::Point).height() * mYFactor - bottom;
    QFont font(mPrefs->fontFamily(), mPrefs->fontSize());
    QFontMetricsF metrics(font, paintdevice());
    y -= metrics.descent();
    y += metrics.lineSpacing();
    for (const auto& marginal: marginals) {
        double line = y + marginal.line() * metrics.lineSpacing();
        printMarginal(painter, line, marginal);
    }
}

void Printer::header(QPainter* painter) {
    auto marginals = parseMarginal(mPrefs->header(), mPageNo, mIds, mId);
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

QString Printer::nextWord(int& charFormat, const QList<QTextFragment>& newFragments) {
    static QList<QTextFragment> fragments;
    static QTextFragment fragment;
    static QList<QString> words;

    if (!newFragments.isEmpty()) {
        fragments = newFragments;
        words.clear();
    }

    if (words.isEmpty()) {
        fragment = { };
        while (!fragments.isEmpty()) {
            fragment = fragments.takeFirst();
            if (fragment.isValid()) break;
        }
        if (!fragment.isValid()) return "";
        if (fragment.charFormat().isEmpty()) {
            // blank line?
            charFormat = 0;
            return "\n";
        }
        if (fragment.charFormat().isImageFormat()) {
            charFormat = image;
            QTextImageFormat imgFmt = fragment.charFormat().toImageFormat();
            return imgFmt.name();
        } else words = fragment.text().split(" ", Qt::KeepEmptyParts);

        if (words.isEmpty()) return "";
    }

    charFormat = 0;
    if (fragment.charFormat().fontItalic())                charFormat |= italic;
    if (fragment.charFormat().fontUnderline())             charFormat |= underline;
    if (fragment.charFormat().fontWeight() >= QFont::Bold) charFormat |= bold;

    while (!words.isEmpty()) {
        QString word = words.takeFirst();
        if (word.isEmpty()) return " ";
        else return word;
    }

    return "";
}

bool Printer::outputNovel(List<qlonglong> ids,
                          const QString& chapterTag,
                          const QString& sceneTag,
                          const QString& coverTag,
                          const QString& sceneSeparator,
                          QPainter* painter,
                          QSizeF pageSize,
                          std::function<void(bool)> finishPage) {
    StringList chapterTags { chapterTag.split(",") };
    chapterTags.trimmed();
    StringList sceneTags { sceneTag.split(",") };
    sceneTags.trimmed();
    StringList coverTags { coverTag.split(",") };
    coverTags.trimmed();

    if (mPrinter == nullptr && mPdf == nullptr) return false;
    if (!painter->isActive()) return false;

    mXFactor = physicalDpiX() / PointsPerInch;
    mYFactor = physicalDpiX() / PointsPerInch;

    painter->setPen(Qt::black);
    painter->setBackground(Qt::white);
    painter->fillRect(paperRect(QPrinter::DevicePixel).toRect(), Qt::white);

    auto pointMargins = mPrefs->margins(Preferences::Units::Points);
    QMarginsF margins(pointMargins[Preferences::Left], pointMargins[Preferences::Top], pointMargins[Preferences::Right], pointMargins[Preferences::Bottom]);
    margins.setLeft(margins.left() * mXFactor);
    margins.setRight(margins.right() * mXFactor);
    margins.setTop(margins.top() * mYFactor);
    margins.setBottom(margins.bottom() * mYFactor);

    pageSize.setWidth(pageSize.width() * mXFactor);
    pageSize.setHeight(pageSize.height() * mYFactor);

    qreal lineWidth = pageSize.width() - margins.left() - margins.right();
    qreal pageHeight = pageSize.height() - margins.top() - margins.bottom();
    qreal at = margins.top();

    qreal scaleX = physicalDpiX() / ScreenDPI;
    qreal scaleY = physicalDpiY() / ScreenDPI;

    mPageNo = 1;
    bool startingPage = true;
    mIds = ids;
    mDoc.setTextWidth(lineWidth);
    QFont font(Main::ref().ui()->textEdit->document()->defaultFont());
    int size = font.pixelSize() * scaleY;
    if (size < 1) size = font.pointSize() * mYFactor;
    font.setPixelSize(size);
    QFontMetrics metrics(font);
    mDoc.setDefaultFont(font);
    bool firstScene = true;
    for (const auto& id: ids) {
        mId = id;
        Item& item = Main::ref().novel().findItem(id);
        if (id != -1 && !item.hasTag(chapterTags) && !item.hasTag(sceneTags) && !item.hasTag(coverTags)) continue;
        if (item.hasTag(chapterTags)) {
            if (!startingPage) {
                page(painter, finishPage, true);
                ++mPageNo;
                at = margins.top();
                startingPage = true;
            }
            firstScene = true;
        }
        auto paragraphs = createParagraphs(item, scaleX, scaleY, lineWidth, pageHeight);
        printParagraphs(painter, pageSize, finishPage, margins, at, startingPage, paragraphs, item.hasTag(coverTags) && mPageNo == 1);
        if (item.hasTag(sceneTags)) {
            if (firstScene) firstScene = false;
            else if (mPrefs->useSeparator()) {
                QString text = mPrefs->separator();
                printLine(painter, pageSize, finishPage, margins, font, at, "\n", startingPage);
                printLine(painter, pageSize, finishPage, margins, font, at, text, startingPage);
                printLine(painter, pageSize, finishPage, margins, font, at, "\n", startingPage);
            }
        }
        if (item.hasTag(coverTags) && startingPage && mPageNo == 1) {
            ++mPageNo;
            at = margins.top();
            startingPage = true;
            page(painter, finishPage, true);
        }
    }

    if (!startingPage) page(painter, finishPage, false);

    return true;
}

const QSizeF Printer::pageSize(QPrinter::Unit unit) const {
    return paperRect(unit).size();
}

void Printer::page(QPainter* painter,
                   std::function<void(bool)> finishPage,
                   bool marginals) {
    finishPage(marginals);
    painter->fillRect(paperRect(QPrinter::DevicePixel).toRect(), Qt::white);
}

List<Printer::Marginal> Printer::parseMarginal(const QString &marginal, int pageNo, List<qlonglong>& ids, qlonglong id) {
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

    /*
     * handle: \\ -      \
     *         \# -      page number
     *         \(tag) - look up tag for this id:value, output value
     *         every other \, remove.
     */
    for (auto& margin: marginals) {
        QString text = margin.text();
        text.replace("\\\\", QChar(0x001e));
        text.replace("\\#", QString::number(pageNo));
        StringList tags { text.split("\\(") };
        text = "";
        bool first = true;
        for (const auto& tag: tags) {
            if (first) text = tag;
            else {
                StringList parts { tag.split(")") };
                QString value = "";
                if (parts.size() != 1) text += Printer::resolveTag(parts[0], ids, id) + parts[1];
            }
            first = false;
        }

        text.replace("\\", "");
        text.replace(QChar(0x001e), "\\");
        margin.setText(text);
    }

    return marginals;
}

void Printer::printLine(QPainter* painter,
                        QSizeF &pageSize,
                        std::function<void(bool)>& emitPage,
                        QMarginsF& margins,
                        QFont& font,
                        qreal& at,
                        const QString& text,
                        bool& startingPage) {
    qreal bottom = pageSize.height() - margins.bottom();
    QFontMetrics fontMetrics(font);
    int baseLine = fontMetrics.descent();
    if (at + baseLine > bottom) {
        page(painter, emitPage, !startingPage);
        ++mPageNo;
        startingPage = true;
        at = margins.top();
    }
    if (text.isEmpty()) {
        at += fontMetrics.lineSpacing();
        return;
    }
    qreal center = pageRect(QPrinter::Point).width() * mXFactor / 2;
    auto width = fontMetrics.horizontalAdvance(text);
    auto x = center - width / 2;
    painter->drawText(QPoint({ int(x + 0.5), int(at + 0.5) }), text);
    at += fontMetrics.lineSpacing();
}

void Printer::printMarginal(QPainter* painter, qlonglong y, const Marginal& object) {
    qreal left = mPrefs->margins()[Preferences::Left] * PointsPerInch * mXFactor;
    qreal right = (pageRect(QPrinter::Point).width() -  mPrefs->margins()[Preferences::Right] * PointsPerInch) * mXFactor;
    qreal center = pageRect(QPrinter::Point).width() * mXFactor / 2;
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

void Printer::printNovel() {
    setFullPage(true);
    QSizeF size = pageSize(QPrinter::Point);
    QPainter* painter = this->painter();
    QString separator = mPrefs->useSeparator() ? mPrefs->separator() : "";
    outputNovel(mIds, mPrefs->chapterTag(), mPrefs->sceneTag(), mPrefs->coverTag(), separator, painter, size,
                [this, &painter](bool m) {
                    if (m) {
                        header(painter);
                        footer(painter);
                    }
                    newPage();
                });
}

void Printer::printParagraphs(QPainter* painter,
                              QSizeF &pageSize,
                              std::function<void(bool)>& emitPage,
                              QMarginsF& margins,
                              qreal& at,
                              bool& startingPage,
                              QList<QTextBlock>& paragraphs,
                              bool isCover) {
    qreal lineWidth = pageSize.width() - margins.left() - margins.right();
    qreal bottom = pageSize.height() - margins.bottom();

    QFont font = mDoc.defaultFont();
    QFontMetrics fontMetrics(font);
    int en = fontMetrics.horizontalAdvance(" ");
    int indent = 4 * en;
    int baseLine = fontMetrics.descent();

    for (auto&& [num, paragraph]: enumerate(paragraphs)) {
        QList<QTextFragment> fragments;
        for (QTextBlock::iterator it = paragraph.begin(); !it.atEnd(); ++it) fragments << it.fragment();

        bool firstLineIndent = true;
        bool fill = false;
        auto paraFormat = paragraph.blockFormat().alignment();
        switch(paraFormat) {
        case Qt::AlignLeft:    firstLineIndent = true;  fill = false; break;
        case Qt::AlignHCenter: firstLineIndent = false; fill = false; break;
        case Qt::AlignRight:   firstLineIndent = false; fill = false; break;
        case Qt::AlignJustify: firstLineIndent = true;  fill = true;  break;
        }

        int x = margins.left() + (firstLineIndent ? indent : 0);
        int lineHeight = fontMetrics.lineSpacing();

        int width = 0;
        int currentFormat = 0;

        bool startLine = true;

        lineImages.clear();
        QList<Word> line;
        QString word = nextWord(currentFormat, fragments);
        int height = lineHeight;
        for (; ; ) {
            if (at + baseLine > bottom) {
                page(painter, emitPage, !startingPage || !isCover);
                ++mPageNo;
                startingPage = true;
                at = margins.top();
            }
            while (word == "\n") {
                at += lineHeight;
                if (at + baseLine > bottom) {
                    page(painter, emitPage, !startingPage || !isCover);
                    ++mPageNo;
                    startingPage = false;
                    at = margins.top();
                }
                word = nextWord(currentFormat);
            }
            if (word.isEmpty()) break;

            if (currentFormat == image) {
                if (!lineImages.contains(word)) lineImages[word] = mParaImages[word];

                int imgHeight = lineImages[word].height();
                height = (height > imgHeight) ? height : imgHeight;

                if (lineImages[word].width() + width > lineWidth || height + at > bottom) {
                    renderLine(painter, font, x, at, line, fill, paraFormat, lineWidth, margins.left());
                    at += height;
                    height = lineHeight;
                    line.clear();
                    startLine = true;
                    width = 0;
                    x = margins.left();
                    startingPage = false;
                    continue;
                }

                line.append({ word, image, lineImages[word].size() });
                width += lineImages[word].width();
                startLine = false;
                startingPage = false;
            } else {
                QFont wordFont = font;
                if (currentFormat & bold)      wordFont.setWeight(QFont::Bold);
                if (currentFormat & italic)    wordFont.setItalic(true);
                if (currentFormat & underline) wordFont.setUnderline(true);
                QFontMetrics metrics(wordFont);
                int len = metrics.horizontalAdvance(word);

                if (len + width > lineWidth) {
                    renderLine(painter, font, x, at, line, fill, paraFormat, lineWidth, margins.left());
                    at += height;
                    height = lineHeight;
                    line.clear();
                    startLine = true;
                    width = 0;
                    x = margins.left();
                    startingPage = false;
                    continue;
                }

                QSize size(len, lineHeight);
                line.append({ word, currentFormat, size });
                width += size.width() + (startLine ? 0: en );
                startLine = false;
            }
            word = nextWord(currentFormat);
        }

        if (line.count() != 0) {
            renderLine(painter, font, x, at, line, fill, paraFormat, lineWidth, margins.left());
            at += height;
            height = lineHeight;
            startingPage = false;
        }
        at += height;
    }
}

void Printer::renderLine(QPainter* painter, QFont font, qreal x, qreal at, QList<Word>& line, bool fill, Qt::Alignment para, int lineWidth, int left) {
    bool start = true;
    int height = 0;
    int width = 0;
    QFontMetrics metrics(font);
    int space = metrics.horizontalAdvance(" ");
    for (auto& word: line) {
        width = width + word.pSize.width() + ((word.pFormat != image && !start) ? space : 0);
        if (height < word.pSize.height()) height = word.pSize.height();
        start = false;
    }

    qreal extraSpace = 0;
    if (para == Qt::AlignRight)        x = left + lineWidth - width;
    else if (para == Qt::AlignHCenter) x = left + (lineWidth - width) / 2;
    else if (fill)                     extraSpace = (lineWidth - width) / qreal(line.count() - 1);

    qreal pos = x;
    start = true;
    at += height;
    qreal lineHeight = metrics.lineSpacing();
    for (auto& word: line) {
        if (word.pFormat == image) {
            QImage img = lineImages[word.pText];
            QSize imgSize = word.pSize;
            QRect rect(QPoint(x + 0.5, at - imgSize.height() + 0.5), imgSize);
            painter->drawImage(rect, img);
            start = false;
            pos += word.pSize.width();
        } else {
            QFont wordFont = font;
            if (word.pFormat & bold)      wordFont.setWeight(QFont::Bold);
            if (word.pFormat & italic)    wordFont.setItalic(true);
            if (word.pFormat & underline) wordFont.setUnderline(true);
            QFontMetrics metrics(wordFont);
            qreal doSpace = space + extraSpace;
            if (!start) pos += doSpace;
            start = false;
            painter->setFont(wordFont);
            painter->drawText(QPoint(pos + 0.5, at - lineHeight + 0.5), word.pText);
            pos += word.pSize.width();
        }
    }
}

QString Printer::resolveTag(const QString& key, List<qlonglong>& ids, qlonglong id) {
    QString value;
    for (const auto& i: ids) {
        const Item& item = Main::ref().novel().findItem(id);
        const auto& tags = item.tags();
        QString ret = "";
        for (const auto& tag: tags) {
            if (tag.startsWith(key + ":", Qt::CaseInsensitive)) value = tag.mid(key.length() + 1);
        }
        if (i == id) break;
    }
    return value;
}
