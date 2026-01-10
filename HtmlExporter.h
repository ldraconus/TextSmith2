#pragma once

#include <QTextEdit>

#include "Exporter.h"
#include "Novel.h"
#include "TextEdit.h"

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

    static bool    convert(Novel& novel, QList<qlonglong>& ids, const QString& cover, TextEdit& doc,
                        const QList<QString>& tag);
    static bool    convert(Novel& novel, QList<qlonglong>& ids, const QString& cover, QTextDocument& doc,
                        const QList<QString>& tag);
    static QString generateCoverHtml(const QString& cover);
    static void    pageBreak(QTextDocument* doc, const QString& html);
};
