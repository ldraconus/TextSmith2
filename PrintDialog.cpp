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
    auto* layout = new QVBoxLayout(mUi->previewFrame);
    mUi->previewFrame->setLayout(layout);
    mPreview = new QPrintPreviewWidget(mPrinter->qprinter(), mUi->previewFrame);
    layout->addWidget(mPreview);

    connect(mUi->printPushButton, &QPushButton::clicked, this, [this]() {
        mUi->printPushButton->setDisabled(true);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        mPreview->print();
        QApplication::restoreOverrideCursor();
        mUi->printPushButton->setEnabled(true);
    });

    connect(mPreview, &QPrintPreviewWidget::paintRequested, this,
            [this](QPrinter* printer) {
                if (mPrinter == nullptr || printer == nullptr) return;
                Printer print(printer);
                print.setPrefs(mPrefs);
                auto* edit = Main::ref().ui()->textEdit;
                print.setImages(edit->internalImages());
                auto& images = edit->externalImageUrls();
                for (const auto& url: images) print.addImage(url);
                print.setIds(mPrinter->ids());
                QPrinterInfo info(*printer);
                auto pageSize = info.defaultPageSize();
                print.setPageSize(pageSize.id());
                QPageLayout::Orientation orientation = mPrefs->orientation();
                print.setPageOrientation(orientation);
                Printer* savedPrinter = Main::ref().exchangePrinter(&print);
                Main::ref().printNovel();
                Main::ref().exchangePrinter(savedPrinter);
            });

    mPrinter->setIds(Main::ref().vectorOfIds(Main::ref().ui()->treeWidget->topLevelItem(0),
                                             { mPrefs->chapterTag(), mPrefs->sceneTag(), mPrefs->coverTag() }));

    updatePreview();

    mReady = true;
}

void PrintDialog::updatePreview() {
    if (mPreview && mReady) mPreview->updatePreview();
}

PrintDialog::~PrintDialog() {
    delete mUi;
}
