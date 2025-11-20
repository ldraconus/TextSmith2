#pragma once

#include <QMainWindow>

#include <StringList.h>
#include <Message.h>

#include "Novel.h"
#include "Preferences.h"

class QApplication;
class QCloseEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
    class Main;
}
QT_END_NAMESPACE

class Main: public QMainWindow {
    Q_OBJECT

private:
    QApplication* mApp;
    QString       mAppDir;
    qlonglong     mCurrentNode { -1 };
    QString       mDocDir;
    QString       mLocalDir;
    Message       mMsg;
    Novel         mNovel;
    Preferences   mPrefs;
    Ui::Main*     mUi;

    void clearChanged() { mNovel.noChanges(); }
    void doNothing()    { }

    void closeEvent(QCloseEvent* event) override;
    void doExit();
    void doNew();
    void doOpen();
    void doPreferences();
    void doSave();
    bool doSaveAs();

    void fitWindow();
    void update();

public:
    Main(QApplication* app, QWidget* parent = nullptr);
    ~Main();

    void setupConnections();

public slots:
    void exitAction()   { doExit(); }
    void newAction()    { doNew(); }
    void openAction()   { doOpen(); }
    void saveAction()   { doSave(); }
    void saveAsAction() { doSaveAs(); }

    void preferencesAction() { doPreferences(); }
};
