#include "Preferences.h"
#include "Main.h"
#include "Novel.h"
#include "PreferencesDialog.h"
#include "ui_Main.h"
#include "ui_PreferencesDialog.h"

#include <QApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

#include <Json5.h>
#include <StringList.h>

constexpr auto ChapterTag       { "ChapterTag" };
constexpr auto Company          { "SoftwareOnHand" };
constexpr auto Program          { "TextSmith" };
constexpr auto AutoSave         { "AutoSave" };
constexpr auto AutoSaveInterval { "AutoSave Interval" };
constexpr auto FontFamily       { "FontFasmily" };
constexpr auto FontSize         { "FontSize" };
constexpr auto MainSplitter     { "MainSplitter" };
constexpr auto OtherSplitter    { "OtherSplitter" };
constexpr auto Position         { "Position" };
constexpr auto SceneTag         { "SceneTag" };
constexpr auto Theme            { "Theme" };
constexpr auto TypingSounds     { "TypingSounds" };
constexpr auto Voice            { "Voice" };
constexpr auto WindowLoc        { "WindowLoc" };

constexpr auto DefaultChapterTag { "chapter" };
constexpr auto DefaultFont       { "Segoe UI" };
constexpr auto DefaultFontSize   { 9 };
constexpr auto DefaultInterval   { 5 * 60 };
const     auto DefaultOther      { QJsonArray() };
constexpr auto DefaultPosition   { 0 };
const     auto DefaultSplitter   { QJsonArray() };
constexpr auto DefaultSave       { false };
constexpr auto DefaultSceneTag   { "scene" };
constexpr auto DefaultSounds     { false };
constexpr auto DefaultTheme      { 2 };
constexpr auto DefaultVoice      { 0 };

bool Preferences::load() {
    QSettings settings(Company, Program);
    mAutoSave =         settings.value(AutoSave,         DefaultSave).toBool();
    mAutoSaveInterval = settings.value(AutoSaveInterval, DefaultInterval).toLongLong();
    mChapterTag =       settings.value(ChapterTag,       DefaultChapterTag).toString();
    mFontFamily =       settings.value(FontFamily,       DefaultFont).toString();
    mFontSize =         settings.value(FontSize,         DefaultFontSize).toInt();
    QJsonArray arr =    settings.value(MainSplitter,     DefaultSplitter).toJsonArray();
    mMainSplitter.clear();
    for (auto i = 0; i < arr.size(); ++i) mMainSplitter.append(qlonglong(arr[i].toInt(-1)));
    QJsonArray other =  settings.value(OtherSplitter,    DefaultOther).toJsonArray();
    mOtherSplitter.clear();
    for (auto i = 0; i < other.size(); ++i) mOtherSplitter.append(qlonglong(other[i].toInt(-1)));
    mPosition =         settings.value(Position,         DefaultPosition).toLongLong();
    mSceneTag =         settings.value(SceneTag,         DefaultSceneTag).toString();
    mTheme =            settings.value(Theme,            DefaultTheme).toInt();
    mTypingSounds =     settings.value(TypingSounds,     DefaultSounds).toBool();
    mVoice =            settings.value(Voice,            DefaultVoice).toInt();
    mWindow =           settings.value(WindowLoc,        { }).toRect();
    if (mWindow.height() < 1 || mWindow.width() < 1) mWindow.setRect(0, 0, -1, -1);
    return true;
}

QString Preferences::newPath(const QString& path) {
    StringList parts(path.split("/"));
    QString& last = parts.last();
    last = "Lt" + last;
    return parts.join("/");
}

bool Preferences::read(Json5Object& obj) {
    Json5Array arr;
    if (obj.contains("v1")) {
        Json5Object prefs = Item::hasObj(obj, "options", {});
        mAutoSave =         Item::hasBool(prefs, "autoSave",         DefaultSave);
        mAutoSaveInterval = Item::hasNum(prefs,  "autoSaveInerval",  DefaultInterval);
        mTypingSounds =     Item::hasBool(prefs, "typewriterSounds", DefaultSounds);
        QString font =      Item::hasStr(prefs,  "textFont",         DefaultFont + (":" + QString::number(DefaultFontSize)));
        StringList parts { font.split(":") };
        if (parts.size() == 2) {
            mFontFamily = parts[0];
            bool ok = true;
            mFontSize   = parts[1].toInt(&ok);
            if (!ok || mFontSize < 1) mFontSize = DefaultFontSize;
        } else {
            mFontFamily = DefaultFont;
            mFontSize   = DefaultFontSize;
        }
        mVoice =             Item::hasNum(obj,  "voice",           DefaultVoice);
        mChapterTag = DefaultChapterTag;
        mSceneTag = DefaultSceneTag;
        mTheme = DefaultTheme;
        mPosition = DefaultPosition;
        Json5Object window = Item::hasObj(obj, "windows", {});
        Json5Object mainWin = Item::hasObj(window, "mainwindow", { });
        Json5Array arr = Item::hasArr(mainWin, "splitter", {});;
        mMainSplitter.clear();
        for (auto& i: arr) {
            if (!i.isNumber()) continue;
            mMainSplitter.append(i.toInt());
        }
        Json5Object fullScr = Item::hasObj(window, "fullscreen", { });
        arr = Item::hasArr(fullScr, "sizes", { });
        mOtherSplitter.clear();
        for (auto& i: arr) {
            if (!i.isNumber()) continue;
            mOtherSplitter.append(i.toInt());
        }
        arr = Item::hasArr(obj, "windoLoc", { });
    } else {
        mAutoSave =         Item::hasBool(obj, AutoSave,         DefaultSave);
        mAutoSaveInterval = Item::hasNum(obj,  AutoSaveInterval, DefaultInterval);
        mChapterTag =       Item::hasStr(obj,  ChapterTag,       DefaultChapterTag);
        mFontFamily =       Item::hasStr(obj,  FontFamily,       DefaultFont);
        mFontSize =         Item::hasNum(obj,  FontSize,         DefaultFontSize);
        mPosition =         Item::hasNum(obj,  Position,         DefaultPosition);
        mSceneTag =         Item::hasStr(obj,  SceneTag,         DefaultSceneTag);
        mTheme =            Item::hasNum(obj,  Theme,            DefaultTheme);
        mTypingSounds =     Item::hasBool(obj, TypingSounds,     DefaultSounds);
        mVoice =            Item::hasBool(obj, Voice,            DefaultVoice);
        arr =               Item::hasArr(obj,  WindowLoc,        { });
        arr = Item::hasArr(obj, MainSplitter, { });
        mMainSplitter.clear();
        for (auto& i: arr) {
            if (!i.isNumber()) continue;
            mMainSplitter.append(i.toInt());
        }
        arr = Item::hasArr(obj, OtherSplitter, { });
        mOtherSplitter.clear();
        for (auto& i: arr) {
            if (!i.isNumber()) continue;
            mOtherSplitter.append(i.toInt());
        }
    }
    QRect geom;
    geom.setX(Item::hasNum(arr, 0, 0));
    geom.setY(Item::hasNum(arr, 1, 0));
    geom.setWidth(Item::hasNum(arr, 2, -1));
    geom.setHeight(Item::hasNum(arr, 3, -1));
    mWindow = geom;
    if (mWindow.height() < 1 || mWindow.width() < 1) mWindow.setRect(0, 0, -1, -1);
    return true;
}

bool Preferences::save() {
    QSettings settings(Company, Program);
    settings.setValue(AutoSave,         mAutoSave);
    settings.setValue(AutoSaveInterval, mAutoSaveInterval);
    settings.setValue(ChapterTag,       mChapterTag);
    settings.setValue(FontFamily,       mFontFamily);
    settings.setValue(FontSize,         mFontSize);
    QJsonArray arr;
    for (auto& a: mMainSplitter) arr.append(a.toInt());
    settings.setValue(MainSplitter,     arr);
    QJsonArray otr;
    for (auto& a: mOtherSplitter) otr.append(a.toInt());
    settings.setValue(OtherSplitter,    otr);
    settings.setValue(Position,         mPosition);
    settings.setValue(SceneTag,         mSceneTag);
    settings.setValue(Theme,            mTheme);
    settings.setValue(TypingSounds,     mTypingSounds);
    settings.setValue(Voice,            mVoice);
    settings.setValue(WindowLoc,        mWindow);
    return true;
}
void Preferences::setDarkTheme() {
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(42,42,42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(66,66,66));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::black);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Mid, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::Dark, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Highlight, QColor(53,53,53).lighter());
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    mApp->setPalette(darkPalette);
    QFont font = mApp->font();
    font.setBold(false);
    if (Main::ptr() != nullptr) {
        Main::ref().ui()->treeWidget->setStyleSheet("QTreeView {\n"
                                                    "    color: white;\n"
                                                    "    background-color: #2A2A2A;\n"
                                                    "}"
                                                    "QTreeView::branch:has-children:!has-siblings:closed,\n"
                                                    "QTreeView::branch:closed:has-children:has-siblings  {\n"
                                                    "    border-image: none;\n"
                                                    "    image: url(:/imgs/DarkClosed.png);\n"
                                                    "}\n"
                                                    "\n"
                                                    "QTreeView::branch:open:has-children:!has-siblings,\n"
                                                    "QTreeView::branch:open:has-children:has-siblings  {\n"
                                                    "    border-image: none;\n"
                                                    "    image: url(:/imgs/DarkOpen.png);\n"
                                                    "}");
        Main::ref().ui()->textEdit->setStyleSheet("QTextEdit {\n"
                                                  "    color: white;\n"
                                                  "    background-color: black;\n"
                                                  "}");
        Main::ref().ui()->menubar->setStyleSheet("QMenuBar {\n"
                                                 "    color: white;\n"
                                                 "    background-color: #2A2A2A"
                                                 "}");
        QString menuStyle {
            "QMenu {\n"
            "    background-color: #2A2A2A;\n"
            "    color: white;\n"
            "    border: 1px solid #444;\n"
            "}\n"
            "\n"
            "QMenu::item {\n"
            "    padding: 6px 20px;\n"
            "    background: transparent;\n"
            "}\n"
            "\n"
            "QMenu::item:selected {\n"
            "    background: #444;\n"
            "}\n"
            "QMenu::item:disabled {\n"
            "    color: #888;\n"
            "}\n"
        };
        Main::ref().ui()->menuFile->setStyleSheet(menuStyle);
        Main::ref().ui()->menuEdit->setStyleSheet(menuStyle);
        Main::ref().ui()->menuFont->setStyleSheet(menuStyle);
        Main::ref().ui()->menuParagraph->setStyleSheet(menuStyle);
        Main::ref().ui()->menuNovel->setStyleSheet(menuStyle);
        Main::ref().ui()->menuView->setStyleSheet(menuStyle);
        Main::ref().ui()->menuHelp->setStyleSheet(menuStyle);
    }
    if (PreferencesDialog::ptr() != nullptr) {
        PreferencesDialog::ref().ui()->textEdit->setStyleSheet("QTextEdit {\n"
                                                               "    color: white;\n"
                                                               "    background-color: black;\n"
                                                               "}");
    }
    mIsDark = true;
    resetIcons(true);
}

void Preferences::setLightTheme() {
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(160, 160, 160));
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, QColor(132, 132, 132));
    lightPalette.setColor(QPalette::AlternateBase, QColor(132, 132, 132));
    lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    lightPalette.setColor(QPalette::Mid, QColor(135, 135, 135));
    lightPalette.setColor(QPalette::Dark, QColor(100, 100, 100));
    lightPalette.setColor(QPalette::Button, QColor(180, 180, 180));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::BrightText, Qt::red);
    lightPalette.setColor(QPalette::Highlight, QColor(160, 160, 160).darker());
    lightPalette.setColor(QPalette::HighlightedText, Qt::darkGray);
    mApp->setPalette(lightPalette);
    QFont font = mApp->font();
    font.setBold(true);
    if (Main::ptr() != nullptr) {
        Main::ref().ui()->treeWidget->setStyleSheet("QTreeView::item {\n"
                                                    "    color: black;\n"
                                                    "    background-color: #B4B4B4;\n"
                                                    "    selection-color: black;\n"
                                                    "    selection-background-color: white;\n"
                                                    "}"
                                                    "QTreeView::branch:has-children:!has-siblings:closed,\n"
                                                    "QTreeView::branch:closed:has-children:has-siblings {\n"
                                                    "    border-image: none;\n"
                                                    "    image: url(:/imgs/Closed.png);\n"
                                                    "}\n"
                                                    "\n"
                                                    "QTreeView::branch:open:has-children:!has-siblings,\n"
                                                    "QTreeView::branch:open:has-children:has-siblings  {\n"
                                                    "    border-image: none;\n"
                                                    "    image: url(:/imgs/Open.png);\n"
                                                    "}");
        Main::ref().ui()->textEdit->setStyleSheet("QTextEdit {\n"
                                                  "    color: black;\n"
                                                  "    background-color: white;\n"
                                                  "}");
        Main::ref().ui()->menubar->setStyleSheet("QMenuBar {\n"
                                                 "    color: black;\n"
                                                 "    background-color: #BEBEBE;\n"
                                                 "}");
        QString menuStyle {
            "QMenu {\n"
            "    background-color: #BEBEBE;\n"
            "    color: black;\n"
            "    border: 1px solid #999;\n"
            "}\n"
            "\n"
            "QMenu::item {\n"
            "    padding: 6px 20px;\n"
            "    background: transparent;\n"
            "}\n"
            "\n"
            "QMenu::item:selected {\n"
            "    background: #999;\n"
            "}\n"
            "QMenu::item:disabled {\n"
            "    color: #888;\n"
            "}\n"
        };
        Main::ref().ui()->menuFile->setStyleSheet(menuStyle);
        Main::ref().ui()->menuEdit->setStyleSheet(menuStyle);
        Main::ref().ui()->menuFont->setStyleSheet(menuStyle);
        Main::ref().ui()->menuParagraph->setStyleSheet(menuStyle);
        Main::ref().ui()->menuNovel->setStyleSheet(menuStyle);
        Main::ref().ui()->menuView->setStyleSheet(menuStyle);
        Main::ref().ui()->menuHelp->setStyleSheet(menuStyle);
    }
    if (PreferencesDialog::ptr() != nullptr) {
        PreferencesDialog::ref().ui()->textEdit->setStyleSheet("QTextEdit {\n"
                                                               "    color: black;\n"
                                                               "    background-color: white;\n"
                                                               "}");
    }
    mIsDark = false;
    resetIcons(false);
}

QString Preferences::checkPath(const QString& path, bool checked) {
    QString work = path;
    if (checked) {
        StringList parts(work.split("."));
        parts[0] += "Ck";
        work = parts.join(".");
    }
    return work;
}

bool Preferences::isDark() {
    QSettings s("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                QSettings::NativeFormat);
    return s.value("AppsUseLightTheme", 1).toInt() == 0;
}

void Preferences::resetIcons(bool isDark) {
    for (auto* button: Main::ref().findChildren<QToolButton*>()) {
        QString path = Main::ref().getIconPath(button->objectName());
        if (!isDark) path = newPath(path);
        QPixmap pm(path);
        QIcon icon;
        icon.addFile(path, QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        if (button->isCheckable()) path = checkPath(path, button->isChecked());
        QPixmap cpm(path);
        icon.addFile(path, QSize(), QIcon::Mode::Normal, QIcon::State::On);
        button->setIcon(icon);
        button->setIconSize(QSize(32, 32));
        button->setStyleSheet("QToolButton {\n"
                              "    background-color: palette(Button);\n"
                              "}");
    }
}

void Preferences::setSystemTheme() {
    mIsDark = isDark();
    if (mIsDark) setDarkTheme();
    else setLightTheme();

    resetIcons(mIsDark);
}

Json5Object Preferences::write() {
    Json5Object obj;
    obj[AutoSave] =         mAutoSave;
    obj[AutoSaveInterval] = mAutoSaveInterval;
    obj[ChapterTag] =       mChapterTag;
    obj[FontFamily] =       mFontFamily;
    obj[FontSize] =         mFontSize;
    obj[MainSplitter] =     mMainSplitter;
    obj[OtherSplitter] =    mOtherSplitter;
    obj[Position] =         mPosition;
    obj[SceneTag] =         mSceneTag;
    obj[Theme] =            mTheme;
    obj[TypingSounds] =     mTypingSounds;
    obj[Voice] =            mVoice;
    Json5Array arr;
    arr.append(qlonglong(mWindow.x()));
    arr.append(qlonglong(mWindow.y()));
    arr.append(qlonglong(mWindow.width()));
    arr.append(qlonglong(mWindow.height()));
    obj[WindowLoc] =        arr;

    return obj;
}
