#pragma once

#include <QDialog>

#include <StringList.h>

#include "Preferences.h"

namespace Ui {
class PreferencesDialog;
}

class QTextToSpeech;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    PreferencesDialog(Preferences& prefs, QWidget* parent = nullptr);
    ~PreferencesDialog();

    Ui::PreferencesDialog* ui() { return mUi; }

    static PreferencesDialog* ptr() { return sPreferencesDialog; }
    static PreferencesDialog& ref() { return *ptr(); }

public slots:
    void font();
    void saveChanges();
    void theme(int idx);

private:
    Preferences*           mPrefs;
    QTextToSpeech*         mSpeech;
    Ui::PreferencesDialog* mUi;

    void loadAvailableVoices();
    void setTextEditFont(QFont& font);
    void setupConnections();

    static PreferencesDialog* sPreferencesDialog;
};
