#pragma once

#include <QTextEdit>

#include "Exporter.h"

class TextExporter: public Exporter<TextExporter> {
public:
    TextExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<TextExporter>(novel, items) { }

    QList<ExportMetadataField> mMetadata = {
        { "title",     "Book Title (title:)", "" },
        { "bold",      "Comma separated Bold makers (bold:)",           "*,*" },
        { "italic",    "Comma separated Italic makers (italic:)",       "/,/" },
        { "underline", "Comma separated Underline makers (underline:)", "_,_" },
    };

    bool    convert() override;

    QList<ExportMetadataField>& metadataFields() override { return mMetadata; }

    static QString Extension() { return "txt"; }
    static QString Name()      { return "Plain Text"; }
};
