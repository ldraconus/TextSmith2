#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "TSVersion.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , mUi(new Ui::AboutDialog) {
    mUi->setupUi(this);

    QString html = mUi->textBrowser->toHtml();
    html.replace("[v]", TSVersion);
    mUi->textBrowser->setHtml(html);
}

AboutDialog::~AboutDialog() {
    delete mUi;
}
