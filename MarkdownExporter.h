#pragma once

#include <QTextEdit>

#include "Exporter.h"

class MarkdownExporter: public Exporter<MarkdownExporter> {
public:
    MarkdownExporter(Novel& novel, const QList<qlonglong>& items)
        : Exporter<MarkdownExporter>(novel, items) { }

    bool    convert() override;
    QString htmlToMarkdown(const QString& html);

    static QString Extension() { return "md"; }
    static QString Name()      { return "Markdown"; }
};
