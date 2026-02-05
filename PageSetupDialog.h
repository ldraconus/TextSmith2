#pragma once

#include <QDialog>
#include <QPrinterInfo>

#include <List.h>

#include "Preferences.h"

namespace Ui {
class PageSetupDialog;
}

class Preferences;
class Printer;
class QDoubleSpinBox;
class QLineEdit;
class QPrintPreviewWidget;
class QTextEdit;
class QTImer;

class PageSetupDialog : public QDialog {
    Q_OBJECT

public:
    explicit PageSetupDialog(QWidget* parent = nullptr);
    ~PageSetupDialog();

    Preferences* preferences() { return mPrefs; }
    Printer*     printer()     { return mPrinter; }

    double       bottomMargin();
    QString      footer();
    QString      header();
    double       leftMargin();
    List<double> margins();
    double       rightMargin();
    double       topMargin();

protected:
    bool event(QEvent *e);

public slots:
    void marginChanged();

private:
    Ui::PageSetupDialog *mUi { nullptr };

    QWidget*             mFocusWidget     { nullptr };
    Preferences*         mPrefs;
    QPrintPreviewWidget* mPreview         { nullptr };
    Printer*             mPrinter         { nullptr };
    QList<QPrinterInfo>  mPrinterList;
    bool                 mReady           { false };
    Printer*             mSavedPrinter    { nullptr };
    QTimer*              mPreviewTimer    { nullptr };

    void    checkMargins();
    void    justify(QTextEdit* edit, Qt::AlignmentFlag which);
    void    setFooter();
    void    setHeader();
    void    setMargins();
    void    updatePreview();
    QString value(QDoubleSpinBox* edit);
};
