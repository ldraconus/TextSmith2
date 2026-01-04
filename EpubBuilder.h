#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QMap>

#include "Novel.h"

struct zip;
struct zip_source;

class EpubBuilder {
public:
    explicit EpubBuilder(const QString& outputPath, QList<qlonglong> ids)
        : mOutputPath(outputPath)
        , mItemIds(ids) { }

    ~EpubBuilder();

    bool open();
    bool close();

    bool isOpen() const { return mZip != nullptr; }

    // Metadata
    void setTitle(const QString& title)   { mTitle = title; }
    void setAuthor(const QString& author) { mAuthor = author; }
    void setIdentifier(const QString& id) { mIdentifier = id; }

    // Content
    // id is used in manifest/spine (e.g. "chap1")
    bool addXhtml(const QString& filenameInEpub, const QString& xhtmlContent);
    // id like "cover-image", filenameInEpub like "images/cover.jpg"
    bool addImage(const QString& filenameInEpub, const QByteArray& imageData);

private:
    bool writeMimetype();
    bool writeContainerXml();
    bool writeContentOpf();

    bool addFileFromData(const QString& filenameInEpub, const QByteArray& data);

    QString mOutputPath;
    zip*    mZip = nullptr;

    QList<Item>      mItems;    // manifest items
    QList<qlonglong> mItemIds;
    QList<QString>   mSpine;    // ordered ids

    QString mTitle  =     QStringLiteral("Untitled");
    QString mAuthor =     QStringLiteral("Unknown");
    QString mIdentifier = QStringLiteral("urn:uuid:dummy-id");
};
