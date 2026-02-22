#pragma once

#include <QDialog>

namespace Ui {
class PrintDialog;
}

class Preferences;
class QPrintPreviewWidget;
class Printer;
class PrintDialog: public QDialog {
    Q_OBJECT

public:
    explicit PrintDialog(QWidget *parent = nullptr);
    ~PrintDialog();

private:
    Ui::PrintDialog*     mUi { nullptr };
    Preferences*         mPrefs { nullptr };
    QPrintPreviewWidget* mPreview { nullptr };
    Printer*             mPrinter { nullptr };
    bool                 mReady { false };

    void updatePreview();
};
