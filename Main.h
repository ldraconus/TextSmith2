#pragma once

#include <QFont>
#include <QMainWindow>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>

#include <atomic>

#include <List.h>
#include <Message.h>
#include <StringList.h>

#include "5th.h"
#include "EasySpelling.h"
#include "Novel.h"
#include "Preferences.h"
#include "Printer.h"
#include "SearchReplace.h"
#include "Speech.h"
#include "TextEdit.h"
#include "Words.h"

class QApplication;
class QCloseEvent;
class QShortcut;
class QShowEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
    class Main;
}
QT_END_NAMESPACE

class QCompleter;
class QPdfWriter;
class QPrinter;
class QTextDocument;
class QTreeWidgetItem;
class Ui_SpellWidget;
class Ui_Widget;
namespace fifth {
class vm;
}

class Main: public QMainWindow {
    Q_OBJECT

public:
    class WordCounts {
    private:
        qlonglong mCurrentItem { 0 };
        qlonglong mOpen        { 0 };
        qlonglong mLastCounted { 0 };
        qlonglong mTotal       { 0 };

    public:
        auto currentItem()      { return mCurrentItem; }
        auto sinceLastCounted() { return mLastCounted; }
        auto sinceOpened()      { return mOpen; }
        auto total()            { return mTotal; }

        void setCurrentItem(qlonglong c)      { mCurrentItem = c; }
        void setSinceLastCounted(qlonglong c) { mLastCounted = c; }
        void setSinceOpened(qlonglong c)      { mOpen = c; }
        void setTotal(qlonglong c)            { mTotal = c; }
    };

private:
    struct WordPos {
        QString   mWord;
        qlonglong mAt;
        qlonglong mNext;
        bool      mUpperCase;
    };

    enum class StartingFlag { Starting, Continuing };

    List<QAction*>                mActions;
    QApplication*                 mApp { nullptr };
    QString                       mAppDir;
    Map<qlonglong, bool>          mById;
    QCompleter*                   mCompleter { nullptr };
    Novel                         mCopy;
    qlonglong                     mCurrentNode { -1 };
    QString                       mDocDir;
    int                           mDocMetaType;
    Ui_Widget*                    mFindWidget { nullptr };
    QWidget*                      mFindLine { nullptr };
    QRect                         mGeom;
    Map<QString, QString>         mIcons;
    Map<QString, QImage>          mImageStore;
    QString                       mLocalDir;
    Message                       mMsg;
    Novel                         mNovel;
    qlonglong                     mPosition;
    Preferences                   mPrefs;
    bool                          mPrefsLoaded { false };
    Printer*                      mPrinter { nullptr };
    QTextCursor                   mSavedCursor;
    std::atomic<bool>             mSaving { false };
    SearchCore*                   mSearch { nullptr };
    Map<QShortcut*, fifth::value> mShortcut;
    SoundPool                     mSoundPool;
    Speech                        mSpeech;
    bool                          mSpeechAvailable { false };
    EasySpelling                  mSpelling;
    Ui_SpellWidget*               mSpellWidget { nullptr };
    QWidget*                      mSpellcheck { nullptr };
    WordPos                       mSpellcheckWord;
    Map<qlonglong, bool>          mState;
    QList<qlonglong>              mSpellcheckIds;
    qlonglong                     mSpellcheckPosition { -1 };
    QToolButton*                  mStopButton { nullptr };
    QString                       mTextToSpeak;
    QTimer                        mTimer;
    Ui::Main*                     mUi { nullptr };
    fifth::vm*                    mVm { nullptr };
    WordCounts                    mWordCount;

    static Main* sMain;

    void       changed()      { mNovel.change(); }
    void       clearChanged() { mNovel.noChanges(); }
    void       doNothing()    { }
    fifth::vm* vm()           { return mVm; }

    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

    void doAboutToShowFileMenu();
    void doAboutToShowEditMenu();
    void doAboutToShowNovelMenu();
    void doAboutToShowViewMenu();
    void doAboutToShowHelpMenu();
    void doAddItem();
    void doBold();
    void doCenterJustify();
    void doCloseAll();
    void doCopy();
    void doCursorPositionChanged();
    void doCut();
    void doEditItem();
    void doExit();
    void doExport(const QString& name);
    void doFindChanged();
    void doFindDone();
    void doFindNext();
    void doFindReplace();
    void doFocusChanged(QWidget *old, QWidget *now);
    void doFullJustify();
    void doFullScreen();
    void doHighlight(const QString& text, qlonglong start);
    void doIndent();
    void doItalic();
    void doItemChanged(QTreeWidgetItem* current);
    void doLeftJustify();
    void doLowercase();
    void doMoveDown();
    void doMoveOut();
    void doMoveUp();
    void doNew();
    void doOpenAll();
    void doOpen();
    void doOutdent();
    void doPageSetup();
    void doPaste();
    void doPreferences();
    void doPrint();
    void doPrintNovel(QPrinter* printer);
    void doReadToMe();
    void doRedo();
    void doRemoveItem();
    void doReplace();
    void doReplaceAll();
    void doReplaceChanged();
    void doRightJustify();
    void doSave();
    bool doSaveAs();
    void doSpellcheck();
    void doSpellcheckAddWord(const QString& word);
    void doSpellcheckDone();
    void doSpellcheckLeft();
    void doSpellcheckNext();
    void doSpellcheckRight();
    void doStop(bool stopped = false);
    void doSuggestion(const QString& from, const QString& to);
    void doTextChanged();
    void doUnderline();
    void doUndo();
    void doUppercase();
    void doWordCount();

    static constexpr bool NoUi = true;

    void      addActionToMenuPath(QMenuBar* bar, QAction* action, const QString& path);
    void      buildTree(const TreeNode& branch, QTreeWidgetItem* tree, Map<qlonglong, bool>& byId);
    void      buildTreeMimeData(const QList<QTreeWidgetItem*>& item, QMimeData* mimeData);
    bool      canPasteMimeData(const QMimeData* mimeData);
    QString   checked(const QString& path);
    void      copyTreeItem();
    void      cutTreeItem();
    QString   findKey(QString& used, const QString& name, bool secondPass = false);
    void      fitWindow();
    void      footer(QPainter& painter, QPrinter* printer);
    void      header(QPainter& painter, QPrinter* printer);
    void      justifyButtons();
    void      loadScripts();
    void      mapTree(Map<qlonglong, bool>& byId, QTreeWidgetItem* item);
    WordPos   nextWord(const QString& str, qlonglong pos = 0);
    bool      nothingAbove();
    bool      nothingBelow();
    bool      parentIsRoot();
    void      pasteTreeItem();
    bool      receiveTreeMimeData(QDropEvent* de, const QMimeData* mimeData);
    void      registerImages(const QString& html, QTextDocument* doc);
    void      replaceText(QTextCursor cursor, const QString& text);
    void      save(Novel& novel, Map<qlonglong, bool>& byId, qlonglong pos, const QRect& geom, bool noUi = false);
    TreeNode  saveTree(QTreeWidgetItem* node);
    QSize     scrollBarSize(QScrollBar* bar);
    void      setHtml(const QString& html);
    void      setIcon(QToolButton* button, bool isChecked);
    void      setMenu(QAction* button, bool isChecked);
    void      setPosition(qlonglong pos);
    void      setupActions();
    void      setupConnections();
    void      setupScripting();
    void      setupIcons();
    void      setupTabOrder();
    qlonglong skipSpaces(const QString& str, qlonglong pos);;
    void      update();
    void      updateFromPrefs();
    void      updateHtml();
    void      workTree(QTreeWidgetItem* item, bool expand);


public:
    Main(QApplication* app, QWidget* parent = nullptr);
    ~Main();

    static constexpr bool WithAlignment =    true;
    static constexpr bool WithoutAlignment = false;

    void         busy()                           { QApplication::setOverrideCursor(Qt::WaitCursor); }
    qlonglong    currentNode()                    { return mCurrentNode; }
    QString      docDir()                         { return mDocDir; }
    Item&        fifthItem(fifth::stack& user)    { return mNovel.fifthItem(user); }
    Message&     msg()                            { return mMsg; }
    QString      getIconPath(const QString& name) { if (mIcons.contains(name)) return mIcons[name]; else return ""; }
    Novel&       novel()                          { return mNovel; }
    Preferences& prefs()                          { return mPrefs; }
    bool         prefsLoaded()                    { return mPrefsLoaded; }
    void         print(QPrinter* printer)         { return doPrintNovel(printer); }
    Printer*     printer()                        { return mPrinter; }
    void         ready()                          { QApplication::restoreOverrideCursor(); }
    void         setDocDir(const QString& d)      { mDocDir = d; }
    void         setPrinter(Printer* p)           { mPrinter = p; }
    Ui::Main*    ui()                             { return mUi; }
    WordCounts&  wordCount()                      { return mWordCount; }

    void               buildDrag(QTreeWidgetItem* branch, QMimeData* mime);
    QTreeWidgetItem*   buildTreeFromJson(Json5Object& obj);
    void               changeNovelFont(const QFont& font);
    StringList         createParagraphs(Item& item, bool align = WithoutAlignment);
    QTreeWidgetItem*   findItem(QTreeWidgetItem* tree, qlonglong node);
    int                handleCover(Item& item,
                                   int pageNo,
                                   bool startingPage,
                                   QSizeF& pageSize,
                                   QMarginsF& margins,
                                   QPainter& painter,
                                   std::function<void()> printer);
    void               loadImageBytes(const QImage& image, std::function<void(QByteArray)> callback);
    void               loadImageBytesFromUrl(const QUrl& url, std::function<void(QByteArray)> callback);
    StringList         mergeWordFragments(QList<Words::Word>& words);
    void               newPage(QPainter& painter, std::function<void()> printer);
    void               outputNovel(QList<qlonglong> ids,
                                   const QString& chapterTag,
                                   const QString& sceneTag,
                                   const QString& coverTag,
                                   QPainter& painter,
                                   QSizeF pageSize,
                                   QMarginsF margins,
                                   std::function<void()> pager);
    QList<Words::Word> paragraphWords(const QString& paragrapth);
    void               removeEmptyFirstBlock(TextEdit* text);
    void               setupHtml(TextEdit& text);
    Json5Object        treeOfItems(QTreeWidgetItem* branch);
    QList<Item*>       vectorOfItems(QTreeWidgetItem* branch);
    QList<qlonglong>   vectorOfIds(QTreeWidgetItem* branch,
                                   const StringList& tags,
                                   StartingFlag starting = StartingFlag::Starting);
    void               wordCounts();

    static Main* ptr() { return sMain; }
    static Main& ref() { return *ptr(); }

    static void  changeDocumentFont(QTextDocument* doc, const QFont& font);

public slots:
    void exitAction()   { doExit(); }
    void newAction()    { doNew(); }
    void openAction()   { doOpen(); }
    void pageSetup()    { doPageSetup(); }
    void printAction()  { doPrint(); }
    void saveAction()   { doSave(); }
    void saveAsAction() { doSaveAs(); }

    void boldAction()          { doBold(); }
    void centerJustifyAction() { doCenterJustify(); }
    void copyAction()          { doCopy(); }
    void cutAction()           { doCut(); }
    void findNext()            { doFindNext(); }
    void findReplace()         { doFindReplace(); }
    void fullJustifyAction()   { doFullJustify(); }
    void indentAction()        { doIndent(); }
    void italicAction()        { doItalic(); }
    void leftJustifyAction()   { doLeftJustify(); }
    void lowerCaseAction()     { doLowercase(); }
    void outdentAction()       { doOutdent(); }
    void pasteAction()         { doPaste(); }
    void preferencesAction()   { doPreferences(); }
    void redoAction()          { doRedo(); }
    void rightJustifyAction()  { doRightJustify(); }
    void underlineAction()     { doUnderline(); }
    void undoAction()          { doUndo(); }
    void uppercaseAction()     { doUppercase(); }

    void addItemAction()    { doAddItem(); }
    void closeAllAction()   { doCloseAll(); }
    void editItemAction()   { doEditItem(); }
    void actionMoveDown()   { doMoveDown(); }
    void actionMoveOut()    { doMoveOut(); }
    void actionMoveUp()     { doMoveUp(); }
    void openAllAction()    { doOpenAll(); }
    void removeItemAction() { doRemoveItem(); }

    void fullScreenAction() { doFullScreen(); }
    void readToMeAction()   { doReadToMe(); }
    void spellCheck()       { doSpellcheck(); }
    void wordCountAction()  { doWordCount(); }

    void aboutToShowFileMenuAction()  { doAboutToShowFileMenu(); }
    void aboutToShowEditMenuAction()  { doAboutToShowEditMenu(); }
    void aboutToShowNovelMenuAction() { doAboutToShowNovelMenu(); }
    void aboutToShowViewMenuAction()  { doAboutToShowViewMenu(); }
    void aboutToShowFileHelpAction()  { doAboutToShowHelpMenu(); }

    void printNovel(QPrinter* p)      { doPrintNovel(p); }

    void focusChanged(QWidget *old, QWidget *now) { doFocusChanged(old, now); }

    void currentItemChangedAction(QTreeWidgetItem* current, QTreeWidgetItem*) { doItemChanged(current); }
    void doubleClickAction(QTreeWidgetItem*, int)                             { doEditItem(); }

    void cursorPositionChanged() { doCursorPositionChanged(); }
    void textChangedAction()     { doTextChanged(); }

    void findChanged()                                   { doFindChanged(); }
    void barDone()                                       { doFindDone(); doSpellcheckDone(); }
    void highlight(const QString& text, qlonglong start) { doHighlight(text, start); }
    void replace()                                       { doReplace(); }
    void replaceAll()                                    { doReplaceAll(); }
    void replaceChanged()                                { doReplaceChanged(); }
    void stop()                                          { doStop(); }
    void stopped()                                       { doStop(true); }

    void scrollSpellcheckLeft()  { doSpellcheckLeft(); }
    void scrollSpellcheckRight() { doSpellcheckRight(); }
    void spellcheckAddWord()     { doSpellcheckAddWord(mSpellcheckWord.mWord); }
    void spellcheckDone()        { doSpellcheckDone(); }
    void spellcheckNext()        { doSpellcheckNext(); }
    void suggestionAction()      { doSuggestion(mSpellcheckWord.mWord, dynamic_cast<QPushButton*>(sender())->text()); }
};
