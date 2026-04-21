#pragma once

#include <QTextEdit>

#include "Exporter.h"
#include "Main.h"

class TextExporter: public Exporter<TextExporter> {
private:
    QString outputFooter(int pageNo, int lineWidth, qlonglong id);
    QString outputHeader(int pageNo, int lineWidth, qlonglong id);

public:
    TextExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<TextExporter>(novel, items) {
    }

    QList<ExportMetadataField> mMetadata = {
        { "title",     "Book Title (title:)",                           "" },
        { "bold",      "Comma separated Bold makers (bold:)",           "*,*" },
        { "italic",    "Comma separated Italic makers (italic:)",       "/,/" },
        { "underline", "Comma separated Underline makers (underline:)", "_,_" },
    };

    bool    convert() override;

    QList<ExportMetadataField>& metadataFields() override {
        mMetadata[1].defaultValue = Main::ref().prefs().bold();
        mMetadata[2].defaultValue = Main::ref().prefs().italic();
        mMetadata[3].defaultValue = Main::ref().prefs().underline();
        return mMetadata;
    }

    static QString Extension() { return "txt"; }
    static QString Name()      { return "Plain Text"; }
};
