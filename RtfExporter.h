#pragma once

#include <QTextEdit>
#include <QTextFragment>

#include "Exporter.h"

#include <StringList.h>

class RtfExporter: public Exporter<RtfExporter> {
public:
    RtfExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<RtfExporter>(novel, items) { }

    static constexpr int Cover =   0;
    static constexpr int Margins = 1;

    QList<ExportMetadataField> mMetadata = {
        { "cover",   "Book Cover (cover:)",                 "" },
        { "margins", "Paper Margins (margins: L, R, T, B)", "1\", 1\", 1\", 1\"" },
        { "header",  "Page Header (header:)",               ">%T\\n>%L" },
        { "footer",  "Page Footer (footer:)",               ">%#<"}
    };

    static constexpr auto PointsPerInch = 72.0;
    static constexpr auto TwipsPerPoint = 20.0;

    bool convert() override;

    QList<ExportMetadataField>& metadataFields() override { return mMetadata; }

    static QString Extension() { return "rtf"; }
    static QString Name()      { return "Rich Text Format"; }

private:
    QMarginsF mMargins { 1.0, 1.0, 1.0, 1.0 };

    QString escapeRtfText(const QString &text);
    QString handleImage(Item& item);
    QString handleImage(const QString& cover);
    QString itemsToRtf(const QString& cover);
    QString processText(const QTextFragment& fragment);
};
