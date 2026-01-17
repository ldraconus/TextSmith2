#pragma once

#include <QTextEdit>

#include "Exporter.h"

class MarkdownExporter: public Exporter<MarkdownExporter> {
public:
    MarkdownExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<MarkdownExporter>(novel, items) { }

    QList<ExportMetadataField> mMetadata = {
        { "title", "Book Title (title:)", "" },
        { "cover", "Book Cover (cover:)", "" }
    };

    bool    convert() override;
    void    convertHex(QString& paragraphs);
    void    convertDecimal(QString paragraphs);
    void    extracted(QString& paragraphs);
    QString htmlToMarkdown(const QString& html, const QString& dir);

    QList<ExportMetadataField>& metadataFields() override { return mMetadata; }

    static QString Extension() { return "md"; }
    static QString Name()      { return "Markdown"; }

    static QString paragraphs(const QString& html);
};
