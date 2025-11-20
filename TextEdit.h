#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QFileInfo>
#include <QKeyEvent>
#include <QImageReader>
#include <QImageWriter>
#include <QMap>
#include <QMimeData>
#include <QTextEdit>

class TextEdit : public QTextEdit {
private:
    QMap<QUrl, QImage> Images;
    bool NewImages = false;
    const bool Save = true;
    QString mDir = ".";
    static size_t mBase;
    bool mAllowTextPaste = false;

    QImage loadImage(const QUrl& url) {
        QString path = url.adjusted(QUrl::RemoveFilename).path();
        if (path.isEmpty()) path = mDir;
        QString filename = url.fileName();
        if (filename.right(3) != "png") filename += ".png";
        QImageReader reader(path + "/" + filename, "png");
        QImage img = reader.read();
        document()->addResource(QTextDocument::ImageResource, url, img);
        return img;
    }

    void saveImage(const QUrl& url, const QImage& img) {
        QString path = url.adjusted(QUrl::RemoveFilename).path();
        if (path.isEmpty()) path = mDir;
        QString filename = url.fileName();
        if (filename.right(3) != "png") filename += ".png";
        QImageWriter writer(path + "/" + filename, "png");
        writer.write(img);
    }

    void dropImage(const QString& file, const QImage& image, bool save = false) {
        if (!image.isNull()) {
            if (save) {
                Images[file] = image;
                NewImages = true;
            }
            document()->addResource(QTextDocument::ImageResource, file, image);
            textCursor().insertImage(file);
        }
    }

    void dropImage(const QUrl& url, const QImage& image, bool save = false) {
        if (!image.isNull()) {
            if (save) {
                Images[url] = image;
                NewImages = true;
            }
            document()->addResource(QTextDocument::ImageResource, url, image);
            textCursor().insertImage(url.toString());
        }
    }

    void dropTextFile(const QUrl& url) {
        QString path = url.adjusted(QUrl::RemoveFilename).path();
        if (path.isEmpty()) path = mDir;
        else if (path.right(1) != "/") path += "/";
        QFile file(path + url.fileName());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) textCursor().insertText(file.readAll());
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
            static size_t i = mBase;
            QUrl url(QString("dropped_image_%1").arg(i++));
            dropImage(url, qvariant_cast<QImage>(source->imageData()), Save);
        } else if (source->hasUrls()) {
            foreach (QUrl url, source->urls()) {
                QFileInfo info(url.fileName());
                if (QImageReader::supportedImageFormats().contains(info.suffix().toLower().toLatin1())) dropImage(url, QImage(info.filePath()));
                else dropTextFile(url);
            }
        } else if (mAllowTextPaste) QTextEdit::insertFromMimeData(source);
    }

    void saveImages() {
        for (auto x = Images.begin(); x != Images.end(); ++x) saveImage(x.key(), x.value());
        NewImages = false;
    }

    bool canInsertFromMimeData(const QMimeData* source) const override { return source->hasImage() || source->hasUrls() || QTextEdit::canInsertFromMimeData(source); }
    bool hasNewImages()                                                { return NewImages; }
    void allowTextPaste()                                              { mAllowTextPaste = true; }
    void denyTextPaste()                                               { mAllowTextPaste = false; }
    void insertFromFile(const QString& file)                           { dropImage(file, QImage(file)); }
    void setDir(QString dir)                                           { mDir = dir; }
};

#endif // TEXTEDIT_H
