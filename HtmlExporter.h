#pragma once

#include <QTextEdit>

#include "Exporter.h"
#include "Novel.h"

class HtmlExporter: public Exporter<HtmlExporter> {
private:
    QList<ExportMetadataField> mMetadata = {
        { "title", "Book Title (title)", "" },
        { "cover", "Book Cover",         "" }
    };

public:
    HtmlExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<HtmlExporter>(novel, items) { }

    bool convert() override;

    QList<ExportMetadataField>& metadataFields() override { return mMetadata; }

    static QString Extension() { return "html"; }
    static QString Name()      { return "Html"; }

    static bool convert(Novel& novel, QList<qlonglong>& ids, QTextEdit& doc);
    static bool convert(Novel& novel, QList<qlonglong>& ids, QTextDocument& doc);
};
