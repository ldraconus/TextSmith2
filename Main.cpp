#include "Main.h"
#include "ui_Main.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QScreen>
#include <QStandardPaths>
#include <QWindow>

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
            mPrefs.save();
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

    mPrefs.load();
    auto screens = QGuiApplication::screens();

    // read preferences from Local
    //  - last window place ment is also a preference item
    //  - last font used
    //  - reading voice
    //  - typing sounds
    //  - auto-save interval
    //  - theme (dark/light/follow system)
    // parse command line for novel to open
    //  - any thing on the command line is aassumed to be a novel
    //    or start with a blank novel

    // [TBD] fit window rectangle to current display setup
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

    connect(mUi->actionPrefereces, &QAction::triggered, this, &Main::preferencesAction);
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
    Json5Object prefs = Item::hasObj(obj, "Prefs", {} );
    mPrefs.read(prefs);
    mCurrentNode = Item::hasNum(obj, "Current", -1);
    // [TBD] get open/close states
    update();
}

void Main::doPreferences() {
    //
}

void Main::doSave() {
    QFileInfo info(mNovel.filename());
    QString name = info.fileName();
    mDocDir = info.absolutePath();
    Json5Object extra;
    extra["Current"] = mCurrentNode;
    mPrefs.setWindowLocation(geometry());
    extra["Prefs"] = mPrefs.write();
    // [TBD] build map of item open/close states
    mNovel.setExtra(extra);
    if (mNovel.filename().isEmpty() && !doSaveAs()) return;
    else {
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

void Main::fitWindow() {
    QRect geom = mPrefs.windowLocation();
    if (geom.isValid()) {
        QScreen* windowScreen = nullptr;
        for (auto* s: QGuiApplication::screens()) {
            if (s->geometry().intersects(geom)) {
                windowScreen = s;
                break;
            }
        }
        if (windowScreen == nullptr) windowScreen = QGuiApplication::primaryScreen();
        if (!windowScreen->geometry().contains(geom.topLeft())) geom.moveTo(windowScreen->geometry().topLeft());
        if (!windowScreen->geometry().contains(geom.bottomRight())) geom.setSize(windowScreen->geometry().size());
        QRect screenGeom = windowScreen->geometry();
        QPoint centerPos = screenGeom.center() - QPoint(geom.width() / 2, geom.height() / 2);
        geom.setTopLeft(centerPos);
        mPrefs.setWindowLocation(geom);
        windowHandle()->setGeometry(geom);
    }
}

void Main::update() {
    // update should only be called on major updates (like opening a file or starting up starting a new one)




    fitWindow();
    // save/open/close state by ID
    // clear the tree
    // go through the novel an add to the tree w/open/close states. Set unknown IDs as open
    // display the html of the current item
}
