#include "HtmlExporter.h"

#include <QFileInfo>

#include "Main.h"

bool HtmlExporter::convert() {
    QTextEdit doc;
    doc.setAcceptRichText(true);
    bool result = convert(mNovel, mItemIds, doc);

    const auto& defaults = collectMetadataDefaults();
    QString title = fetchValue(0, defaults, "title");

    QString html = doc.toHtml();
    if (!title.isEmpty()) {
        if (html.contains("<head>")) html.replace("<head>", QString("<head><title>%1</title>").arg(title.toHtmlEscaped()));
        else html.prepend(QString("<title>%1</title>").arg(title.toHtmlEscaped()));
    }

    if (mFilename.isEmpty()) return false;
    QFileInfo info(mFilename);
    Main::ref().setDocDir(info.absolutePath());

    QFile file(mFilename);
    if (!file.open(QIODeviceBase::WriteOnly)) return false;
    return result && (file.write(html.toUtf8()) != -1);
}

bool HtmlExporter::convert(Novel& novel, QList<qlonglong>& ids, QTextEdit& doc) {
    for (auto& id: ids) {
        Item& item = novel.findItem(id);
        doc.append(item.html());
    }
    return true;
}

bool HtmlExporter::convert(Novel& novel, QList<qlonglong>& ids, QTextDocument& doc) {
    QTextCursor cursor(&doc);
    for (auto& id: ids) {
        Item& item = novel.findItem(id);
        cursor.insertHtml(item.html());
    }
    return true;
}
