#pragma once
#include <QTextDocument>
#include <QTextEdit>
#include <QImage>
#include <QImageReader>
#include <QBuffer>
#include <QMimeData>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

class TextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit TextEdit(QWidget* parent = nullptr);

    QMap<QUrl, QImage>& internalImages()    { return mOriginals; }
    QSet<QUrl>&         externalImageUrls() { return mExternalUrls; }

    // Serialization: internal images only
    void           addInternalImage(const QUrl& url, const QImage& image, bool add = true);
    void           clearInternalImages();
    void           removeInternalImage(const QUrl& url);
    QJsonArray     serializeExternalImagesToJson();
    QJsonArray     serializeInternalImagesToJson();

protected:
    bool       canInsertFromMimeData(const QMimeData *source) const override;
    QMimeData* createMimeDataFromSelection() const override;
    void       dragEnterEvent(QDragEnterEvent* event) override;
    void       dropEvent(QDropEvent* event) override;
    void       insertFromMimeData(const QMimeData* source) override;
    void       resizeEvent(QResizeEvent* event) override;

private:
    QSet<QUrl>         mExternalUrls; // Track external URLs (http/https)
    QMap<QUrl, QImage> mOriginals;    // Originals for internal images (crisp source of truth)

    // Insertion
    void insertExternalUrlImage(const QUrl& url);
    void insertInternalImage(const QImage& image);
    void insertLocalImage(const QString& localFilePath);

    // Resizing
    int  contentMaxWidth() const;
    void resizeImagesToFit();
    void scheduleResize();

    // Helpers
    static QByteArray imageToPngBytes(const QImage& img);
    QUrl              makeInternalUrl();

    // Guards and state
    quint64 mIdBase =            0;
    bool    mInserting =         false;
    int     mLastViewportWidth = -1;
    int     mReentry =           0;
    bool    mResizing =          false;
};
