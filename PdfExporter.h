#pragma once

#include <QTextEdit>

#include "Exporter.h"
#include "Novel.h"

class PdfExporter: public Exporter<PdfExporter> {
private:
    QList<ExportMetadataField> mMetadata {
        { "title",  "Book Title (title)", "" },
        { "author", "Book Author",        "" },
        { "cover",  "Book Cover",         "" }
    };

public:
    PdfExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<PdfExporter>(novel, items) { }

    bool convert() override;

    QList<ExportMetadataField>& metadataFields() override { return mMetadata; }

    static QString Extension() { return "pdf"; }
    static QString Name()      { return "PDF"; }
};
