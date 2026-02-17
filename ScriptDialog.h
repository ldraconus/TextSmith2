#pragma once

#include <QDialog>

namespace Ui {
class ScriptDialog;
}

class ScriptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptDialog(QWidget *parent = nullptr);
    ~ScriptDialog();

private:
    Ui::ScriptDialog *ui;
};
