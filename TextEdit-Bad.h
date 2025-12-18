#ifndef TEXTEDIT-OLD_H
#define TEXTEDIT-OLD_H

#include <QBuffer>
#include <QFileInfo>
#include <QKeyEvent>
#include <QImageReader>
#include <QImageWriter>
#include <QTextBlock>
#include <QMap>
#include <QMimeData>
#include <QTextEdit>
#include <QTimer>

#include "Json5.h"

class TextEdit : public QTextEdit {
private:
    QMap<QUrl, QImage> Images;
    bool NewImages = false;
    const bool Save = true;
    QString mDir = ".";
    static size_t mBase;
    bool mAllowTextPaste = false;
    bool mInserting = false;
    int mLastViewportWidth = -1;
    bool mResizing = false;

    void clearSaved() {
        auto keys = Images.keys();
        for (auto i = 0; i < keys.size(); ++i) {
            QUrl url = keys[i];
            document()->addResource(QTextDocument::ImageResource, url, QVariant());
        }
        Images.clear();
        mBase = 0;
    }

    QImage loadImage(Json5Object& obj, const QUrl& url) {
        QByteArray data = QByteArray::fromHex(obj[url.toString()].toString().toUtf8());
        QImage img;
        img.loadFromData(data);
        return img;
    }

    QImage loadImage(const QUrl& url) {
        QString path = url.adjusted(QUrl::RemoveFilename).path();
        if (path.isEmpty()) path = mDir;
        QString filename = url.fileName();
        auto type = "png";
        if (filename.right(4).toLower() == ".gif") type = "gif";
        else if (filename.right(4).toLower() == ".jpg") type = "jpg";
        else if (filename.right(4).toLower() == ".jpeg") type = "jpg";
        else filename += QString(".") + type;
        QImageReader reader(url.toString(), type);
        QImage img = reader.read();
        return img;
    }

    void saveImage(Json5Object& obj, const QUrl& url, const QImage& img) {
        QString filename = url.fileName();
        QStringList parts { filename.split(".") };
        auto ext = "png";
        if (parts.size() > 1) {
            ext = parts.takeLast().toUtf8();
            filename = parts.join(".");
            QString test(ext);
            if (test.toLower() == "png") ext = "png";
            else if (test.toLower() == "jpg") ext = "jpg";
            else if (test.toLower() == "gif") ext = "gif";
        }
        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, ext);
        obj[url.toString()] = QString().fromStdString(data.toHex().toStdString());
    }

    void dropImage(const QString& file, const QImage& image, bool save = false) {
        if (image.isNull() || mInserting) return;
        mInserting = true;

        QUrl url(file);
        if (save) {
            Images[url] = image;
            NewImages = true;
        }
        document()->addResource(QTextDocument::ImageResource, url.toString(), image);

        QTextImageFormat format;
        format.setName(file);
        format.setWidth(image.width());
        format.setHeight(image.height());
        textCursor().insertImage(format);
        mInserting = false;

        QTimer::singleShot(0, this, [this] { resizeImagesToFit(); });
    }

    void dropTextFile(const QUrl& url) {
        QString path = url.adjusted(QUrl::RemoveFilename).path();
        if (path.isEmpty()) path = mDir;
        else if (path.right(1) != "/") path += "/";
        QFile file(path + url.fileName());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString str = file.readAll();
            if (str.contains("<html")) textCursor().insertHtml(str);
            textCursor().insertText(file.readAll());
        }
    }

    void dropImage(const QUrl& url, const QImage& image, bool save = false) {
        if (image.isNull() || mInserting) return;
        mInserting = true;

        if (save) {
            Images[url] = image;
            NewImages = true;
        }
        document()->addResource(QTextDocument::ImageResource, url, image);

        QTextImageFormat format;
        format.setName(url.toString());
        format.setWidth(image.width());
        format.setHeight(image.height());
        textCursor().insertImage(format);
        mInserting = false;

        QTimer::singleShot(0, this, [this] { resizeImagesToFit(); });
    }

    void resizeImagesToFit() {
        if (mResizing || mInserting) return;
        mResizing = true;

        QTextDocument* doc = document();
        int maxWidth = viewport()->width() - doc->documentMargin() * 2;
        if (maxWidth <= 0) {
            mResizing = false;
            return;
        }

        const bool prevUndo = doc->isUndoRedoEnabled();
        doc->setUndoRedoEnabled(false);

        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::Start);

        const double eps = 0.5;
        while (!cursor.atEnd()) {
            QTextBlock block = cursor.block();
            for (QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
                QTextFragment frag = it.fragment();
                if (!frag.isValid()) continue;

                QTextCharFormat cf = frag.charFormat();
                if (!cf.isImageFormat()) continue;

                QTextImageFormat imgFmt = cf.toImageFormat();
                QUrl url(imgFmt.name());

                if (std::abs(imgFmt.width() - maxWidth) < eps) continue;

                const QImage orig = Images.value(url.toString());

                if (orig.isNull()) continue;

                const double scale = double(maxWidth) / double(orig.width());
                const int newW = maxWidth;
                const int newH = int(orig.height() * scale);

                // Update format dimensions
                imgFmt.setWidth(newW);
                imgFmt.setHeight(newH);

                QTextCursor rc(doc);
                rc.setPosition(frag.position());
                rc.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
                rc.setCharFormat(imgFmt);
            }
            cursor.movePosition(QTextCursor::NextBlock);
        }

        doc->setUndoRedoEnabled(prevUndo);
        mResizing = false;
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QTextEdit::resizeEvent(event);
        const int w = viewport()->width() - document()->documentMargin() * 2;
        if (w <= 0) return;
        if (w == mLastViewportWidth) return;

        mLastViewportWidth = w;
        QTimer::singleShot(0, this, [this] { resizeImagesToFit(); });
    }

public:
    TextEdit(QWidget* parent = nullptr): TextEdit(".", parent) { }
    TextEdit(QString dir = ".", QWidget* parent = nullptr): QTextEdit(parent) {
        mDir = dir;
        document()->setResourceProvider([this](const QUrl& url) -> QVariant { return loadImage(url); });
        if (mBase == 0) mBase = time(nullptr);
    }

    void insertFromMimeData(const QMimeData* source) override {
        if (source->hasImage()) {
            QUrl url(QString("internal://image_%1").arg(mBase++));
            dropImage(url, qvariant_cast<QImage>(source->imageData()), Save);
        } else if (source->hasUrls()) {
            foreach (QUrl url, source->urls()) {
                QString fileName = url.path();
                QFileInfo info(fileName);
                if (QImageReader::supportedImageFormats().contains(info.suffix().toLower().toLatin1())) {
                    QString filePath = info.filePath();
                    if (filePath.startsWith("/")) filePath = filePath.mid(1);
                    QImage img(filePath);
                    dropImage(url, img);
                } else dropTextFile(url);
            }
        } else if (mAllowTextPaste) QTextEdit::insertFromMimeData(source);
    }

    void saveImages(Json5Object& obj) {
        for (auto x = Images.begin(); x != Images.end(); ++x) saveImage(obj, x.key(), x.value());
        NewImages = false;
    }

    void addImage(Json5Object& obj, const QUrl& url)                   { loadImage(obj, url); }
    void clearImages()                                                 { clearSaved(); }
    bool canInsertFromMimeData(const QMimeData* source) const override { return source->hasImage() || source->hasUrls() || QTextEdit::canInsertFromMimeData(source); }
    bool hasNewImages()                                                { return NewImages; }
    void allowTextPaste()                                              { mAllowTextPaste = true; }
    void denyTextPaste()                                               { mAllowTextPaste = false; }
    void insertFromFile(const QString& file)                           { dropImage(file, QImage(file)); }
    void setDir(QString dir)                                           { mDir = dir; }
};

#endif // TEXTEDIT-OLD_H
