#pragma once

#include "Preferences.h"

#include <QDialog>
#include <QFont>
#include <QTimer>

namespace Ui {
class FullScreenDialog;
}

class FullScreenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FullScreenDialog(Preferences& prefs, QWidget* parent = nullptr);
    ~FullScreenDialog();

    QString    html();
    Json5Array other();
    qlonglong  position();
    void       setFont(const QFont& font);
    void       setHtml(const QString& html);
    void       setOther(Json5Array& other);
    void       setPosition(qlonglong pos);

protected:
    void keyPressEvent(QKeyEvent* evt) override;

private:
    bool                  mCtrlJSeen { false };
    Preferences*          mPrefs;
    QTimer                mTimer;
    Ui::FullScreenDialog* mUi;

    void imageFix();
    void setDark();
    void setLight();
    void setSystem();
};
