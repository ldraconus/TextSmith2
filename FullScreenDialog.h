#pragma once

#include "Preferences.h"
#include "SoundPool.h"

#include <QDialog>
#include <QFont>
#include <QTimer>

#include <List.h>

namespace Ui {
class FullScreenDialog;
}

class QTextDocument;

class FullScreenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FullScreenDialog(Preferences& prefs, QWidget* parent = nullptr);
    ~FullScreenDialog();

    List<int>  other();
    qlonglong  position();
    void       setDocument(QTextDocument* doc);
    void       setOther(List<int>& other);
    void       setPosition(qlonglong pos);
    void       setSoundPool(SoundPool* soundPool);

protected:
    void keyPressEvent(QKeyEvent* evt) override;

private:
    bool                  mCtrlJSeen { false };
    Preferences*          mPrefs;
    QTimer                mTimer;
    Ui::FullScreenDialog* mUi;

    void applyNovelFormatting();
    void imageFix();
    void setDark();
    void setLight();
    void setSystem();
};
