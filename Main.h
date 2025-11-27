#pragma once

#include <QFont>
#include <QMainWindow>
#include <QTimer>

#include <atomic>

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
    Map<qlonglong, bool>  mById;
    Novel                 mCopy;
    qlonglong             mCurrentNode { -1 };
    QString               mDocDir;
    QRect                 mGeom;
    Map<QString, QString> mIcons;
    QString               mLocalDir;
    Message               mMsg;
    Novel                 mNovel;
    qlonglong             mPosition;
    std::atomic<bool>     mSaving { false };
    Map<qlonglong, bool>  mState;
    Preferences           mPrefs;
    QTimer                mTimer;
    Ui::Main*             mUi;

    static Main* sMain;

    void    clearChanged() { mNovel.noChanges(); }
    void    doNothing()    { }

    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

    void doAddItem();
    void doBold();
    void doCenterJustify();
    void doCopy();
    void doCut();
    void doExit();
    void doFullJustify();
    void doFullScreen();
    void doIndent();
    void doItalic();
    void doLeftJustify();
    void doNew();
    void doOpen();
    void doOutdent();
    void doPaste();
    void doPreferences();
    void doRightJustify();
    void doSave();
    bool doSaveAs();
    void doUnderline();

    static constexpr bool NoUi = true;

    void             buildTree(Item* item, QTreeWidgetItem* tree, Map<qlonglong, bool>& byId);
    QTreeWidgetItem* findItem(QTreeWidgetItem* tree, qlonglong node);
    void             fitWindow();
    void             mapTree(Map<qlonglong, bool>& byId, QTreeWidgetItem* item);
    void             save(Novel& novel, Map<qlonglong, bool>& byId, qlonglong pos, const QRect& geom, bool noUi = false);
    void             setHtml(const QString& html);
    void             setPosition(qlonglong pos);
    void             setupConnections();
    void             setupIcons();
    void             update();
    void             updateFromPrefs();
    void             updateHtml();

public:
    Main(QApplication* app, QWidget* parent = nullptr);
    ~Main();

    Ui::Main*    ui()                             { return mUi; }
    QString      getIconPath(const QString& name) { if (mIcons.contains(name)) return mIcons[name]; else return ""; }
    Preferences& prefs()                          { return mPrefs; }

    void changeNovelFont(qlonglong skip, const QFont& font);

    static Main* ptr() { return sMain; }
    static Main& ref() { return *ptr(); }

    static void  changeDocumentFont(QTextDocument* doc, const QFont& font);

public slots:
    void exitAction()   { doExit(); }
    void newAction()    { doNew(); }
    void openAction()   { doOpen(); }
    void saveAction()   { doSave(); }
    void saveAsAction() { doSaveAs(); }

    void addItemAction()       { doAddItem(); }
    void boldAction()          { doBold(); }
    void centerJustifyAction() { doCenterJustify(); }
    void copyAction()          { doCopy(); }
    void cutAction()           { doCut(); }
    void fullJustifyAction()   { doFullJustify(); }
    void indentAction()        { doIndent(); }
    void italicAction()        { doItalic(); }
    void leftJustifyAction()   { doLeftJustify(); }
    void outdentAction()       { doOutdent(); }
    void pasteAction()         { doPaste(); }
    void preferencesAction()   { doPreferences(); }
    void rightJustifyAction()  { doRightJustify(); }
    void underlineAction()     { doUnderline(); }

    void fullScreenAction() { doFullScreen(); }

};
