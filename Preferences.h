#pragma once

#include <QApplication>
#include <QJsonArray>
#include <QObject>
#include <QPalette>
#include <QPushButton>
#include <QRect>

#include <Json5.h>
#include <Map.h>

class Preferences {    
public:
    Preferences()
        : mWasDark(isDark())               { load(); }
    explicit Preferences(Json5Object& obj) { read(obj); }
    ~Preferences()                         { save(); }

    QString     checkPath(const QString& path, bool checked);
    bool        isDark();
    bool        load();
    QString     newPath(const QString& path);
    bool        read(Json5Object& obj);
    void        resetIcons(bool isDark);
    bool        save();
    void        setDarkTheme();
    void        setLightTheme();
    void        setSystemTheme();
    Json5Object write();

    bool       autoSave()          { return mAutoSave; }
    qlonglong  autoSaveIntyerval() { return mAutoSaveInterval; }
    QString    chapterTag()        { return mChapterTag.isEmpty() ? "Chapter" : mChapterTag; }
    QString    fontFamily()        { return mFontFamily; }
    qlonglong  fontSize()          { return mFontSize; }
    bool       isDarkTheme()       { return mIsDark; }
    Json5Array mainSplitter()      { return mMainSplitter; }
    Json5Array otherSplitter()     { return mOtherSplitter; }
    qlonglong  position()          { return mPosition; }
    QString    sceneTag()          { return mSceneTag.isEmpty() ? "Scene" : mSceneTag; }
    qlonglong  theme()             { return mTheme; }
    bool       typingSounds()      { return mTypingSounds; }
    qlonglong  voice()             { return mVoice; }
    bool       wasDark()           { return mWasDark; }
    QRect      windowLocation()    { return mWindow; }

    void setApplicaiton(QApplication* a)  { mApp = a; }
    void setAutoSave(bool a)              { mAutoSave = a; }
    void setAutoSaveInterval(qlonglong i) { mAutoSaveInterval = i; }
    void setChapterTag(const QString& c)  { mChapterTag = c; }
    void setFontFamily(const QString& f)  { mFontFamily = f; }
    void setFontSize(qlonglong s)         { mFontSize = s; }
    void setMainSplitter(Json5Array& s)   { mMainSplitter = s; }
    void setOtherSplitter(Json5Array& s)  { mOtherSplitter = s; }
    void setPosition(qlonglong pos)       { mPosition = pos; }
    void setSceneTag(const QString& s)    { mSceneTag = s; }
    void setTheme(qlonglong t)            { mTheme = t; }
    void setTypingSounds(bool t)          { mTypingSounds = t; }
    void setVoice(qlonglong v)            { mVoice = v; }
    void setWindowLocation(QRect r)       { mWindow = r; }

private:
    QApplication* mApp              { nullptr };
    bool          mAutoSave         { false };
    qlonglong     mAutoSaveInterval { 5 * 60 };
    QString       mChapterTag       { "chapter" };
    QString       mFontFamily       { "Segoe UI" };
    qlonglong     mFontSize         { 9 };
    bool          mIsDark           { false };
    Json5Array    mMainSplitter     { };
    Json5Array    mOtherSplitter    { };
    qlonglong     mPosition         { 0 };
    QString       mSceneTag         { "scene" };
    qlonglong     mTheme            { 2 };
    bool          mTypingSounds     { false };
    qlonglong     mVoice            { 0 };
    bool          mWasDark          { false };
    QRect         mWindow;
};
