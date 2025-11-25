#pragma once

#include <QFont>
#include <QMainWindow>

#include <StringList.h>
#include <Message.h>

#include "Novel.h"
#include "Preferences.h"

class QApplication;
class QCloseEvent;
class QShowEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
    class Main;
}
QT_END_NAMESPACE

class QTextDocument;
class QTreeWidgetItem;

class Main: public QMainWindow {
    Q_OBJECT

private:
    QApplication*         mApp;
    QString               mAppDir;
    qlonglong             mCurrentNode { -1 };
    QString               mDocDir;
    Map<QString, QString> mIcons;
    QString               mLocalDir;
    Message               mMsg;
    Novel                 mNovel;
    qlonglong             mPosition;
    Map<qlonglong, bool>  mState;
    Preferences           mPrefs;
    Ui::Main*             mUi;

    static Main* sMain;

    void    clearChanged() { mNovel.noChanges(); }
    void    doNothing()    { }

    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

    void doExit();
    void doNew();
    void doOpen();
    void doPreferences();
    void doSave();
    bool doSaveAs();

    void             buildTree(Item* item, QTreeWidgetItem* tree, Map<qlonglong, bool>& byId);
    QTreeWidgetItem* findItem(QTreeWidgetItem* tree, qlonglong node);
    void             fitWindow();
    void             mapTree(Map<qlonglong, bool>& byId, QTreeWidgetItem* item);
    void             update();
    void             updateFromPrefs();

public:
    Main(QApplication* app, QWidget* parent = nullptr);
    ~Main();

    Ui::Main*    ui()                             { return mUi; }
    QString      getIconPath(const QString& name) { if (mIcons.contains(name)) return mIcons[name]; else return ""; }
    Preferences& prefs()                          { return mPrefs; }

    void changeNovelFont(qlonglong skip, const QFont& font);
    void setupConnections();

    static Main* ptr() { return sMain; }
    static Main& ref() { return *ptr(); }

    static void  changeDocumentFont(QTextDocument* doc, const QFont& font);

public slots:
    void exitAction()   { doExit(); }
    void newAction()    { doNew(); }
    void openAction()   { doOpen(); }
    void saveAction()   { doSave(); }
    void saveAsAction() { doSaveAs(); }

    void preferencesAction() { doPreferences(); }
};
