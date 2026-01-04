#include "MarkdownExporter.h"
#include "HtmlExporter.h"
#include "html2md-main/include/html2md.h"

#include <QFileInfo>

#include "Main.h"

bool MarkdownExporter::convert() {
    QTextEdit doc;
    doc.setAcceptRichText(true);
    bool result = HtmlExporter::convert(mNovel, mItemIds, doc);

    if (mFilename.isEmpty()) return false;
    QFileInfo info(mFilename);
    Main::ref().setDocDir(info.absolutePath());

    QString md = htmlToMarkdown(doc.toHtml());

    QFile file(mFilename);
    if (!file.open(QIODeviceBase::WriteOnly)) return false;
    return result && (file.write(md.toUtf8()) != -1);
}

QString MarkdownExporter::htmlToMarkdown(const QString& html) {
    return QString::fromStdString(html2md::Convert(html.toStdString()));
}
