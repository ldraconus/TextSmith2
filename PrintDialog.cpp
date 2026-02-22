#include "PrintDialog.h"
#include "ui_PrintDialog.h"
#include "Main.h"
#include "ui_Main.h"

#include <QDir>
#include <QPrintPreviewWidget>
#include <QPrinterInfo>

#include "Preferences.h"
#include "Printer.h"
#include "TextEdit.h"

PrintDialog::PrintDialog(QWidget *parent)
    : QDialog(parent)
    , mPrefs(&Main::ref().prefs())
    , mPrinter(Main::ref().printer())
    , mUi(new Ui::PrintDialog) {
    mUi->setupUi(this);

    mPrinter = new Printer(QPrinter::HighResolution);
    if (mPrinter == nullptr || mPrinter->qprinter() == nullptr) {
        Main::ref().msg().Statement("Unable to talk to the preview printer");
        reject();
        return;
    }

    mPrinter->setPrefs(mPrefs);
/*
    mPrinter->setOutputFormat(QPrinter::PdfFormat);
    mPrinter->setOutputFileName(QDir::temp().filePath("preview.pdf"));
    mPrinter->setPrinterName(QString());
*/
    auto* layout = new QVBoxLayout(mUi->previewFrame);
    mUi->previewFrame->setLayout(layout);
    mPreview = new QPrintPreviewWidget(mPrinter->qprinter(), mUi->previewFrame);
    layout->addWidget(mPreview);

    connect(mUi->printPushButton, &QPushButton::clicked, mPreview, &QPrintPreviewWidget::print);

    connect(mPreview, &QPrintPreviewWidget::paintRequested, this,
            [this](QPrinter* printer) {
                if (mPrinter == nullptr) return;
                if (mPrinter->qprinter() == nullptr) return;
                Printer* savedPrinter = Main::ref().exchangePrinter(mPrinter);
                Main::ref().printNovel();
                mPrinter = Main::ref().exchangePrinter(savedPrinter);
            });

    auto* edit = Main::ref().ui()->textEdit;
    mPrinter->setImages(edit->internalImages());
    auto& images = edit->externalImageUrls();
    for (const auto& url: images) mPrinter->addImage(url);
    mPrinter->setIds(Main::ref().vectorOfIds(Main::ref().ui()->treeWidget->topLevelItem(0),
                                             { mPrefs->chapterTag(), mPrefs->sceneTag(), mPrefs->coverTag() }));

    QPrinterInfo info(*mPrinter->qprinter());
    auto pageSize = info.defaultPageSize();
    mPrinter->setPageSize(pageSize.id());
    QPageLayout::Orientation orientation = mPrefs->orientation();
    mPrinter->setPageOrientation(orientation);

    updatePreview();

    mReady = true;
}

void PrintDialog::updatePreview() {
    if (mPreview && mReady) mPreview->updatePreview();
}

PrintDialog::~PrintDialog() {
    delete mUi;
}
