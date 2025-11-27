#include "Main.h"
#include "ui_Main.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QScreen>
#include <QShowEvent>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QThread>
#include <QTreeWidget>
#include <QWindow>

#include "FullScreenDialog.h"
#include "PreferencesDialog.h"

constexpr qlonglong second = 1000;

Main* Main::sMain = nullptr;

void Main::closeEvent(QCloseEvent* ce) {
    if (mNovel.isChanged()) {
        mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave novel before exiting?",
                         [this]() { doSave(); },
                         [this]() { clearChanged(); },
                         [this]() { doNothing(); },
                         "Warning!");
        if (mNovel.isChanged()) {
            ce->ignore();
            return;
        }
    }
    mPrefs.setWindowLocation(geometry());
    ce->accept();
}

void Main::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    fitWindow();
}

void Main::doAddItem() {
    // pop up the item description dialog
    // if canceled: return
    // get the current action
    // create a new Item at the end current action's children
    // update the tree with the name and id of the new item at the end of the current item
}

void Main::doBold() {
    auto cursor = mUi->textEdit->textCursor();
    QTextCharFormat format;
    format.setFontWeight(cursor.charFormat().fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
    cursor.mergeCharFormat(format);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::doCenterJustify() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(Qt::AlignCenter);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::doCopy() {
    mUi->textEdit->copy();
}

void Main::doCut() {
    mUi->textEdit->cut();
}

void Main::doExit() {
    close();
}

void Main::doFullJustify() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(Qt::AlignJustify);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::doFullScreen() {
    FullScreenDialog dlg(mPrefs, this);
    dlg.setHtml(mUi->textEdit->toHtml());
    dlg.setFont(mUi->textEdit->document()->defaultFont());
    dlg.setPosition(mUi->textEdit->textCursor().position());
    dlg.showFullScreen();
    dlg.exec();
    mUi->textEdit->setHtml(dlg.html());
    auto cursor = mUi->textEdit->textCursor();
    cursor.setPosition(dlg.position());
    mUi->textEdit->setTextCursor(cursor);
    mUi->textEdit->ensureCursorVisible();
}

void Main::doIndent() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setIndent(block.indent() + 1);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::doItalic() {
    auto cursor = mUi->textEdit->textCursor();
    QTextCharFormat format;
    format.setFontItalic(!cursor.charFormat().fontItalic());
    cursor.mergeCharFormat(format);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::doLeftJustify() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(Qt::AlignLeft);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::doNew() {
    if (mNovel.isChanged()) {
        if (mNovel.isChanged()) {
            mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave this novel before starting a new Novel?",
                             [this]() { doSave(); },
                             [this]() { clearChanged(); },
                             [this]() { doNothing(); },
                             "Warning!");
            if (mNovel.isChanged()) return;
        }
    }
    mNovel.clear();
    mPosition = 0;
    mState.clear();
    clearChanged();
    update();
}

void Main::doOpen() {
    if (mNovel.isChanged()) {
        if (mNovel.isChanged()) {
            mMsg.YesNoCancel("The current file has unsaved changes.\n\nSave this novel before working a different Novel?",
                             [this]() { doSave(); },
                             [this]() { clearChanged(); },
                             [this]() { doNothing(); },
                             "Warning!");
            if (mNovel.isChanged()) return;
        }
    }

    QString filename;
    if (mNovel.filename().isEmpty()) {
        filename = QFileDialog::getOpenFileName(this, "Open Which Novel?", mDocDir, "Novels (*.novel);;All Files (*.*)");
    }

    QFileInfo info(filename);
    mDocDir = info.absolutePath();

    mNovel.clear();
    mNovel.open();
    mNovel.setFilename(filename);
    clearChanged();
    Json5Object obj = mNovel.extra();
    Json5Object prefs = Item::hasObj(obj, "Prefs", {} );
    mPrefs.read(prefs);
    mCurrentNode = Item::hasNum(obj, "Current", -1);
    mPosition = Item::hasNum(obj, "Position", 0);
    Json5Array array = Item::hasArr(obj, "State");
    mState.clear();
    for (auto& state: array) {
        if (!state.isArray()) continue;
        auto& item = state.toArray();
        int id = Item::hasNum(item, 0);
        bool expanded = Item::hasBool(item, 1);
        mState[id] = expanded;
    }
    update();
}

void Main::doOutdent() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setIndent(block.indent() - 1);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::doPaste() {
    mUi->textEdit->paste();
    auto* doc = mUi->textEdit->document();
    auto pos = mUi->textEdit->textCursor().position();
    changeDocumentFont(doc, doc->defaultFont());
    setPosition(pos);
    mUi->textEdit->ensureCursorVisible();
}

void Main::doPreferences() {
    PreferencesDialog dlg(mPrefs, this);
    if (dlg.exec() == QDialog::Rejected) return;
    updateFromPrefs();
}

void Main::doRightJustify() {
    auto cursor = mUi->textEdit->textCursor();
    auto block = cursor.blockFormat();
    block.setAlignment(Qt::AlignRight);
    cursor.setBlockFormat(block);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::doSave() {
    if (mSaving.exchange(true)) return;
    mNovel.setHtml(mCurrentNode, mUi->textEdit->toHtml());
    Map<qlonglong, bool> byId;
    mapTree(byId, mUi->treeWidget->topLevelItem(0));
    save(mNovel, byId, mUi->textEdit->textCursor().position(), geometry());
    mSaving = false;
}

bool Main::doSaveAs() {
    QString filename = QFileDialog::getSaveFileName(this, "Save the Novel as?", mDocDir, "Novels (*.novel);;All Files (*.*)");
    if (filename.isEmpty()) return false;
    mNovel.setFilename(filename);
    mNovel.change();
    doSave();
    return true;
}

void Main::doUnderline() {
    auto cursor = mUi->textEdit->textCursor();
    QTextCharFormat format;
    format.setFontUnderline(!cursor.charFormat().fontUnderline());
    cursor.mergeCharFormat(format);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::buildTree(Item* item, QTreeWidgetItem* branch, Map<qlonglong, bool>& byId) {
    auto twItem = new QTreeWidgetItem(mUi->treeWidget);
    twItem->setText(0, item->name());
    twItem->setData(0, Qt::UserRole, item->id());

    if (branch == nullptr) mUi->treeWidget->addTopLevelItem(twItem);
    else branch->addChild(twItem);

    auto& children = item->children();
    for (auto& id: item->order()) buildTree(&children[id], twItem, byId);

    if (byId.contains(item->id())) twItem->setExpanded(byId[item->id()]);
    else twItem->setExpanded(true);
}

QTreeWidgetItem* Main::findItem(QTreeWidgetItem* tree, qlonglong node) {
    if (tree == nullptr) return nullptr;
    if (auto id = tree->data(0, Qt::UserRole).toLongLong(); id == node) return tree;
    else if (tree->childCount()) {
        for (auto i = 0; i < tree->childCount(); ++i) {
            if (auto* found = findItem(tree->child(i), node); found != nullptr) return found;
        }
    }
    return nullptr;
}

void Main::fitWindow() {
    if (windowHandle() == nullptr) return;

    QRect geom = mPrefs.windowLocation();
    QScreen* windowScreen = nullptr;
    if (geom.isValid()) {
        for (auto* s: QGuiApplication::screens()) {
            if (s->geometry().intersects(geom)) {
                windowScreen = s;
                break;
            }
        }
    }
    bool needsCenter = false;
    if ((needsCenter = windowScreen == nullptr)) windowScreen = QGuiApplication::primaryScreen();

    if (!geom.isValid()) geom.setSize({ 1024, 800 });

    QRect screenGeom = windowScreen->availableGeometry();
    if (!windowScreen->geometry().contains(geom.topLeft())) geom.moveTo(screenGeom.topLeft());
    if (!windowScreen->geometry().contains(geom.bottomRight())) geom.setSize(screenGeom.size());
    if (needsCenter) {
        QPoint centerPos = screenGeom.center() - QPoint(geom.width() / 2, geom.height() / 2);
        geom.moveTopLeft(centerPos);
    }
    geom = geom.intersected(screenGeom);

    move(geom.topLeft());
    resize(geom.size());

    QRect frame = frameGeometry();
    if (!frame.contains(geom)) geom = geom.intersected(frame);

    move(geom.topLeft());
    resize(geom.size());

    mPrefs.setWindowLocation(geom);
}

void Main::mapTree(Map<qlonglong, bool>& byId, QTreeWidgetItem* item) {
    auto id = item->data(0, Qt::UserRole).toLongLong();
    bool open = item->isExpanded();
    byId[id] = open;
    for (int i = 0; i < item->childCount(); ++i) mapTree(byId, item->child(i));
}

void Main::save(Novel& novel, Map<qlonglong, bool>& byId, qlonglong pos, const QRect& geom, bool noUi) {
    if ((novel.filename().isEmpty() && !noUi) || !novel.isChanged()) return;
    QFileInfo info(mNovel.filename());
    QString name = info.fileName();
    mDocDir = info.absolutePath();
    Json5Object extra;
    extra["Current"] = mCurrentNode;
    extra["Position"] = qlonglong(pos);
    mPrefs.setWindowLocation(geom);
    extra["Prefs"] = mPrefs.write();
    mState = byId;
    Json5Array state;
    for (const auto& idState: byId) {
        Json5Array item;
        item.append(idState.first);
        item.append(idState.second);
        state.append(item);
    }
    extra["State"] = state;
    mNovel.setExtra(extra);
    if (mNovel.filename().isEmpty() && !doSaveAs()) return;
    else {
        if (!mNovel.save() && !noUi) mMsg.OK("Unable to save the file.\n\nTry and save it under a different name\nor save it to a different directory.",
                                             [this]() { doNothing(); },
                                             "Something unexpected has happened");
    }

}

void Main::setHtml(const QString& html) {
    mUi->textEdit->setHtml(html);
}

void Main::setPosition(qlonglong pos) {
    auto cursor = mUi->textEdit->textCursor();
    cursor.setPosition(pos);
    mUi->textEdit->setTextCursor(cursor);
}

void Main::update() { // new, open
    QTreeWidget* tree = mUi->treeWidget;
    tree->clear();
    buildTree(&mNovel, nullptr, mState);
    updateHtml();
    updateFromPrefs();
}

void Main::updateFromPrefs() {
    if (mPrefs.autoSave()) mTimer.start(mPrefs.autoSaveIntyerval() * second);
    else mTimer.stop();

    QFont font(mPrefs.fontFamily(), mPrefs.fontSize());
    mUi->textEdit->setCurrentFont(font);
    mUi->textEdit->document()->setDefaultFont(font);
    changeDocumentFont(mUi->textEdit->document(), QFont(font));
    setPosition(mPrefs.position());
    mUi->textEdit->ensureCursorVisible();
    changeNovelFont(mCurrentNode, QFont(font));

    switch (mPrefs.theme()) {
    case 0: mPrefs.setLightTheme();  break;
    case 1: mPrefs.setDarkTheme();   break;
    case 2: mPrefs.setSystemTheme(); break;
    }

    if (mPrefs.mainSplitter().size() != 0) {
        QList<int> arr;
        for (auto& m: mPrefs.mainSplitter()) arr.append(m.toInt());
        mUi->splitter->setSizes(arr);
    }
}

void Main::updateHtml() { // update, and post-fullscreen
    QTreeWidget* tree = mUi->treeWidget;
    QTreeWidgetItem* item = findItem(tree->topLevelItem(0), mCurrentNode);
    if (item != nullptr) {
        tree->setCurrentItem(item);
        Item* item = mNovel.findItem(mCurrentNode);
        mUi->textEdit->setHtml(item->html());
        mUi->textEdit->textCursor().position();
        mUi->textEdit->ensureCursorVisible();
    }
}

Main::Main(QApplication* app, QWidget* parent)
    : QMainWindow(parent)
    , mApp(app)
    , mAppDir(QApplication::applicationDirPath())
    , mDocDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
    , mLocalDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
    , mTimer(this)
    , mUi(new Ui::Main) {
    sMain = this;
    mUi->setupUi(this);

    mNovel.init();
    mCurrentNode = mNovel.id();

    setupIcons();

    // set up three directories: App/Doc/Local
    QDir().mkpath(mAppDir);
    QDir().mkpath(mDocDir);
    QDir().mkpath(mDocDir + "/TextSmith");
    QDir().mkpath(mLocalDir);

    setupConnections();

    mPrefs.load();

    mUi->treeWidget->setColumnCount(1);

    // parse command line for novel to open
    //  - any thing on the command line is aassumed to be a novel
    //    or start with a blank novel
    StringList arguments { app->arguments() };
    if (arguments.size() > 1) {
        mNovel.setFilename(arguments[1]);
        clearChanged();
        doOpen();
    } else doNew();
}

Main::~Main() {
    delete mUi;
}

void Main::changeNovelFont(qlonglong skip, const QFont& font) {
    mNovel.changeFont(skip, font);
}

void Main::setupConnections() {
    mUi->actionExit->setShortcut(QKeySequence("Alt+F4"));
    connect(mUi->actionExit,    &QAction::triggered, this, &Main::exitAction);
    connect(mUi->actionNew,     &QAction::triggered, this, &Main::newAction);
    connect(mUi->actionOpen,    &QAction::triggered, this, &Main::openAction);
    connect(mUi->actionSave,    &QAction::triggered, this, &Main::saveAction);
    connect(mUi->actionSave_As, &QAction::triggered, this, &Main::saveAsAction);

    connect(mUi->actionBold,               &QAction::triggered, this, &Main::boldAction);
    connect(mUi->actionCenter,             &QAction::triggered, this, &Main::centerJustifyAction);
    connect(mUi->actionCopy,               &QAction::triggered, this, &Main::copyAction);
    connect(mUi->actionCut,                &QAction::triggered, this, &Main::cutAction);
    connect(mUi->actionFull_Justification, &QAction::triggered, this, &Main::fullJustifyAction);
    connect(mUi->actionIndent,             &QAction::triggered, this, &Main::indentAction);
    connect(mUi->actionItalic,             &QAction::triggered, this, &Main::italicAction);
    connect(mUi->actionLeft_Justify,       &QAction::triggered, this, &Main::leftJustifyAction);
    connect(mUi->actionUndent,             &QAction::triggered, this, &Main::outdentAction);
    connect(mUi->actionPaste,              &QAction::triggered, this, &Main::pasteAction);
    connect(mUi->actionPrefereces,         &QAction::triggered, this, &Main::preferencesAction);
    connect(mUi->actionRight_Justify,      &QAction::triggered, this, &Main::rightJustifyAction);
    connect(mUi->actionUnderline,          &QAction::triggered, this, &Main::underlineAction);

    mUi->actionRead_To_Me->setShortcut(QKeySequence("Alt+R"));
    connect(mUi->actionDistraction_Free, &QAction::triggered, this, &Main::fullScreenAction);

    connect(mUi->boldToolButton,           &QToolButton::clicked, this, &Main::boldAction);
    connect(mUi->centerToolButton,         &QToolButton::clicked, this, &Main::doCenterJustify);
    connect(mUi->fullJustifyToolButton,    &QToolButton::clicked, this, &Main::doFullJustify);
    connect(mUi->leftJustifyToolButton,    &QToolButton::clicked, this, &Main::doLeftJustify);
    connect(mUi->increaseIndentToolButton, &QToolButton::clicked, this, &Main::indentAction);
    connect(mUi->italicToolButton,         &QToolButton::clicked, this, &Main::italicAction);
    connect(mUi->leftJustifyToolButton,    &QToolButton::clicked, this, &Main::leftJustifyAction);
    connect(mUi->decreaseIndentToolButton, &QToolButton::clicked, this, &Main::outdentAction);
    connect(mUi->rightJustifyToolButton,   &QToolButton::clicked, this, &Main::rightJustifyAction);
    connect(mUi->underlineToolButton,      &QToolButton::clicked, this, &Main::underlineAction);

    connect(&mTimer, &QTimer::timeout, this, [this]() {
                if (mSaving.exchange(true)) return;
                mNovel.setHtml(mCurrentNode, mUi->textEdit->toHtml());
                mCopy = mNovel;
                mapTree(mById, mUi->treeWidget->topLevelItem(0));
                mGeom = geometry();
                QThread* thread = QThread::create([this]() {
                                                               save(mCopy, mById, mUi->textEdit->textCursor().position(), mGeom, NoUi);
                                                               mSaving = false;
                                                           });
                connect(thread, &QThread::finished, thread, &QObject::deleteLater);
                thread->start();
            });
}

void Main::setupIcons() {
    mIcons["newItemToolButton"] =        ":/icon/newItem.ico";
    mIcons["deleteItemToolButton"] =     ":/icon/deleteItem.ico";
    mIcons["upToolButton"] =             ":/icon/moveItemUp.ico";
    mIcons["downToolButton"] =           ":/icon/moveItemDown.ico";
    mIcons["outToolButton"] =            ":/icon/moveItemOut.ico";
    mIcons["boldToolButton"] =           ":/icon/bold.ico";
    mIcons["italicToolButton"] =         ":/icon/italic.ico";
    mIcons["underlineToolButton"] =      ":/icon/underline.ico";
    mIcons["leftJustifyToolButton"] =    ":/icon/leftjustify.ico";
    mIcons["centerToolButton"] =         ":/icon/Center.ico";
    mIcons["rightJustifyToolButton"] =   ":/icon/rightJustify.ico";
    mIcons["fullJustifyToolButton"] =    ":/icon/fullJustify.ico";
    mIcons["increaseIndentToolButton"] = ":/icon/increseIndent.ico";
    mIcons["decreaseIndentToolButton"] = ":/icon/decreaseIndent.ico";
}

void Main::changeDocumentFont(QTextDocument* doc, const QFont& font) {
    QTextCursor cursor(doc);

    struct Range {
        int pos;
        int len;
    };
    QVector<Range> ranges;

    QFontMetrics metrics(font);
    int lineHeight = metrics.height();
    int indent = 2 * lineHeight;
    QTextBlockFormat toIndent;
    toIndent.setTextIndent(indent);
    toIndent.setBottomMargin(lineHeight);

    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); it != block.end(); ++it) {
            if (!it.fragment().isValid()) continue;
            const QTextFragment frag = it.fragment();
            ranges.append({ frag.position(), frag.length() });
        }

        cursor.setPosition(block.position());
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.mergeBlockFormat(toIndent);
    }

    for (const auto& range: ranges) {
        cursor.setPosition(range.pos);
        cursor.setPosition(range.pos + range.len, QTextCursor::KeepAnchor);

        QTextCharFormat fmt;
        fmt.setFontFamilies({ font.family() });
        fmt.setFontPointSize(font.pointSize());
        cursor.mergeCharFormat(fmt);
    }
}

