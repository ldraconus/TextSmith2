#include "ItemDescriptionDialog.h"
#include "ui_ItemDescriptionDialog.h"

ItemDescriptionDialog::ItemDescriptionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ItemDescriptionDialog)
{
    ui->setupUi(this);
}

ItemDescriptionDialog::~ItemDescriptionDialog()
{
    delete ui;
}
