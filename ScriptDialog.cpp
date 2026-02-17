#include "ScriptDialog.h"
#include "ui_ScriptDialog.h"

ScriptDialog::ScriptDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ScriptDialog)
{
    ui->setupUi(this);
}

ScriptDialog::~ScriptDialog()
{
    delete ui;
}
