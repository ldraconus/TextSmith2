#pragma once
#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>

#include "SoundPool.h"

class TextEdit: public QTextEdit {
    Q_OBJECT

public:
    explicit TextEdit(QWidget* parent = nullptr);

    QMap<QUrl, QImage>& internalImages()                                  { return mOriginals; }
    QSet<QUrl>&         externalImageUrls()                               { return mExternalUrls; }
    void                setInternalImages(const QMap<QUrl, QImage>& imgs) { mOriginals = imgs; }
    void                setSoundPool(SoundPool* s)                        { mSoundPool = s; }

    // Serialization: internal images only
    void       addInternalImage(const QUrl& url, const QImage& image, bool add = true);
    void       clearInternalImages();
    void       registerInternalImages();
    void       removeInternalImage(const QUrl& url);
    QJsonArray serializeExternalImagesToJson();
    QJsonArray serializeInternalImagesToJson();
    void       setWrapMargin();

protected:
    bool       canInsertFromMimeData(const QMimeData *source) const override;
    QMimeData* createMimeDataFromSelection() const override;
    void       dragEnterEvent(QDragEnterEvent* event) override;
    void       dropEvent(QDropEvent* event) override;
    void       insertFromMimeData(const QMimeData* source) override;
    void       keyPressEvent(QKeyEvent* key) override;
    void       resizeEvent(QResizeEvent* event) override;

private:
    QSet<QUrl>         mExternalUrls;
    quint64            mIdBase =            0;
    bool               mInserting =         false;
    int                mLastViewportWidth = -1;
    QMap<QUrl, QImage> mOriginals;
    int                mReentry =           0;
    bool               mResizing =          false;
    SoundPool*         mSoundPool =         nullptr;
    int                mWrapMarginPx =      0;

    int              contentMaxWidth() const;
    void             fromJson(const QJsonDocument& document);
    QTextBlockFormat fromTextBlockFormatObject(QJsonObject &obj);
    QTextCharFormat  fromTextCharFormatObject(QJsonObject &obj);
    void             insertExternalUrlImage(const QUrl& url);
    void             insertInternalImage(const QImage& image);
    void             insertLocalImage(const QString& localFilePath);
    QUrl             makeInternalUrl();
    void             resizeImagesToFit();
    void             scheduleResize();
    QJsonObject      serializeInteralImageToJson(const QUrl& url, const QImage& img);
    QJsonDocument    toJson(const QTextCursor& selection) const;
    void             toTextBlockFormat(QJsonObject& obj, const QTextBlockFormat format) const;
    void             toTextCharFormat(QJsonObject& obj, const QTextCharFormat format) const;

    static QByteArray imageToPngBytes(const QImage& img);
};
