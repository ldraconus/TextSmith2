#pragma once

#include <QTextEdit>

#include "Exporter.h"
#include "Novel.h"

#include "Map.h"

struct zip;

class EBookExporter: public Exporter<EBookExporter> {
private:
    QList<ExportMetadataField> mMetadata {
        { "title",     "Book Title (title)",          "" },
        { "author",    "Book Author (author)",        "Anoonymous" },
        { "cover",     "Book Cover (cover)",          "" },
        { "rights",    "Publication Rights (rights)", "Public Domain" },
        { "langauge",  "Langauge (language)",         "en-US" },
        { "publisher", "Publisher (publisher)",       "Independent" },
        { "id",        "ISBN (id)",                   "" },
        { "year",      "Publication Year (year)",     "" }
    };

    QString                 mAuthor;
    Map<qlonglong, QString> mBook;
    QString                 mCover;
    Map<qlonglong, QString> mHtml;
    QString                 mId;
    Map<QString, QString>   mImages;
    Map<QUrl, QByteArray>   mJpg;
    QString                 mRights;
    QString                 mLanguage;
    QString                 mPublisher;
    QString                 mTitle;
    QString                 mYear;
    zip*                    mZip { nullptr };

    bool    addEntry(const QString& name, const QString& value);
    bool    addEntry(const QString& name, const QByteArray& value);
    QString chapterManifest();
    int     chapterNumWidth();
    QString convertHTML(const QString& qHtml);
    QString fixImages(QMap<QString, QString>& jpgs, const QString& qHtml);
    void    loadImageBytes(const QImage& image, std::function<void(QByteArray)> callback);
    void    loadImageBytesFromUrl(const QUrl& url, std::function<void(QByteArray)> callback);
    QString navPoints();
    void    novelToBook();
    QString replace(const QString& qHtml, const QString& front, const QString& back, const QString& with);
    QString spineTOC();
    bool    writeContentOpf();
    bool    writeContainerXml();
    bool    writeCover();
    bool    writeMimetype();
    bool    writePageTemplate();
    bool    writeStylesheet();
    bool    writeToc();

public:
    EBookExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<EBookExporter>(novel, items) { }
    ~EBookExporter();

    bool convert() override;

    QList<ExportMetadataField>& metadataFields() override { return mMetadata; }

    static QString Extension() { return "epub"; }
    static QString Name()      { return "E-Book"; }
};
