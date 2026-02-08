#include "TextEdit.h"
#include <QAbstractTextDocumentLayout>
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

void TextEdit::insertFromMimeData(const QMimeData *source) {
    ReentryGuard guard(mReentry);
    if (guard.active() || mInserting || mResizing) return; // guard against re-entry

    if (!source) return;

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
        QTextCursor cur = this->textCursor();
        cur.beginEditBlock(); // Group into one Undo step

        // 3. Iterate over the confused HTML mess
        for (auto it = tempDoc.begin(); it != tempDoc.end(); it = it.next()) {
            QTextBlock block = it;

            // Iterate over text fragments (chunks of text with same format)
            for (auto it2 = block.begin(); it2 != block.end(); it2++) {
                QTextFragment frag = it2.fragment();
                if (!frag.isValid()) continue;

                // 4. THE MAGIC: Extract only what we care about
                QTextCharFormat dirtyFmt = frag.charFormat();
                QTextCharFormat cleanFmt;

                // Only copy the "Novel" attributes
                if (dirtyFmt.fontItalic()) cleanFmt.setFontItalic(true);
                if (dirtyFmt.fontWeight() > QFont::Normal) cleanFmt.setFontWeight(QFont::Bold);

                // Enforce YOUR standard font (e.g. Times, 12pt)
                cleanFmt.setFontFamilies({ this->fontFamily() });
                cleanFmt.setFontPointSize(this->fontPointSize());

                // Insert the sanitized chunk
                cur.insertText(frag.text(), cleanFmt);
            }

            // Insert a block break (paragraph) unless it's the last one
            if (block.next().isValid()) cur.insertBlock();
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

void TextEdit::keyPressEvent(QKeyEvent* key) {
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
                if (r.left() > r.size().width()) mSoundPool->playDelayed(SoundPool::Sound::ReturnClunk, 80);
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
                                      Qt::ControlModifier))) mSoundPool->play(SoundPool::Sound::KeyWhack);
            break;
        }
    }

    // Let QTextEdit do its normal behavior
    QTextEdit::keyPressEvent(key);

    // Margin wrap detection
    if (mSoundPool) {
        QRect r = cursorRect();
        if (r.right() >= mWrapMarginPx) mSoundPool->play(SoundPool::Sound::MarginDing);
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

QJsonArray TextEdit::serializeInternalImagesToJson() {
    QJsonArray arr;
    for (auto it = internalImages().begin(); it != internalImages().end(); ++it) {
        const QUrl url = it.key();
        const QImage img = it.value();
        const QByteArray png = imageToPngBytes(img);

        QJsonObject obj;
        obj["url"] = url.toString();
        obj["mime"] = "image/png";
        obj["data"] = QString::fromLatin1(png.toBase64());
        obj["width"] = img.width();
        obj["height"] = img.height();
        arr.push_back(obj);
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
    if (source && (source->hasHtml() || source->hasText())) return true;
    if (source && (source->hasImage() || source->hasUrls())) return true;
    return QTextEdit::canInsertFromMimeData(source);
}

QMimeData* TextEdit::createMimeDataFromSelection() const {
    QMimeData* mime = new QMimeData();

    QString html = textCursor().selection().toHtml();
    QString text = textCursor().selection().toPlainText();

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



