#pragma once

#include <QDialog>

namespace Ui {
class ItemDescriptionDialog;
}

class ItemDescriptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ItemDescriptionDialog(QWidget *parent = nullptr);
    ~ItemDescriptionDialog();

private:
    Ui::ItemDescriptionDialog *ui;
};
