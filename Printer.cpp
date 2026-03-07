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
                QImage phantomPixel(1, 1, QImage::Format_ARGB32);
                phantomPixel.fill(Qt::transparent);

                mDoc.addResource(QTextDocument::ImageResource, QUrl(imgFmt.name()), phantomPixel);

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
    auto marginals = parseMarginal(mPrefs->footer());
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
        if (fragment.charFormat().isEmpty()) return " ";
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

    return words.takeFirst();
}

bool Printer::outputNovel(List<qlonglong> ids,
                          const QString &chapterTag,
                          const QString &sceneTag,
                          const QString &coverTag,
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
    mDoc.setDefaultFont(font);
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
        }

        auto paragraphs = createParagraphs(item, scaleX, scaleY, lineWidth, pageHeight);
        printParagraphs(painter, pageSize, finishPage, margins, at, startingPage, paragraphs, item.hasTag(coverTags) && mPageNo == 1);
    }

    if (!startingPage) page(painter, finishPage, true);

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
    QPainter* paint = painter();
    outputNovel(mIds, mPrefs->chapterTag(), mPrefs->sceneTag(), mPrefs->coverTag(), paint, size,
                [this, &paint](bool m) {
                    if (m) {
                        header(paint);
                        footer(paint);
                    }
                    newPage();
                });
    delete paint;
}

void Printer::printParagraphs(QPainter* painter,
                              QSizeF &pageSize,
                              std::function<void(bool)>& newPage,
                              QMarginsF& margins,
                              qreal& at,
                              bool& startingPage,
                              QList<QTextBlock>& paragraphs,
                              bool isCover) {
    qreal lineWidth = pageSize.width() - margins.left() - margins.right();
    qreal bottom = pageSize.height() - margins.bottom();

    QFont font = mDoc.defaultFont();
    painter->setFont(font);
    QFontMetrics fontMetrics(font);
    int en = fontMetrics.horizontalAdvance("N");
    int indent = 4 * en;
    int baseLine = fontMetrics.descent();
    painter->fillRect({ QPoint(0, 0), pageSize }, Qt::white);

    for (auto&& paragraph: paragraphs) {
        QList<QTextFragment> fragments;
        for (QTextBlock::iterator it = paragraph.begin(); !it.atEnd(); ++it) fragments << it.fragment();

        bool firstLineIndent = true;
        bool fill = false;
        auto paraFormat = paragraph.blockFormat().alignment();
        switch(paraFormat) {
        case Qt::AlignLeft:    firstLineIndent = true;  fill = false; break;
        case Qt::AlignCenter:  firstLineIndent = false; fill = false; break;
        case Qt::AlignRight:   firstLineIndent = false; fill = false; break;
        case Qt::AlignJustify: firstLineIndent = true;  fill = true;  break;
        }

        int x = firstLineIndent ? indent : 0;
        int lineHeight = fontMetrics.lineSpacing();

        int width = 0;
        int currentFormat = 0;

        bool startLine = true;

        lineImages.clear();
        QList<Word> line;
        QString word = nextWord(currentFormat, fragments);
        int height = 0;
        for (; ; ) {
            if (at + baseLine > bottom) {
                page(painter, newPage, !startingPage);
                ++mPageNo;
                startingPage = false;
                at = margins.top();
            }
            if (word.isEmpty()) break;

            if (currentFormat == image) {
                if (!lineImages.contains(word)) {
                    if (word.startsWith("internal://")) {
                        auto& images = Main::ref().ui()->textEdit->internalImages();
                        if (images.contains(word)) lineImages[word] = images[word];
                    } else lineImages[word] = Main::ref().ui()->textEdit->document()->resource(QTextDocument::ImageResource, QUrl(word)).value<QImage>();
                }

                int imgHeight = lineImages[word].height();
                int newHeight = (height > imgHeight) ? height : imgHeight;

                if (lineImages[word].width() + width > lineWidth ||
                    newHeight + at > bottom) {
                    renderLine(painter, margins.left(), at, line, fill, paraFormat, lineWidth, margins.left());
                    at += height;
                    height = 0;
                    width = 0;
                    x = 0;
                    line.clear();
                    startLine = true;
                    continue;
                }

                line.append({ word, image, lineImages[word].size() });
                width += lineImages[word].width();
                startLine = false;
            } else {
                int len = fontMetrics.horizontalAdvance(word);

                if (len + width > lineWidth) {
                    renderLine(painter, x, at, line, fill, paraFormat, lineWidth, margins.left());
                    at += height;
                    height = 0;
                    line.clear();
                    startLine = true;
                    width = 0;
                    x = 0;
                    continue;
                }
                QSize size(len, lineHeight);
                line.append({ word, currentFormat, size });
                width += size.width() + (startLine ? 0: en );
                startLine = false;
            }
            word = nextWord(currentFormat);
        }

        if (line.count() != 0) renderLine(painter, x, at, line, fill, paraFormat, lineWidth, margins.left());
    }
}

void Printer::renderLine(QPainter* painter, qreal x, qreal at, QList<Word>& line, bool fill, Qt::Alignment para, int lineWidth, int left) {
    bool start = true;
    QFont font = painter->font();
    QFontMetrics metrics(font);
    int space = metrics.horizontalAdvance("N");
    int height = 0;
    int width = 0;
    for (auto& word: line) {
        width = width + word.pSize.width() + ((word.pFormat != image && !start) ? space : 0);
        if (height < word.pSize.height()) height = word.pSize.height();
    }

    if (para == Qt::AlignRight)       x = left + lineWidth - width;
    else if (para == Qt::AlignCenter) x = left + (lineWidth - width) / 2;
    else if (fill)                    space += (lineWidth - width) / (line.count() - 1);

    start = true;
    at += height;
    qreal lineHeight = metrics.lineSpacing();
    for (auto& word: line) {
        if (word.pFormat == image) {
            QImage img = lineImages[word.pText];
            QSize imgSize = img.size();
            QRect rect(QPoint(x + 0.5, at - word.pSize.height() + 0.5), imgSize);
            painter->drawImage(rect, img);
            start = false;
            x += word.pSize.width();
        } else {
            if (!start) x += space;
            QFont wordFont = font;
            if (word.pFormat & bold)      font.setWeight(QFont::Bold);
            if (word.pFormat & italic)    font.setItalic(true);
            if (word.pFormat & underline) font.setUnderline(true);
            start = false;
            painter->drawText(QPoint(x + 0.5, at - lineHeight + 0.5), word.pText);
            x += word.pSize.width();
        }
    }
}

QString Printer::resolveTag(const QString &key) {
    QString value;
    for (const auto& id: mIds) {
        const Item& item = Main::ref().novel().findItem(id);
        const auto& tags = item.tags();
        QString ret = "";
        for (const auto& tag: tags) {
            if (tag.startsWith(key + ":", Qt::CaseInsensitive)) value = tag.mid(key.length() + 1);
        }
        if (mId == id) break;
    }
    return value;
}
