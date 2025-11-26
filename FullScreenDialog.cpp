#include "FullScreenDialog.h"
#include "ui_FullScreenDialog.h"

FullScreenDialog::FullScreenDialog(Preferences& prefs, QWidget *parent)
    : QDialog(parent)
    , mPrefs(&prefs)
    , mUi(new Ui::FullScreenDialog)
{
    mUi->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setWindowState(Qt::WindowMaximized);
    setWindowTitle("");
}

FullScreenDialog::~FullScreenDialog()
{
    delete mUi;
}
