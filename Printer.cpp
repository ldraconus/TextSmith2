#include "Main.h"
#include "ui_Main.h"
#include "Printer.h"

#include <QAbstractTextDocumentLayout>

static constexpr auto InchesPerMeter = 39.3701;
static constexpr auto ScreenDPI = 96.0;

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
                              std::function<void()>& newPage,
                              QMarginsF &margins,
                              qreal &at,
                              bool &startingPage,
                              List<QTextBlock>& paragraphs) {
    qreal lineWidth = pageSize.width() - margins.left() - margins.right();
    qreal bottom = pageSize.height() - margins.bottom();
    qreal scaleX = physicalDpiX() / ScreenDPI;
    qreal scaleY = physicalDpiY() / ScreenDPI;

    for (const auto& paragraph: paragraphs) {
        QTextLayout* layout = paragraph.layout();
        layout->beginLayout();
        for (; ; ) {
            QTextLine line = layout->createLine();
            if (!line.isValid()) break;
            line.setLineWidth(lineWidth);
        }
        layout->endLayout();

        for (auto i = 0; i < layout->lineCount(); ++i) {
            QTextLine line = layout->lineAt(i);
            if (at + line.height() > bottom && !startingPage) {
                page(painter, newPage, true);
                at = margins.top();
            }
            line.draw(painter, QPointF(margins.left(), at));
            at += line.height();
            startingPage = false;

            QTextBlock::iterator it;
            for (it = paragraph.begin(); !it.atEnd(); ++it) {
                QTextFragment fragment = it.fragment();

                if (fragment.charFormat().isImageFormat()) {
                    int relativePos = fragment.position() - paragraph.position();

                    if (relativePos >= line.textStart() &&
                        relativePos < line.textStart() + line.textLength()) {

                        QTextImageFormat imgFmt = fragment.charFormat().toImageFormat();

                        // Pull the REAL image from the original UI source!
                        QImage img;
                        if (imgFmt.name().startsWith("internal://")) {
                            auto& images = Main::ref().ui()->textEdit->internalImages();
                            if (images.contains(imgFmt.name())) img = images[imgFmt.name()];
                        } else {
                            img = Main::ref().ui()->textEdit->document()->resource(QTextDocument::ImageResource, QUrl(imgFmt.name())).value<QImage>();
                        }

                        if (!img.isNull()) {
                            qreal imageXOffset = line.cursorToX(relativePos);

                            // Because imgFmt is logical, we MUST scale it up here to physically fill the layout hole!
                            qreal physicalImgW = imgFmt.width() * scaleX;
                            qreal physicalImgH = imgFmt.height() * scaleY;

                            // Draw upwards from the baseline using the physical dimensions
                            QRectF targetRect(margins.left() + imageXOffset, at - physicalImgH, physicalImgW, physicalImgH);
                            painter->drawImage(targetRect, img);
                        }
                    }
                }
            }
        }
    }
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

void Printer::page(QPainter* painter,
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
                          std::function<void()> newPage) {
    if (mPrinter == nullptr) return;
    mXFactor = physicalDpiX() / PointsPerInch;
    mYFactor = physicalDpiX() / PointsPerInch;

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
    QTextDocument doc;
    doc.documentLayout()->setPaintDevice(paintdevice());
    doc.setTextWidth(lineWidth);
    for (const auto& id : ids) {
        mId = id;
        Item& item = Main::ref().novel().findItem(id);
        if (!item.hasTag(chapterTag) && !item.hasTag(sceneTag) && !item.hasTag(coverTag)) continue;
        if (item.hasTag(chapterTag)) {
            if (!startingPage && mPageNo != 1) {
                page(painter, newPage, true);
                ++mPageNo;
                at = margins.top();
                startingPage = true;
            }
        }

        auto paragraphs = createParagraphs(&doc, item, scaleX, scaleY, lineWidth, pageHeight);
        printParagraphs(painter, pageSize, newPage, margins, at, startingPage, paragraphs);

        if (item.hasTag(coverTag)) {
            page(painter, newPage, false);
            at = margins.top();
            startingPage = true;
            continue;
        }
    }

    if (!startingPage) page(painter, newPage);
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
        QString ret = "";
        for (const auto& tag: tags) {
            if (tag.startsWith(key + ":", Qt::CaseInsensitive)) value = tag.mid(key.length() + 1);
        }
        if (mId == id) break;
    }
    return value;
}

List<QTextBlock> Printer::createParagraphs(QTextDocument* doc, Item& item, qreal scaleX, qreal scaleY, qreal lineWidth, qreal pageHeight) {
    doc->setHtml(item.html());

    qreal logicalLineWidth = lineWidth / scaleX;
    qreal logicalPageHeight = pageHeight / scaleY;

    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
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

                qreal origW = imgFmt.width() > 0 ? imgFmt.width() : img.width();
                qreal origH = imgFmt.height() > 0 ? imgFmt.height() : img.height();

                if (origW > logicalLineWidth) {
                    qreal shrinkRatio = logicalLineWidth / origW;
                    origW = logicalLineWidth;
                    origH = origH * shrinkRatio;
                }

                if (origH > logicalPageHeight) {
                    qreal shrinkRatio = logicalPageHeight / origH;
                    origH = logicalPageHeight;
                    origW = origW * shrinkRatio;
                }
                QImage phantomPixel(1, 1, QImage::Format_ARGB32);
                phantomPixel.fill(Qt::transparent);

                doc->addResource(QTextDocument::ImageResource, QUrl(imgFmt.name()), phantomPixel);

                imgFmt.setWidth(origW);
                imgFmt.setHeight(origH);

                QTextCursor cursor(doc);
                cursor.setPosition(fragment.position());
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                cursor.setCharFormat(imgFmt);
            }
        }
    }

    List<QTextBlock> paragraphs;
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        if (block.isValid()) paragraphs.append(block);
    }
    return paragraphs;
}

const QSizeF Printer::pageSize(QPrinter::Unit unit) const {
    return paperRect(unit).size();
}
