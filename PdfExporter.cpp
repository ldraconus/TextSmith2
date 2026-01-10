#include "PdfExporter.h"
#include "HtmlExporter.h"

#include <QFileInfo>
#include <QPainter>
#include <QPrinter>

#include "Main.h"
#include "ui_Main.h"

bool PdfExporter::convert() {
    if (mFilename.isEmpty()) return false;

    TextEdit* doc = Main::ref().ui()->textEdit;
    QString preHtml = doc->toHtml();
    QTextDocument* preDoc = doc->document();
    QTextDocument* tempDoc = new QTextDocument();
    tempDoc->setParent(nullptr);
    doc->setDocument(tempDoc);
    const auto& defaults = collectMetadataDefaults();
    QString cover = fetchValue(0, defaults, "cover");
    HtmlExporter::convert(mNovel, mItemIds, cover, *doc, { mChapterTag, mSceneTag, mCoverTag });
    doc->registerInternalImages();
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(mFilename);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setPageMargins(QMarginsF(0.5, 0.5, 0.5, 0.5), QPageLayout::Inch);

    QString title  = fetchValue(0, defaults, "title");
    QString author = fetchValue(1, defaults, "author");
    if (!title.isEmpty()) printer.setDocName(title);
    // (Author metadata support is limited here, but title at least is set.)

    doc->print(&printer);  // <-- Qt handles pagination for you

//    QPainter painter(&printer);

    doc->setDocument(preDoc);
    doc->setHtml(preHtml);
    QFile file(mFilename);
    if (!file.exists() || file.size() == 0) return false;

    QFileInfo info(mFilename);
    Main::ref().setDocDir(info.absolutePath());

    return true;
}
