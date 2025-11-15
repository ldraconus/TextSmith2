#include "Main.h"
#include "ui_Main.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>

/*
    textEdit->setStyleSheet(
        "QTextEdit {"
        "   background-color: #f0f8ff;"   // Light blue background
        "   color: #003366;"              // Dark blue text
        "   font-family: 'Courier New';"
        "   font-size: 14px;"
        "   border: 2px solid #003366;"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "}"
    );
 */

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
    ce->accept();
}

Main::Main(QApplication* app, QWidget* parent)
    : QMainWindow(parent)
    , mApp(app)
    , mAppDir(QApplication::applicationDirPath())
    , mDocDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
    , mLocalDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
    , mUi(new Ui::Main) {
    mUi->setupUi(this);

    // set up three directories: App/Doc/Local
    QDir().mkpath(mAppDir);
    QDir().mkpath(mDocDir);
    QDir().mkpath(mDocDir + "/TextSmith");
    QDir().mkpath(mLocalDir);

    // read preferences from Local
    // parse command line for novel to open
    // or start with a blank novel

    setupConnections();
}

Main::~Main() {
    delete mUi;
}

void Main::setupConnections() {
    connect(mUi->actionExit,    &QAction::triggered, this, &Main::exitAction);
    connect(mUi->actionNew,     &QAction::triggered, this, &Main::newAction);
    connect(mUi->actionOpen,    &QAction::triggered, this, &Main::openAction);
    connect(mUi->actionSave,    &QAction::triggered, this, &Main::saveAction);
    connect(mUi->actionSave_As, &QAction::triggered, this, &Main::saveAsAction);
}

void Main::doExit() {
    close();
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
    mNovel.noChanges();
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
    mNovel.noChanges();
    Json5Object obj = mNovel.extra();
    mCurrentNode = Item::hasNum(obj, "Current", -1);
    Json5Array arr = Item::hasArr(obj, "At", {});
    if (arr.size() == 4) {
        auto old = geometry();
        auto x = Item::hasNum(arr, 0, old.x());
        auto y = Item::hasNum(arr, 1, old.y());
        auto width = Item::hasNum(arr, 2, old.width());
        auto height = Item::hasNum(arr, 3, old.height());
        // [TBD] make the window fit on current windows
        QRect geom(x, y, width, height);
        setGeometry(geom);
    }
    // [TBD] set open/close states
    update();
}

void Main::doSave() {
    QFileInfo info(mNovel.filename());
    QString name = info.fileName();
    mDocDir = info.absolutePath();
    Json5Object extra;
    extra["Current"] = mCurrentNode;
    Json5Array arr;
    auto geom = geometry();
    arr.append(qlonglong(geom.x()));
    arr.append(qlonglong(geom.y()));
    arr.append(qlonglong(geom.width()));
    arr.append(qlonglong(geom.height()));
    extra["At"] = arr;
    // [TBD] build map of item open/close states
    mNovel.setExtra(extra);
    if (mNovel.filename().isEmpty() && !doSaveAs()) return;
    else {
        // set the d
        if (!mNovel.save()) mMsg.OK("Unabkle to save the file.\n\nTry and save it under a different name",
                                    [this]() { doNothing(); },
                                    "Something unexpected has happened");
    }
}

bool Main::doSaveAs() {
    QString filename = QFileDialog::getSaveFileName(this, "Save the Novel as?", mDocDir, "Novels (*.novel);;All Files (*.*)");
    if (filename.isEmpty()) return false;
    mNovel.setFilename(filename);
    doSave();
    return true;
}

void Main::update() {
    // save/open/close state by ID
    // clear the tree
    // go through the novel an add to the tree w/open/close states. Set unknown IDs as open
    // display the html of the current item
}
