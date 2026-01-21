#pragma once

#include <QDialog>

namespace Ui {
class PageSetupDialog;
}

class Preferences;
class Printer;
class QLineEdit;
class QPrintPreviewWidget;

class PageSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PageSetupDialog(QWidget* parent = nullptr);
    ~PageSetupDialog();

    double        bottomMargin();
    QString       footer();
    QString       header();
    double        leftMargin();
    QList<double> margins();
    double        rightMargin();
    double        topMargin();

private:
    Ui::PageSetupDialog *mUi;

    Preferences*         mPrefs;
    QPrintPreviewWidget* mPreview;
    Printer*             mPrinter;

    QList<double> list(QLineEdit* edit);
    QString       value(QLineEdit* edit);
};
