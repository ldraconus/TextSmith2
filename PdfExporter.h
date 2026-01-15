#pragma once

#include <QPageSize>
#include <QTextEdit>

#include "Exporter.h"
#include "Novel.h"

class PdfExporter: public Exporter<PdfExporter> {
private:
    QString               mAuthor;
    QString               mCover;
    QMarginsF             mMargins     { 1.0, 1.0, 1.0, 1.0 };
    QString               mPaperSize = "Letter / ANSI A";
    QPageSize::PageSizeId mPid =       QPageSize::Letter;
    QString               mTitle;

    static constexpr auto Title =        0;
    static constexpr auto Cover =        1;
    static constexpr auto Author =       2;
    static constexpr auto PaperSize =    3;
    static constexpr auto PaperMargins = 4;
    static constexpr auto Header =       5;
    static constexpr auto Footer =       6;

    QList<ExportMetadataField> mMetadata {
        { "title",      "Book Title (title:)",                  "" },
        { "cover",      "Book Cover (cover:file)",              "" },
        { "author",     "Author (author:)",                     "" },
        { "paper size", "Paper Size (paperSize:)",              "Letter / ANSI A", false,
            []() -> StringList {
                StringList list;
                for (int id = QPageSize::Letter; id <= QPageSize::LastPageSize; ++id) {
                    QPageSize::PageSizeId pid = static_cast<QPageSize::PageSizeId>(id);
                    QPageSize size(pid);
                    QString name = size.name();
                    if (size.isValid() && ((name.startsWith("A") && name[1].isDigit()) ||
                                            name.startsWith("Letter") ||
                                            name.startsWith("Legal"))) list.append(size.name());
                }
                list.sort();
                return list;
            }
        },
        { "margins",     "Paper Margins (margins: L, R, T, B)", "1\", 1\", 1\", 1\"" },
        { "header",      "Page Header (header:)",               ">%T\\n>%L" },
        { "footer",      "Page Footer (footer:)",               ">%#<"}
    };

    void render();

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
