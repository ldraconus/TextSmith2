#include "PageSetupDialog.h"
#include "ui_PageSetupDialog.h"
#include "Main.h"
#include "ui_Main.h"
#include "PdfExporter.h"

#include <QLayout>
#include <QPrintPreviewWidget>

PageSetupDialog::PageSetupDialog(QWidget* parent)
    : QDialog(parent)
    , mPrefs(&Main::ref().prefs())
    , mPrinter(Main::ref().printer())
    , mUi(new Ui::PageSetupDialog) {
    mUi->setupUi(this);

    auto* layout = new QVBoxLayout(this);
    mPreview = new QPrintPreviewWidget(mPrinter, this);
    layout->addWidget(mPreview);
    mUi->previewFrame->setLayout(layout);

    if (mPrinter == nullptr) Main::ref().setPrinter(mPrinter = new Printer(QPrinter::HighResolution));
    if (mPrinter == nullptr || mPrinter->qprinter() == nullptr) {
        Main::ref().msg().Statement("Unable to talk to the printer");
        return;
    }
    TextEdit* edit = Main::ref().ui()->textEdit;
    mPrinter->setImages(edit->internalImages());
    auto& images = edit->externalImageUrls();
    for (const auto& url: images) mPrinter->addImage(url);
    mPrinter->setIds(Main::ref().vectorOfIds(Main::ref().ui()->treeWidget->currentItem(),
                                             { mPrefs->chapterTag(), mPrefs->sceneTag(), mPrefs->coverTag() }));

    connect(mPreview, &QPrintPreviewWidget::paintRequested, this, [](QPrinter* printer) { Main::ref().print(printer); });
}

              PageSetupDialog::~PageSetupDialog() { delete mUi; }
double        PageSetupDialog::bottomMargin()     { return PdfExporter::toInches(value(mUi->bottomMaarginineEdit)); }
QString       PageSetupDialog::footer()           { return value(mUi->footerLineEdit); }
QString       PageSetupDialog::header()           { return value(mUi->headerLineEdit); }
double        PageSetupDialog::leftMargin()       { return PdfExporter::toInches(value(mUi->leftMarginLineEdit)); }
QList<double> PageSetupDialog::margins()          { return list(mUi->marginsLineEdit); }
double        PageSetupDialog::rightMargin()      { return PdfExporter::toInches(value(mUi->rightMarginLineEdit)); }
double        PageSetupDialog::topMargin()        { return PdfExporter::toInches(value(mUi->topMaginLineEdit)); }

QList<double> PageSetupDialog::list(QLineEdit* edit) {
    QString text = value(edit);
    StringList margins { text.split(",") };
    while (margins.count() > 4) margins.takeLast();
    while (margins.count() < 4) margins += "1\"";
    QList<double> results;
    for (auto i = 0; i < margins.count(); ++i) results += PdfExporter::toInches(margins[i].trimmed());
    return results;
}

QString PageSetupDialog::value(QLineEdit* edit) {
    QString text = edit->text();
    if (text.isEmpty()) text = edit->placeholderText();
    return text;
}
