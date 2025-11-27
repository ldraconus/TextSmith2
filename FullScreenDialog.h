#pragma once

#include "Preferences.h"

#include <QDialog>
#include <QFont>

namespace Ui {
class FullScreenDialog;
}

class FullScreenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FullScreenDialog(Preferences& prefs, QWidget* parent = nullptr);
    ~FullScreenDialog();

    QString   html();
    qlonglong position();
    void      setFont(const QFont& font);
    void      setHtml(const QString& html);
    void      setPosition(qlonglong pos);

protected:
    void keyPressEvent(QKeyEvent* evt) override;

private:
    bool                  mCtrlJSeen { false };
    Preferences*          mPrefs;
    Ui::FullScreenDialog* mUi;
};
