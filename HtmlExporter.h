#pragma once

#include <QTextEdit>

#include "Exporter.h"
#include "Novel.h"

class HtmlExporter: public Exporter<HtmlExporter> {
private:
    QList<ExportMetadataField> mMetadata = {
        { "title", "Book Title (title:)", "" },
        { "cover", "Book Cover (cover:)", "" }
    };

public:
    HtmlExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<HtmlExporter>(novel, items) { }

    bool convert() override;

    QList<ExportMetadataField>& metadataFields() override { return mMetadata; }

    static QString Extension() { return "html"; }
    static QString Name()      { return "Html"; }

    static QString convert(Novel& novel, QList<qlonglong>& ids, const QString& cover, const QList<QString>& tag);
    static void    extracted(QString& html, const QString& dir, QMap<QUrl, QImage>& images);
    static QString generateImageHtml(const QString& url);
    static QString addParagraphs(const QString& html);
};
