#pragma once

#include <QPageSize>
#include <QTextEdit>

#include "Exporter.h"
#include "Novel.h"

class PdfExporter: public Exporter<PdfExporter> {
private:
    QString               mAuthor;
    QString               mCover;
    QString               mTitle;

    static constexpr auto Title =        0;
    static constexpr auto Cover =        1;
    static constexpr auto Author =       2;

    QList<ExportMetadataField> mMetadata {
        { "title",      "Book Title (title:)",                  "" },
        { "cover",      "Book Cover (cover:file)",              "" },
        { "author",     "Author (author:)",                     "" },
    };

    bool render();

public:
    PdfExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<PdfExporter>(novel, items) { }

    bool convert() override;

    QList<ExportMetadataField>& metadataFields() override { return mMetadata; }

    static QString Extension() { return "pdf"; }
    static QString Name()      { return "PDF"; }

    static QMarginsF parseMargins(StringList& inputMargins);
    static double    parseNumber(const QString& s);
    static double    toInches(const QString& measure);
};
