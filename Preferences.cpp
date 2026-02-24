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

constexpr auto ActingScripts    { "ActingScripts" };
constexpr auto ChapterTag       { "ChapterTag" };
constexpr auto Company          { "SoftwareOnHand" };
constexpr auto CoverTag         { "CoverTag" };
constexpr auto Program          { "TextSmith" };
constexpr auto AutoSave         { "AutoSave" };
constexpr auto AutoSaveInterval { "AutoSave Interval" };
constexpr auto FontFamily       { "FontFasmily" };
constexpr auto FontSize         { "FontSize" };
constexpr auto Footer           { "Footer" };
constexpr auto Header           { "Header" };
constexpr auto MainSplitter     { "MainSplitter" };
constexpr auto MarginBottom     { "MarginBottom" };
constexpr auto MarginLeft       { "MarginLeft" };
constexpr auto MarginRight      { "MarginRight" };
constexpr auto MarginTop        { "MarginTop" };
constexpr auto Orientation      { "Orientation" };
constexpr auto OtherSplitter    { "OtherSplitter" };
constexpr auto PageSize         { "PageSize" };
constexpr auto Position         { "Position" };
constexpr auto SceneTag         { "SceneTag" };
constexpr auto Theme            { "Theme" };
constexpr auto ToolbarVisible   { "ToolbarVisible" };
constexpr auto TypingSounds     { "TypingSounds" };
constexpr auto UiFontFamily     { "UiFontFasmily" };
constexpr auto UiFontSize       { "UiFontSize" };
constexpr auto Voice            { "Voice" };
constexpr auto WindowLoc        { "WindowLoc" };

const     auto DefaultActingScripts  { QJsonArray() };
constexpr auto DefaultChapterTag     { "chapter" };
constexpr auto DefaultCoverTag       { "cover" };
constexpr auto DefaultFont           { "Segoe UI" };
constexpr auto DefaultFontSize       { 9 };
constexpr auto DefaultFooter         { "\x1F\x1F" };
constexpr auto DefaultHeader         { "\x1F\x1F" };
constexpr auto DefaultInterval       { 5 * 60 };
constexpr auto DefaultMarginBottom   { 1.0 };
constexpr auto DefaultMarginLeft     { 1.0 };
constexpr auto DefaultMarginRight    { 1.0 };
constexpr auto DefaultMarginTop      { 1.0 };
constexpr auto DefaultOrientation    { QPageLayout::Portrait };
const     auto DefaultOther          { QJsonArray() };
constexpr auto DefaultPageSize       { "Letter" };
constexpr auto DefaultPosition       { 0 };
const     auto DefaultSplitter       { QJsonArray() };
constexpr auto DefaultSave           { false };
constexpr auto DefaultSceneTag       { "scene" };
constexpr auto DefaultSounds         { false };
constexpr auto DefaultToolbarVisible { true };
constexpr auto DefaultTheme          { 2 };
constexpr auto DefaultVoice          { 0 };

bool Preferences::load() {
    QFont def;
    QSettings settings(Company, Program);
    QJsonArray scripts = settings.value(ActingScripts, DefaultActingScripts).toJsonArray();
    mActingScripts.clear();
    for (const auto& script: scripts) mActingScripts.append(script.toString());
    mAutoSave =          settings.value(AutoSave,         DefaultSave).toBool();
    mAutoSaveInterval =  settings.value(AutoSaveInterval, DefaultInterval).toLongLong();
    mChapterTag =        settings.value(ChapterTag,       DefaultChapterTag).toString();
    mCoverTag =          settings.value(CoverTag ,        DefaultCoverTag).toString();
    mFontFamily =        settings.value(FontFamily,       DefaultFont).toString();
    mFontSize =          settings.value(FontSize,         DefaultFontSize).toInt();
    mFooter =            settings.value(Footer,           DefaultFooter).toString();
    mHeader =            settings.value(Header,           DefaultHeader).toString();
    QJsonArray arr =     settings.value(MainSplitter,     DefaultSplitter).toJsonArray();
    mMainSplitter.clear();
    for (auto i = 0; i < arr.size(); ++i) mMainSplitter.append(qlonglong(arr[i].toInt(-1)));
    mMargins.clear();
    mMargins.append(     settings.value(MarginBottom,     DefaultMarginBottom).toDouble());
    mMargins.append(     settings.value(MarginLeft,       DefaultMarginLeft).toDouble());
    mMargins.append(     settings.value(MarginRight,      DefaultMarginRight).toDouble());
    mMargins.append(     settings.value(MarginTop,        DefaultMarginTop).toDouble());
    QJsonArray other =   settings.value(OtherSplitter,    DefaultOther).toJsonArray();
    mOrientation =       QPageLayout::Orientation(settings.value(Orientation,      DefaultOrientation).toInt());
    mOtherSplitter.clear();
    for (auto i = 0; i < other.size(); ++i) mOtherSplitter.append(qlonglong(other[i].toInt(-1)));
    mPageSize =          settings.value(PageSize,         DefaultPageSize).toString();
    mPosition =          settings.value(Position,         DefaultPosition).toLongLong();
    mSceneTag =          settings.value(SceneTag,         DefaultSceneTag).toString();
    mTheme =             settings.value(Theme,            DefaultTheme).toInt();
    mToolbarVisible =    settings.value(ToolbarVisible,   DefaultToolbarVisible).toBool();
    mTypingSounds =      settings.value(TypingSounds,     DefaultSounds).toBool();
    mUiFontFamily =      settings.value(UiFontFamily,     def.defaultFamily()).toString();
    mUiFontSize =        settings.value(UiFontSize,       def.pointSize()).toInt();
    mVoice =             settings.value(Voice,            DefaultVoice).toInt();
    mWindow =            settings.value(WindowLoc,        { }).toRect();
    if (mWindow.height() < 1 || mWindow.width() < 1) mWindow.setRect(0, 0, -1, -1);
    return true;
}

QString Preferences::newPath(const QString& path) {
    StringList parts(path.split("/"));
    QString& last = parts.last();
    last = "Lt" + last;
    return parts.join("/");
}

QPageSize::PageSizeId Preferences::pageSizeToPid(const QString& size) {
    QPageSize::PageSizeId pid { QPageSize::Letter };
    for (int id = 0; id <= QPageSize::LastPageSize; ++id) {
        pid = static_cast<QPageSize::PageSizeId>(id);
        QString name = QPageSize(pid).name();
        if (QPageSize(pid).isValid() && ((name.startsWith("A") && name[1].isDigit()) ||
                                         name.startsWith("Letter") ||
                                         name.startsWith("Legal"))) {
            if ((size == "A" && name == "A4") ||
                (name.startsWith(size)) ||
                (name.contains(size))) {
                break;
            }
        }
    }
    return pid;
}

QString Preferences::pidToSize(const QPageSize::PageSizeId pid) {
    if (pid < 0 || pid >= QPageSize::LastPageSize) return QPageSize(QPageSize::Letter).name();
    else return QPageSize(pid).name();
}

bool Preferences::read(Json5Object& obj) {
    QFont def;
    Json5Array arr;
    if (obj.contains("v1")) {
        Json5Object prefs = Item::hasObj(obj, "options", {});
        mAutoSave =         Item::hasBool(prefs, "autoSave",         DefaultSave);
        mAutoSaveInterval = Item::hasNum(prefs,  "autoSaveInerval",  qlonglong(DefaultInterval));
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
        font =              Item::hasStr(prefs,  "uIextFont",        def.defaultFamily() + (":" + QString::number(def.pixelSize())));
        parts = { font.split(":") };
        if (parts.size() == 2) {
            mUiFontFamily = parts[0];
            bool ok = true;
            mUiFontSize   = parts[1].toInt(&ok);
            if (!ok || mUiFontSize < 1) mUiFontSize = DefaultFontSize;
        } else {
            mUiFontFamily = DefaultFont;
            mUiFontSize   = DefaultFontSize;
        }
        mVoice =             Item::hasNum(obj,  "voice",           qlonglong(DefaultVoice));
        mActingScripts.clear();
        mChapterTag = DefaultChapterTag;
        mFooter = DefaultFooter;
        mHeader = DefaultHeader;
        mOrientation = DefaultOrientation;
        mPageSize = DefaultPageSize;
        mPosition = DefaultPosition;
        mSceneTag = DefaultSceneTag;
        mTheme = DefaultTheme;
        mToolbarVisible = DefaultToolbarVisible;
        mUiFontFamily = def.defaultFamily();
        mUiFontSize = def.pointSize();
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
        auto scripts =      Item::hasArr(obj,  ActingScripts,    { });
        mActingScripts.clear();
        for (const auto& script: scripts) mActingScripts.append(script.toString());
        mAutoSave =         Item::hasBool(obj, AutoSave,         DefaultSave);
        mAutoSaveInterval = Item::hasNum(obj,  AutoSaveInterval, qlonglong(DefaultInterval));
        mChapterTag =       Item::hasStr(obj,  ChapterTag,       DefaultChapterTag);
        mCoverTag =         Item::hasStr(obj,  CoverTag,         DefaultCoverTag);
        mFontFamily =       Item::hasStr(obj,  FontFamily,       DefaultFont);
        mFontSize =         Item::hasNum(obj,  FontSize,         qlonglong(DefaultFontSize));
        mFooter =           Item::hasStr(obj,  Footer,           DefaultFooter);
        mHeader =           Item::hasStr(obj,  Header,           DefaultHeader);
        mPosition =         Item::hasNum(obj,  Position,         qlonglong(DefaultPosition));
        mPageSize =         Item::hasStr(obj,  PageSize,         DefaultPageSize);
        mSceneTag =         Item::hasStr(obj,  SceneTag,         DefaultSceneTag);
        mTheme =            Item::hasNum(obj,  Theme,            qlonglong(DefaultTheme));
        mToolbarVisible =   Item::hasBool(obj, ToolbarVisible,   DefaultToolbarVisible);
        mTypingSounds =     Item::hasBool(obj, TypingSounds,     DefaultSounds);
        mUiFontFamily =     Item::hasStr(obj,  UiFontFamily,     def.defaultFamily());
        mUiFontSize =       Item::hasNum(obj,  UiFontSize,       qlonglong(def.pointSize()));
        mVoice =            Item::hasBool(obj, Voice,            DefaultVoice);
        arr =               Item::hasArr(obj,  MainSplitter,     { });
        mMainSplitter.clear();
        for (auto& i: arr) {
            if (!i.isNumber()) continue;
            mMainSplitter.append(i.toInt());
        }
        arr = Item::hasArr(obj, OtherSplitter, { });
        mMargins.clear();
        mMargins.append(    Item::hasNum(obj, MarginBottom,            DefaultMarginBottom));
        mMargins.append(    Item::hasNum(obj, MarginLeft,              DefaultMarginLeft));
        mMargins.append(    Item::hasNum(obj, MarginRight,             DefaultMarginRight));
        mMargins.append(    Item::hasNum(obj, MarginTop,               DefaultMarginTop));
        mOrientation =      QPageLayout::Orientation(Item::hasNum(obj, Orientation, qlonglong(DefaultOrientation)));
        mOtherSplitter.clear();
        for (auto& i: arr) {
            if (!i.isNumber()) continue;
            mOtherSplitter.append(i.toInt());
        }
        arr =               Item::hasArr(obj,  WindowLoc,        { });
    }
    QRect geom;
    geom.setX(Item::hasNum(arr, 0, qlonglong(0)));
    geom.setY(Item::hasNum(arr, 1, qlonglong(0)));
    geom.setWidth(Item::hasNum(arr, 2, qlonglong(-1)));
    geom.setHeight(Item::hasNum(arr, 3, qlonglong(-1)));
    mWindow = geom;
    if (mWindow.height() < 1 || mWindow.width() < 1) mWindow.setRect(0, 0, -1, -1);
    return true;
}

bool Preferences::save() {
    QSettings settings(Company, Program);
    QJsonArray act;
    for (const auto& a: mActingScripts) act.append(a);
    settings.setValue(ActingScripts,    act);
    settings.setValue(AutoSave,         mAutoSave);
    settings.setValue(AutoSaveInterval, mAutoSaveInterval);
    settings.setValue(ChapterTag,       mChapterTag);
    settings.setValue(CoverTag,         mCoverTag);
    settings.setValue(FontFamily,       mFontFamily);
    settings.setValue(FontSize,         mFontSize);
    settings.setValue(Footer,           mFooter);
    settings.setValue(Header,           mHeader);
    QJsonArray arr;
    for (auto& a: mMainSplitter) arr.append(a);
    settings.setValue(MainSplitter,     arr);
    settings.setValue(MarginBottom,     mMargins[Bottom]);
    settings.setValue(MarginLeft,       mMargins[Left]);
    settings.setValue(MarginRight,      mMargins[Right]);
    settings.setValue(MarginTop,        mMargins[Top]);
    settings.setValue(Orientation,      mOrientation);
    QJsonArray otr;
    for (auto& a: mOtherSplitter) otr.append(a);
    settings.setValue(OtherSplitter,    otr);
    settings.setValue(PageSize,         mPageSize);
    settings.setValue(Position,         mPosition);
    settings.setValue(SceneTag,         mSceneTag);
    settings.setValue(Theme,            mTheme);
    settings.setValue(ToolbarVisible,   mToolbarVisible);
    settings.setValue(TypingSounds,     mTypingSounds);
    settings.setValue(UiFontFamily,     mUiFontFamily);
    settings.setValue(UiFontSize,       mUiFontSize);
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
        Main::ref().ui()->menubar->setNativeMenuBar(false);
        Main::ref().ui()->treeWidget->setStyleSheet("QTreeView {\n"
                                                    "    color: white;\n"
                                                    "    background-color: #2A2A2A;\n"
                                                    "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
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
                                                  "    font: "  + QString::number(mFontSize) + "pt \"" + mFontFamily + "\";\n" +
                                                  "    color: white;\n"
                                                  "    background-color: black;\n"
                                                  "}");
        Main::ref().ui()->menubar->setStyleSheet("QMenuBar {\n"
                                                 "    color: white;\n"
                                                 "    background-color: #2A2A2A"
                                                 "}"
                                                 "QMenuBar::item {\n"
                                                 "    color: white;\n"
                                                 "    background: transparent;\n"
                                                 "}");
        QFont newFont(mUiFontFamily, mUiFontSize);
        Main::ref().ui()->menubar->setFont(newFont);
        QApplication::setFont(newFont);
        QString menuStyle {
            "QMenu {\n"
            "    background-color: #2A2A2A;\n"
            "    color: white;\n"
            "    border: 1px solid #444;\n"
            "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
            "}\n"
            "\n"
            "QMenu::item {\n"
            "    padding: 6px 20px;\n"
            "    background: transparent;\n"
            "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
            "}\n"
            "\n"
            "QMenu::item:selected {\n"
            "    background: #444;\n"
            "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
            "}\n"
            "QMenu::item:disabled {\n"
            "    color: #888;\n"
            "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
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
                                                               "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
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
        Main::ref().ui()->menubar->setNativeMenuBar(false);
        Main::ref().ui()->treeWidget->setStyleSheet("QTreeView::item {\n"
                                                    "    color: black;\n"
                                                    "    background-color: #B4B4B4;\n"
                                                    "    selection-color: black;\n"
                                                    "    selection-background-color: white;\n"
                                                    "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
                                                    "}"
                                                    "QTreeView::branch:has-children:!has-siblings:closed,\n"
                                                    "QTreeView::branch:closed:has-children:has-siblings {\n"
                                                    "    border-image: none;\n"
                                                    "    image: url(:/imgs/Closed.png);\n"
                                                    "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
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
                                                  "    font: "  + QString::number(mFontSize) + "pt \"" + mFontFamily + "\";\n" +
                                                  "}");
        Main::ref().ui()->menubar->setStyleSheet("QMenuBar {\n"
                                                 "    background-color: #BEBEBE;\n"
                                                 "    color: black;\n"
                                                 "}"
                                                 "QMenuBar::item {\n"
                                                 "    background-color: #BEBEBE;\n"
                                                 "    color: black;\n"
                                                 "}");
        QFont newFont(mUiFontFamily, mUiFontSize);
        Main::ref().ui()->menubar->setFont(newFont);
        QApplication::setFont(newFont);
        QString menuStyle {
            "QMenu {\n"
            "    background-color: #BEBEBE;\n"
            "    color: black;\n"
            "    border: 1px solid #999;\n"
            "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
            "}\n"
            "\n"
            "QMenu::item {\n"
            "    padding: 6px 20px;\n"
            "    background: transparent;\n"
            "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
            "}\n"
            "\n"
            "QMenu::item:selected {\n"
            "    background: #999;\n"
            "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
            "}\n"
            "QMenu::item:disabled {\n"
            "    color: #888;\n"
            "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
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
                                                               "    font: "  + QString::number(mUiFontSize) + "pt \"" + mUiFontFamily + "\";\n" +
                                                               "}");
    }
    mIsDark = false;
    resetIcons(false);
}

Preferences::Preferences()
    : mWasDark(isDark()) {
    load();
}

void Preferences::applyFontToTree(QWidget* w, const QFont& f) {
    if (!w) return;

    w->setFont(f);

    const QObjectList& kids = w->children();
    for (QObject* obj: kids) {
        QWidget* child = qobject_cast<QWidget*>(obj);
        if (child) applyFontToTree(child, f);
    }
}

QString Preferences::checkPath(const QString& path, bool checked)
{
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
        QIcon icon;
        if (!isDark) icon = Main::ref().getIcon("Lt" + button->objectName());
        else icon = Main::ref().getIcon(button->objectName());
        button->setIcon(icon);
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
    Json5Array ac;
    for (const auto& a: mActingScripts) ac.append(a);
    obj[ActingScripts] =    ac;
    obj[AutoSave] =         mAutoSave;
    obj[AutoSaveInterval] = mAutoSaveInterval;
    obj[ChapterTag] =       mChapterTag;
    obj[CoverTag] =         mCoverTag;
    obj[FontFamily] =       mFontFamily;
    obj[FontSize] =         mFontSize;
    obj[Footer] =           mFooter;
    obj[Header] =           mHeader;
    Json5Array ms;
    for (const auto v: mMainSplitter) ms.append(qlonglong(v));
    obj[MainSplitter] =     ms;
    obj[MarginBottom] =     mMargins[Bottom];
    obj[MarginLeft] =       mMargins[Left];
    obj[MarginRight] =      mMargins[Right];
    obj[MarginTop] =        mMargins[Top];
    Json5Array os;
    for (const auto v: mOtherSplitter) os.append(qlonglong(v));
    obj[Orientation] =      qlonglong(mOrientation);
    obj[OtherSplitter] =    os;
    obj[PageSize] =         mPageSize;
    obj[Position] =         mPosition;
    obj[SceneTag] =         mSceneTag;
    obj[Theme] =            mTheme;
    obj[ToolbarVisible] =   mToolbarVisible;
    obj[TypingSounds] =     mTypingSounds;
    obj[UiFontFamily] =     mUiFontFamily;
    obj[UiFontSize] =       mUiFontSize;
    obj[Voice] =            mVoice;
    Json5Array arr;
    arr.append(qlonglong(mWindow.x()));
    arr.append(qlonglong(mWindow.y()));
    arr.append(qlonglong(mWindow.width()));
    arr.append(qlonglong(mWindow.height()));
    obj[WindowLoc] =        arr;

    return obj;
}

static constexpr auto PointsPerInch = 72.0;

List<qreal> Preferences::margins(Units unit) const {
    if (unit == Units::Inches) return mMargins;
    auto margins = mMargins;
    margins[Left]   *= PointsPerInch;
    margins[Right]  *= PointsPerInch;
    margins[Top]    *= PointsPerInch;
    margins[Bottom] *= PointsPerInch;
    return margins;
}
