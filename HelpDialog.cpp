#include "HelpDialog.h"
#include "ui_HelpDialog.h"
#include "Main.h"

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
    , mUi(new Ui::HelpDialog) {
    mUi->setupUi(this);

    QString filename = Main::ref().appDir() + "/Documentation.html";

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QByteArray data(file.readAll());
    file.close();

    QString html(data);
    mUi->textBrowser->setHtml(html);
}

HelpDialog::~HelpDialog() {
    delete mUi;
}
