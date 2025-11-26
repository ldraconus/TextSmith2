#pragma once

#include "Preferences.h"

#include <QDialog>

namespace Ui {
class FullScreenDialog;
}

class FullScreenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FullScreenDialog(Preferences& prefs, QWidget *parent = nullptr);
    ~FullScreenDialog();

private:
    Preferences*          mPrefs;
    Ui::FullScreenDialog* mUi;
};
