#include "PdfExporter.h"
#include "HtmlExporter.h"

#include <QFileInfo>
#include <QPainter>
#include <QPrinter>

#include "Main.h"

bool PdfExporter::convert() {
    if (mFilename.isEmpty()) return false;

    QTextDocument doc;
    QFont font(Main::ref().prefs().fontFamily(), Main::ref().prefs().fontSize());
    doc.setDefaultFont(font);
    HtmlExporter::convert(mNovel, mItemIds, doc);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(mFilename);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setPageMargins(QMarginsF(0.5, 0.5, 0.5, 0.5), QPageLayout::Inch);

    const auto& defaults = collectMetadataDefaults();
    QString title  = fetchValue(0, defaults, "title");
    QString author = fetchValue(1, defaults, "author");
    if (!title.isEmpty())  printer.setDocName(title);
    // (Author metadata support is limited here, but title at least is set.)

    doc.print(&printer);  // <-- Qt handles pagination for you

    QPainter painter(&printer);

    QFile file(mFilename);
    if (!file.exists() || file.size() == 0) return false;

    QFileInfo info(mFilename);
    Main::ref().setDocDir(info.absolutePath());

    return true;
}
