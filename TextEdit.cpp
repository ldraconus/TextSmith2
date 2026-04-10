#include "TextEdit.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QUrl>
#include <QDateTime>

class ReentryGuard {
public:
    ReentryGuard(int& counter): c(counter) { ++c; }
    ~ReentryGuard()                        { --c; }

    bool active() const { return c > 1; } // already inside

private:
    int& c;
};

TextEdit::TextEdit(QWidget* parent)
    : QTextEdit(parent) {
    mIdBase = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    setAcceptRichText(true);
    setAcceptDrops(true);
    // Default margin is fine; you can tweak per your layout
}

static constexpr auto TextSmith2MimeData = "x-TextSmith2-json";

void TextEdit::insertFromMimeData(const QMimeData *source) {
    ReentryGuard guard(mReentry);
    if (guard.active() || mInserting || mResizing) return; // guard against re-entry

    if (!source) return;

    if (source->hasFormat(TextSmith2MimeData)) {
        QByteArray data(source->data(TextSmith2MimeData));
        QJsonDocument doc(QJsonDocument::fromJson(data));
        fromJson(doc);
        return;
    }

    if (source->hasImage()) {
        QImage img = qvariant_cast<QImage>(source->imageData());
        if (!img.isNull()) {
            insertInternalImage(img);
            return;
        }
    }

    if (source->hasHtml()) { // strange, but possible ...
        // 1. Use a lightweight document to parse the HTML structure
        QTextDocument tempDoc;
        tempDoc.setHtml(source->html());

        // 2. Create a cursor for your MAIN editor
        QTextCursor cur = textCursor();
        cur.beginEditBlock(); // Group into one Undo step
        QTextBlockFormat blockFmt = cur.blockFormat();

        // 3. Iterate over the confused HTML mess
        for (auto it = tempDoc.begin(); it != tempDoc.end(); it = it.next()) {
            QTextBlock block = it;

            cur.setBlockFormat(blockFmt);

            for (auto it2 = block.begin(); it2 != block.end(); it2++) {
                QTextFragment frag = it2.fragment();
                if (!frag.isValid()) continue;

                QTextCharFormat dirtyFmt = frag.charFormat();

                // --- 1. CHECK FOR IMAGE ---
                if (dirtyFmt.isImageFormat()) {
                    QTextImageFormat dirtyImg = dirtyFmt.toImageFormat();

                    // Create a pristine image format (strips size, borders, alignment)
                    QTextImageFormat cleanImg;
                    cleanImg.setName(dirtyImg.name()); // Keep ONLY the URL

                    cur.insertImage(cleanImg);
                }
                // --- 2. HANDLE TEXT ---
                else {
                    QTextCharFormat cleanFmt;

                    // Only copy the "Novel" attributes
                    if (dirtyFmt.fontItalic()) cleanFmt.setFontItalic(true);
                    if (dirtyFmt.fontUnderline()) cleanFmt.setFontUnderline(true);
                    if (dirtyFmt.fontWeight() > QFont::Normal) cleanFmt.setFontWeight(QFont::Bold);

                    // Enforce YOUR standard font
                    cleanFmt.setFontFamilies({ fontFamily() });
                    cleanFmt.setFontPointSize(fontPointSize());

                    // Insert the sanitized chunk
                    // Note: Since we are in the 'else', we verify this isn't an image placeholder
                    cur.insertText(frag.text(), cleanFmt);
                }
            }

            if (block.next().isValid()) cur.insertBlock(blockFmt); // <--- CRITICAL: Insert next block with YOUR format
        }

        cur.endEditBlock();
        return;
    }

    if (source->hasText()) {
        insertPlainText(source->text());
        return;
    }

    if (source->hasUrls()) {
        for (const QUrl& url: source->urls()) {
            if (!url.isValid()) continue;
            const QString local = url.toLocalFile();
            QImage img;
            if (!local.isEmpty()) {
                img = QImage(local);
                if (img.isNull()) continue;
                insertInternalImage(img);
                return;
            } else {
                img = QImage(url.path());
                if (img.isNull()) continue;
                insertInternalImage(img);
                return;
            }
        }
    }

    QTextEdit::insertFromMimeData(source); // non-image content
}

QString TextEdit::embedImagesAsBase64(const QString &html) {
    QString result = html;
    QRegularExpression reFile(R"(src=\"(file://[^\"]+)\")");
    QRegularExpression reInternal(R"(src=\"(internal://[^\"]+\"))");
    QRegularExpressionMatchIterator itFile = reFile.globalMatch(html);
    QRegularExpressionMatchIterator intFile = reInternal.globalMatch(html);

    // Collect replacements first to avoid offset shifting
    QList<QPair<QString, QString>> replacements;

    while (itFile.hasNext()) {
        QRegularExpressionMatch match = itFile.next();
        QString fullSrc = match.captured(0);   // src="file:///..."
        QString fileUrl = match.captured(1);   // file:///...

        // Convert file URL to local path
        QString localPath = QUrl(fileUrl).toLocalFile();
        QFile file(localPath);

        if (!file.open(QIODevice::ReadOnly)) continue;

        QByteArray fileData = file.readAll();
        file.close();

        // Determine MIME type from extension
        QString ext = QFileInfo(localPath).suffix().toLower();
        QString mimeType;
        if      (ext == "png")  mimeType = "image/png";
        else if (ext == "jpg" || ext == "jpeg") mimeType = "image/jpeg";
        else if (ext == "gif")  mimeType = "image/gif";
        else if (ext == "webp") mimeType = "image/webp";
        else if (ext == "bmp")  mimeType = "image/bmp";
        else                    mimeType = "image/png"; // fallback

        QString base64 = fileData.toBase64();
        QString dataUri = QString("src=\"data:%1;base64,%2\"")
                              .arg(mimeType, base64);

        replacements.append({fullSrc, dataUri});
    }

    while (intFile.hasNext()) {
        QRegularExpressionMatch match = intFile.next();
        QString fullSrc = match.captured(0);   // src="internal:///..."
        QString intUrl = match.captured(1);   // internal:///...

        // Convert file URL to local path
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);

        // Save as PNG
        mOriginals[intUrl].save(&buffer, "PNG");        QByteArray fileData;
        buffer.close();

        // Determine MIME type from extension
        QString ext = "png";
        QString mimeType = "image/png";

        QString base64 = fileData.toBase64();
        QString dataUri = QString("src=\"data:%1;base64,%2\"")
                              .arg(mimeType, base64);

        replacements.append({fullSrc, dataUri});
    }

    for (const auto &pair : replacements) result.replace(pair.first, pair.second);

    return result;
}

void TextEdit::keyPressEvent(QKeyEvent* key) {
//    if (key->matches(QKeySequence::Copy) ||
//        key->matches(QKeySequence::Cut)) {
//        QTextCursor cursor = textCursor();
//        QTextDocumentFragment fragment = cursor.selection();
//        QString html = fragment.toHtml();
//        QString text = fragment.toPlainText();
//        QString embedded = embedImagesAsBase64(html);
//        QByteArray json = toJson(cursor).toJson().toBase64();
//
//        QMimeData* mimeData = new QMimeData();
//        mimeData->setHtml(embedded);
//        mimeData->setText(text);;
//        mimeData->setData(TextSmith2MimeData, json);
//
//        QApplication::clipboard()->setMimeData(mimeData);
//
//        if (key->matches(QKeySequence::Cut)) textCursor().removeSelectedText();
//
//        return;
//    } else if (key->matches(QKeySequence::Paste)) {
//        QClipboard* clipboard = QApplication::clipboard();
//        auto& mimeData = *clipboard->mimeData();
//        this->canInsertFromMimeData(&mimeData);
//        return;
//    }

    if (mSoundPool) {
        switch (key->key()) {
        case Qt::Key_Escape:
        case Qt::Key_F1:
        case Qt::Key_F2:
        case Qt::Key_F3:
        case Qt::Key_F4:
        case Qt::Key_F5:
        case Qt::Key_F6:
        case Qt::Key_F7:
        case Qt::Key_F8:
        case Qt::Key_F9:
        case Qt::Key_F10:
        case Qt::Key_F11:
        case Qt::Key_F12:
        case Qt::Key_Insert:
        case Qt::Key_Print:
        case Qt::Key_ScrollLock:
        case Qt::Key_Pause:
            break;

        case Qt::Key_Space:
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
        case Qt::Key_Right:
        case Qt::Key_Left:
        case Qt::Key_Tab:
            mSoundPool->play(SoundPool::Sound::SpaceThunk);
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            mSoundPool->play(SoundPool::Sound::ReturnRoll);
            {
                QRect r = cursorRect();
                if (r.left() < r.size().width()) mSoundPool->playDelayed(SoundPool::Sound::ReturnClunk, 80);
            }
            break;

        case Qt::Key_Home:
        case Qt::Key_CapsLock:
            mSoundPool->playDelayed(SoundPool::Sound::ReturnClunk, 80);
            break;

        case Qt::Key_End:
            mSoundPool->play(SoundPool::Sound::SpaceThunk);
            break;

        default:
            if (!key->text().isEmpty() && !key->text().at(0).isSpace() &&
                !(key->modifiers() & (Qt::AltModifier |
                                      Qt::ShiftModifier |
                                      Qt::ControlModifier))) mSoundPool->play(SoundPool::Sound::KeyWhack);
            break;
        }
    }

    if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();
        int oldBlockNumber = cursor.blockNumber();
        QTextBlockFormat currentFmt = cursor.blockFormat();

        if (key->modifiers() != Qt::NoModifier) {
            QKeyEvent* myKey = new QKeyEvent(QEvent::KeyPress, key->key(), Qt::NoModifier);
            QTextEdit::keyPressEvent(myKey);
            delete myKey;
        } else QTextEdit::keyPressEvent(key);

        QTextCursor newCursor = textCursor();
        int newBlockNumber = newCursor.blockNumber();

        if (newBlockNumber == oldBlockNumber) {
            newCursor.insertBlock(currentFmt);
            setTextCursor(newCursor);
        }

        if (newCursor.blockFormat().textIndent() != currentFmt.textIndent()) {
            QTextBlockFormat fixedFmt = newCursor.blockFormat();
            fixedFmt.setTextIndent(currentFmt.textIndent());
            fixedFmt.setBottomMargin(currentFmt.bottomMargin());
            newCursor.setBlockFormat(currentFmt);
            setTextCursor(newCursor);
        }

        return;
    } else if (key->key() == Qt::Key_Backspace) {
        auto cursor = textCursor();
        if (cursor.position() == 0) return;
        if (cursor.atBlockStart() && cursor.selectedText().length() == 0) {
            cursor.setPosition(cursor.position() - 1);
            setTextCursor(cursor);
            QKeyEvent* myKey = new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
            QTextEdit::keyPressEvent(myKey);
            delete myKey;
        } else QTextEdit::keyPressEvent(key);
    } else QTextEdit::keyPressEvent(key);

    // Margin wrap detection
    if (mSoundPool) {
        QRect r = cursorRect();
        if (r.right() > mWrapMarginPx && r.left() <= mWrapMarginPx) mSoundPool->play(SoundPool::Sound::MarginDing);
    }

}

void TextEdit::insertLocalImage(const QString& localFilePath) {
    QImageReader reader(localFilePath);
    QImage img = reader.read();
    if (img.isNull()) return;

    insertInternalImage(img);
}

void TextEdit::insertInternalImage(const QImage& image) {
    if (image.isNull()) return;

    mInserting = true;
    const bool prevAcceptDrops = acceptDrops();
    setAcceptDrops(false);
    viewport()->setUpdatesEnabled(false);

    QSignalBlocker blockDoc(document());
    QSignalBlocker blockLay(document()->documentLayout());

    // Create an internal URL
    const QUrl url = makeInternalUrl();

    // Store original for fidelity
    internalImages().insert(url, image);

    // Initial display size: clamp to viewport width
    const int maxW = contentMaxWidth();
    const double scale = maxW > 0 ? double(maxW) / double(image.width()) : 1.0;
    const int displayW = (maxW > 0 && image.width() > maxW) ? maxW : image.width();
    const int displayH = (maxW > 0 && image.width() > maxW)
                             ? int(image.height() * scale)
                             : image.height();

    // Register original once as resource (Qt will paint scaled per format)
    document()->addResource(QTextDocument::ImageResource, url, image);

    // Insert with display size
    QTextCursor cursor = textCursor();

    // Ensure we’re at a fresh block
    cursor.insertBlock();

    // Clear indentation/margins
    QTextBlockFormat blockFmt;
    blockFmt.setLeftMargin(0);
    blockFmt.setIndent(0);
    cursor.setBlockFormat(blockFmt);

    QTextImageFormat fmt;
    fmt.setName(url.toString());
    fmt.setWidth(displayW);
    fmt.setHeight(displayH);
    cursor.insertImage(fmt);

    // Release isolation
    viewport()->setUpdatesEnabled(true);
    setAcceptDrops(prevAcceptDrops);
    mInserting = false;

    scheduleResize();
}

void TextEdit::insertExternalUrlImage(const QUrl& url) {
    if (!url.isValid()) return;

    const bool prevAcceptDrops = acceptDrops();
    setAcceptDrops(false);
    viewport()->setUpdatesEnabled(false);

    QSignalBlocker blockDoc(document());
    QSignalBlocker blockLay(document()->documentLayout());

    // Keep URL as-is, no internal storage; just insert with a conservative width
    externalImageUrls().insert(url);

    // We don’t fetch or addResource here; Qt will request when painting via the URL.
    // Use a placeholder size until first paint; then resize pass will clamp to viewport.
    const int maxW = contentMaxWidth();
    const int displayW = maxW > 0 ? maxW : 512; // fallback
    const int displayH = displayW; // square placeholder if unknown

    QTextCursor cursor = textCursor();

    // Ensure we’re at a fresh block
    cursor.insertBlock();

    // Clear indentation/margins
    QTextBlockFormat blockFmt;
    blockFmt.setLeftMargin(0);
    blockFmt.setIndent(0);
    cursor.setBlockFormat(blockFmt);

    QTextImageFormat fmt;
    fmt.setName(url.toString());
    fmt.setWidth(displayW);
    fmt.setHeight(displayH);
    cursor.insertImage(fmt);

    viewport()->setUpdatesEnabled(true);
    setAcceptDrops(prevAcceptDrops);
    mInserting = false;

    scheduleResize();
}

int TextEdit::contentMaxWidth() const {
    // Visible width minus margins
    const int w = viewport()->width();
    const int margin = int(document()->documentMargin());
    return w > 0 ? w - margin * 2 : w;
}

void TextEdit::resizeEvent(QResizeEvent *event) {
    QTextEdit::resizeEvent(event);
    const int w = contentMaxWidth();
    if (w <= 0) return;
    if (w == mLastViewportWidth) return;
    mLastViewportWidth = w;
    scheduleResize();
}

void TextEdit::scheduleResize() {
    QTimer::singleShot(0, this, [this]{ resizeImagesToFit(); });
}

static constexpr auto Alignment = "Alignment";
static constexpr auto Blocks =    "Blocks";
static constexpr auto Bold =      "Bold";
static constexpr auto Default =   "Default";
static constexpr auto FontName =  "FontName";
static constexpr auto FontSize =  "FontSize";
static constexpr auto Format =    "Format";
static constexpr auto Fragments = "Fragments";
static constexpr auto Height =    "Height";
static constexpr auto Image =     "Image";
static constexpr auto Images =    "Images";
static constexpr auto Indent =    "Indent";
static constexpr auto Italic =    "Italic";
static constexpr auto Name =      "Name";
static constexpr auto Text =      "Text";
static constexpr auto Underline = "Underline";
static constexpr auto Width =     "Width";

QTextBlockFormat TextEdit::fromTextBlockFormatObject(QJsonObject &obj) {
    QTextBlockFormat format;
    QFontMetrics metrics(font());
    int lineHeight = metrics.lineSpacing();
    int paraIndent = 4 * metrics.averageCharWidth();
    format.setBottomMargin(lineHeight);
    format.setTextIndent(paraIndent);
    if (obj.contains(Alignment) && obj[Alignment].isDouble()) {
        auto alignment = obj[Alignment].toInt();
        Qt::Alignment align = Qt::AlignLeft;
        if (alignment == 1) align = Qt::AlignLeft;
        if (alignment == 3) align = Qt::AlignRight;
        if (alignment == 4) align = Qt::AlignJustify;
        if (alignment == 2) align = Qt::AlignHCenter;
        format.setAlignment(align);
    }
    if (obj.contains(Indent) && obj[Indent].isDouble()) format.setIndent(obj[Indent].toInt());
    return format;
}

QTextCharFormat TextEdit::fromTextCharFormatObject(QJsonObject &obj) {
    QTextCharFormat format;
    format.setFontFamilies(font().families());
    format.setFontPointSize(font().pointSize());
    if (obj.contains(Bold) &&      obj[Bold].isBool() &&      obj[Bold].toBool())      format.setFontWeight(QFont::Bold);
    if (obj.contains(Italic) &&    obj[Italic].isBool() &&    obj[Italic].toBool())    format.setFontItalic(obj[Italic].toBool());
    if (obj.contains(Underline) && obj[Underline].isBool() && obj[Underline].toBool()) format.setFontUnderline(obj[Underline].toBool());
    return format;
}

void TextEdit::toTextBlockFormat(QJsonObject& obj, const QTextBlockFormat format) const {
    auto align = format.alignment();
    qlonglong alignment = 0;
    if (align & Qt::AlignLeft) alignment = 1;
    if (align & Qt::AlignRight) alignment = 3;
    if (align & Qt::AlignJustify) alignment = 4;
    if (align & Qt::AlignHCenter) alignment = 2;
    obj[Alignment] = alignment;
    obj[Indent] =    int(format.indent());
}

void TextEdit::toTextCharFormat(QJsonObject& obj, const QTextCharFormat format) const {
    if (format.fontWeight() >= QFont::Bold) obj[Bold] =      true;
    if (format.fontItalic())                obj[Italic] =    true;
    if (format.fontUnderline())             obj[Underline] = true;
}

static QTextBlock::iterator findFragment(QTextBlock& blk, int pos) {
    for (auto frag = blk.begin(); frag != blk.end(); ++frag) {
        auto fragment = frag.fragment();
        if (fragment.contains(pos)) return frag;
    }
    return blk.end();
}

void TextEdit::fromJson(const QJsonDocument& doc) {
    QTextCursor cursor = textCursor();
    if (!doc.isObject()) return;
    QJsonObject top = doc.object();
    if (!top.contains(Images) || !top[Images].isArray()) return;
    if (!top.contains(Blocks) || !top[Blocks].isArray()) return;
    cursor.beginEditBlock();
    QJsonArray images = top[Images].toArray();
    QMap<QUrl, QUrl> in2Internal;
    for (auto&& image: images) {
        if (!image.isObject()) continue;
        QJsonObject img = image.toObject();
        if (!img.contains(Name) || !img[Name].isString()) continue;
        QString name = img[Name].toString();
        if (name.isEmpty()) continue;
        QUrl in(name);
        if (!img.contains(Image) || !img[Image].isString()) continue;
        QString str = img[Image].toString();
        if (str.isEmpty()) continue;
        QByteArray data = QByteArray::fromBase64(str.toUtf8());
        if (data.isEmpty()) continue;
        QImage pic;
        pic.loadFromData(data);
        if (pic.isNull()) continue;
        if (!mOriginals.contains(in)) {
            const QUrl url = makeInternalUrl();
            in2Internal[in] = url;
            addInternalImage(url, pic, true);
        } else in2Internal[in] = in;
    }

    QJsonArray paragraphs = top[Blocks].toArray();;
    bool first = true;
    for (auto&& paragraph: paragraphs) {
        if (!paragraph.isObject()) return;
        QJsonObject blk = paragraph.toObject();
        QTextBlockFormat format = fromTextBlockFormatObject(blk);
        QTextCharFormat charFormat = fromTextCharFormatObject(blk);
        if (first) {
            first = false;
            cursor.setBlockFormat(format);
            cursor.setBlockCharFormat(charFormat);
        } else cursor.insertBlock(format, charFormat);
        if (!blk.contains(Fragments) || !blk[Fragments].isArray()) continue;
        QJsonArray fragments = blk[Fragments].toArray();
        for (auto&& fragment: fragments) {
            if (fragment.isObject()) {
                QJsonObject frag = fragment.toObject();
                if (frag.contains(Image)) {
                    if (!frag[Image].isString()) continue;
                    QString id = frag[Image].toString();
                    QUrl urlId(id);
                    if (!in2Internal.contains(urlId)) continue;
                    QTextImageFormat imgFmt;
                    imgFmt.setName(in2Internal[urlId].toString());
                    int height = -1;
                    if (frag.contains(Height) && frag[Height].isDouble()) height = frag[Height].toInt();
                    int width = -1;
                    if (frag.contains(Width) && frag[Width].isDouble()) width = frag[Width].toInt();
                    if (height != -1) imgFmt.setHeight(height);
                    if (width != -1) imgFmt.setWidth(width);
                    cursor.insertImage(imgFmt);
                } else {
                    if (!frag.contains(Text) || !frag[Text].isString()) continue;
                    QString text = frag[Text].toString();
                    charFormat = fromTextCharFormatObject(frag);
                    cursor.insertText(text, charFormat);
                }
            }
        }
    }

    cursor.endEditBlock();
}

QJsonDocument TextEdit::toJson(const QTextCursor& selection) const {
    QJsonDocument doc;
    QJsonObject data;

    auto* document = selection.document();
    int pos = selection.selectionStart();
    int endPos = selection.selectionEnd();
    QTextBlock startBlock = document->findBlock(pos);
    QTextBlock endBlock = document->findBlock(endPos);
    QTextBlock block = startBlock;
    QJsonArray slctBlks;
    QTextBlock::iterator fragment = findFragment(startBlock, pos);
    QStringList images;
    for (; ; ) {
        QJsonObject blk;
        toTextBlockFormat(blk, block.blockFormat());
        toTextCharFormat(blk, block.charFormat());
        QJsonArray fragments;
        while (fragment != block.end()) {
            QTextFragment frag = fragment.fragment();
            QJsonObject frgmnt;
            if (frag.charFormat().isImageFormat()) {
                QTextImageFormat imgFmt = frag.charFormat().toImageFormat();
                QString name = imgFmt.name();
                frgmnt[Image] = name;
                images.append(name);
                frgmnt[Width] = imgFmt.width();
                frgmnt[Height] = imgFmt.height();
            } else {
                QString text = frag.text();
                if (frag.contains(endPos)) text = text.left(endPos - frag.position());
                if (frag.contains(pos)) text = text.mid(pos - frag.position());
                frgmnt[Text] = text;
                toTextCharFormat(frgmnt, frag.charFormat());
            }
            fragments.append(frgmnt);
            if (frag.contains(endPos)) break;
            ++fragment;
        }
        blk[Fragments] = fragments;
        slctBlks.append(blk);
        if (block.blockNumber() == endBlock.blockNumber()) break;
        block = block.next();
        fragment = block.begin();
        if (fragment != block.end()) pos = fragment.fragment().position();
        else pos = 0;
    }
    data[Blocks] = slctBlks;

    QJsonArray slctImgs;
    for (auto&& name: images) {
        if (name.startsWith("internal://")) {
            QJsonObject img;
            img[Name] = name;
            QImage image = mOriginals[name];
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");
            buffer.close();
            img[Image] = QString(bytes.toBase64());
            slctImgs.append(img);
        } else slctImgs.append(name);
    }
    data[Images] = slctImgs;

    doc.setObject(data);
    return doc;
}

void TextEdit::resizeImagesToFit() {
    if (mResizing || mInserting) return;
    mResizing = true;

    const int maxW = contentMaxWidth();
    if (maxW <= 0) {
        mResizing = false;
        return;
    }

    setWrapMargin();

    // Phase 1: collect using block iteration (cursor-free scan)
    struct ImgFrag { int s, e; QUrl url; QSize orig; };
    QVector<ImgFrag> frags;

    for (QTextBlock b = document()->begin(); b.isValid(); b = b.next()) {
        for (QTextBlock::iterator it = b.begin(); !(it.atEnd()); ++it) {
            const QTextFragment f = it.fragment();
            if (!f.isValid()) continue;

            const QTextCharFormat cf = f.charFormat();
            if (!cf.isImageFormat()) continue;

            const QTextImageFormat imf = cf.toImageFormat();
            ImgFrag g;
            g.s = f.position();
            g.e = f.position() + f.length();
            g.url = QUrl(imf.name());

            const auto itOrig = internalImages().find(g.url);
            g.orig = (itOrig != internalImages().end()) ? itOrig.value().size() : QSize();

            frags.push_back(g);
        }
    }

    if (frags.isEmpty()) {
        mResizing = false;
        return;
    }

    // Phase 2: apply format-only updates in a single edit block
    const bool prevUndo = document()->isUndoRedoEnabled();
    document()->setUndoRedoEnabled(false);

    QTextCursor editCursor(document());
    editCursor.beginEditBlock();  // group changes; avoids thrash

    const double eps = 0.5;

    for (const ImgFrag& g : frags) {
        QTextCursor rc(document());
        rc.setPosition(g.s);
        rc.setPosition(g.e, QTextCursor::KeepAnchor);

        QTextCharFormat cf = rc.charFormat();
        if (!cf.isImageFormat()) continue;

        QTextImageFormat imf = cf.toImageFormat();
        if (std::abs(imf.width() - maxW) < eps) continue;

        int newW = maxW;
        int newH;

        if (g.orig.isValid() && g.orig.width() > 0) {
            if (newW > g.orig.width()) {
                newW = g.orig.width();
                newH = g.orig.height();
            } else {
                const double s = double(maxW) / double(g.orig.width());
                newH = int(double(g.orig.height()) * s);
            }
            QTextBlockFormat blockFmt;
            blockFmt.setLeftMargin(0);
            blockFmt.setIndent(0);
            rc.setBlockFormat(blockFmt);
        } else {
            newW = imf.width();
            newH = imf.height();
        }

        imf.setWidth(newW);
        imf.setHeight(newH);
        rc.setCharFormat(imf);
    }

    editCursor.endEditBlock();
    document()->setUndoRedoEnabled(prevUndo);

    mResizing = false;
}

QUrl TextEdit::makeInternalUrl() {
    const quint64 id = mIdBase++;
    return QUrl(QString("internal://image_%1").arg(id));
}

QByteArray TextEdit::imageToPngBytes(const QImage& img) {
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return bytes;
}

QJsonArray TextEdit::serializeExternalImagesToJson() {
    QJsonArray arr;
    for (auto& url: externalImageUrls()) arr.append(url.toString());
    return arr;
}

QJsonObject TextEdit::serializeInteralImageToJson(const QUrl& url, const QImage& img) {
    const QByteArray png = imageToPngBytes(img);
    QJsonObject obj;
    obj["url"] = url.toString();
    obj["mime"] = "image/png";
    obj["data"] = QString::fromLatin1(png.toBase64());
    obj["width"] = img.width();
    obj["height"] = img.height();
    return obj;
}

QJsonArray TextEdit::serializeInternalImagesToJson() {
    QJsonArray arr;
    for (auto it = internalImages().begin(); it != internalImages().end(); ++it) {
        const QUrl url = it.key();
        const QImage img = it.value();

        arr.push_back(serializeInteralImageToJson(url, img));
    }
    return arr;
}

void TextEdit::setWrapMargin() {
    QFontMetrics metrics(document()->defaultFont());
    mWrapMarginPx = contentMaxWidth() - metrics.averageCharWidth() * 8;
}

void TextEdit::addInternalImage(const QUrl& url, const QImage& image, bool add) {
    if (url.isEmpty() || image.isNull()) return;

    internalImages().insert(url, image);

    if (add) document()->addResource(QTextDocument::ImageResource, url, image);
}

void TextEdit::clearInternalImages() {
    internalImages().clear();
    externalImageUrls().clear();
}

void TextEdit::registerInternalImages() {
    auto images = internalImages();
    auto keys = images.keys();
    for (auto i = 0; i < keys.size(); ++i) {
        auto& url = keys[i];
        auto& image = images[url];
        document()->addResource(QTextDocument::ImageResource, url, image);
    }
}

void TextEdit::removeInternalImage(const QUrl& url) {
    internalImages().remove(url);
}

bool TextEdit::canInsertFromMimeData(const QMimeData *source) const {
    if (source && (source->hasFormat("x-TextSmith2-json"))) return true;
    if (source && (source->hasHtml() || source->hasText())) return true;
    if (source && (source->hasImage() || source->hasUrls())) return true;
    return QTextEdit::canInsertFromMimeData(source);
}

QMimeData* TextEdit::createMimeDataFromSelection() const {
    QMimeData* mime = new QMimeData();

    QJsonDocument json = toJson(textCursor());
    QString html = textCursor().selection().toHtml();
    QString text = textCursor().selection().toPlainText();

    if (!json.isEmpty()) mime->setData("x-TextSmith2-json", json.toJson());

    if (!html.isEmpty()) mime->setHtml(html);

    if (!text.isEmpty()) mime->setText(text);

    return mime;
}

// Own drag/drop pipeline (never invoke base for images/urls)
void TextEdit::dragEnterEvent(QDragEnterEvent *de) {
    const QMimeData* m = de->mimeData();
    if (m->hasImage() || m->hasUrls()) de->acceptProposedAction();
    else QTextEdit::dragEnterEvent(de);
}

void TextEdit::dropEvent(QDropEvent* de) {
    const QMimeData* m = de->mimeData();
    if (m->hasText() || m->hasHtml() || m->hasImage() || m->hasUrls()) {
        insertFromMimeData(m);
        de->acceptProposedAction();
    } else QTextEdit::dropEvent(de);
}



