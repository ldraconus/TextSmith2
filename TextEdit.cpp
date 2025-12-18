#include "TextEdit.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextImageFormat>
#include <QUrl>
#include <QDateTime>

TextEdit::TextEdit(QWidget* parent)
    : QTextEdit(parent) {
    mIdBase = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    setAcceptRichText(true);
    setAcceptDrops(true);
    setUndoRedoEnabled(true);
    // Default margin is fine; you can tweak per your layout
}

bool TextEdit::canInsertFromMimeData(const QMimeData *source) const {
    if (!source) return false;
    if (source->hasImage()) return true;
    if (source->hasUrls()) return true;
    return QTextEdit::canInsertFromMimeData(source);
}

void TextEdit::insertFromMimeData(const QMimeData *source) {
    if (!source) return;

    // Pasted image (single image object)
    if (source->hasImage()) {
        const QImage img = qvariant_cast<QImage>(source->imageData());
        if (!img.isNull()) {
            insertInternalImage(img);
            return;
        }
    }

    // Dropped URLs
    if (source->hasUrls()) {
        for (const QUrl& url : source->urls()) {
            if (!url.isValid()) continue;

            // Treat http/https as external
            const QString scheme = url.scheme().toLower();
            if (scheme == "http" || scheme == "https") {
                insertExternalUrlImage(url);
                continue;
            }

            // Local file
            const QString local = url.toLocalFile();
            if (!local.isEmpty()) {
                insertLocalImage(local);
                continue;
            }

            // Fallback: attempt local path via .path(), if present
            if (!url.path().isEmpty()) {
                insertLocalImage(url.path());
                continue;
            }
        }
        return;
    }

    // Fallback to default rich text insertion
    QTextEdit::insertFromMimeData(source);
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

    // Create an internal URL
    const QUrl url = makeInternalUrl();

    // Store original for fidelity
    mOriginals.insert(url, image);

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
    QTextImageFormat fmt;
    fmt.setName(url.toString());
    fmt.setWidth(displayW);
    fmt.setHeight(displayH);
    textCursor().insertImage(fmt);

    mInserting = false;

    scheduleResize();
}

void TextEdit::insertExternalUrlImage(const QUrl& url) {
    if (!url.isValid()) return;

    // Keep URL as-is, no internal storage; just insert with a conservative width
    mExternalUrls.insert(url);

    // We don’t fetch or addResource here; Qt will request when painting via the URL.
    // Use a placeholder size until first paint; then resize pass will clamp to viewport.
    const int maxW = contentMaxWidth();
    const int displayW = maxW > 0 ? maxW : 512; // fallback
    const int displayH = displayW; // square placeholder if unknown

    QTextImageFormat fmt;
    fmt.setName(url.toString());
    fmt.setWidth(displayW);
    fmt.setHeight(displayH);
    textCursor().insertImage(fmt);

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
    // Defer to next event loop tick to avoid synchronous relayout loops
    QTimer::singleShot(0, this, [this]{ resizeImagesToFit(); });
}

void TextEdit::resizeImagesToFit() {
    if (mResizing || mInserting) return;
    mResizing = true;

    const int maxW = contentMaxWidth();
    if (maxW <= 0) { mResizing = false; return; }

    // 1) Collect image fragments first (positions + URLs), then update.
    struct ImgFrag {
        int posStart;
        int posEnd;
        QUrl url;
        QSize origSize; // for internal images; empty for external
    };
    QVector<ImgFrag> frags;

    QTextCursor c(document());
    c.movePosition(QTextCursor::Start);

    while (!c.atEnd()) {
        const QTextBlock block = c.block();
        for (QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;

            const QTextCharFormat cf = frag.charFormat();
            if (!cf.isImageFormat()) continue;

            const QTextImageFormat imgFmt = cf.toImageFormat();
            const QUrl url(imgFmt.name());

            ImgFrag f;
            f.posStart = frag.position();
            f.posEnd   = frag.position() + frag.length();
            f.url      = url;

            // Internal originals provide exact aspect ratio
            const auto itOrig = mOriginals.find(url);
            if (itOrig != mOriginals.end()) {
                f.origSize = itOrig.value().size();
            } else {
                f.origSize = QSize(); // unknown; keep current aspect
            }
            frags.push_back(f);
        }
        c.movePosition(QTextCursor::NextBlock);
    }

    // 2) Apply updates without querying resources or re-inserting.
    // Disable undo for these non-user-initiated layout adjustments.
    const bool prevUndo = document()->isUndoRedoEnabled();
    document()->setUndoRedoEnabled(false);

    const double eps = 0.5;

    for (const ImgFrag& f : frags) {
        QTextCursor rc(document());
        rc.setPosition(f.posStart);
        rc.setPosition(f.posEnd, QTextCursor::KeepAnchor);

        QTextCharFormat cf = rc.charFormat();
        if (!cf.isImageFormat()) continue;
        QTextImageFormat imgFmt = cf.toImageFormat();

        // Skip if width is already effectively maxW
        if (std::abs(imgFmt.width() - maxW) < eps) continue;

        int newW = maxW;
        int newH = int(imgFmt.height()); // fallback

        if (f.origSize.isValid() && f.origSize.width() > 0) {
            const double scale = double(maxW) / double(f.origSize.width());
            newH = int(f.origSize.height() * scale);
        } else {
            // Unknown original size (external URL): keep current aspect
            const double aspect = (imgFmt.width() > 0) ? (imgFmt.height() / imgFmt.width()) : 1.0;
            newH = int(double(newW) * aspect);
        }

        imgFmt.setWidth(newW);
        imgFmt.setHeight(newH);
        rc.setCharFormat(imgFmt);
    }

    document()->setUndoRedoEnabled(prevUndo);
    mResizing = false;
}

QUrl TextEdit::makeInternalUrl() {
    // Unique internal URL for in-document images
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

QString TextEdit::serializeInternalImagesToJson() const {
    QJsonArray arr;
    for (auto it = mOriginals.constBegin(); it != mOriginals.constEnd(); ++it) {
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
    QJsonDocument doc(arr);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}
