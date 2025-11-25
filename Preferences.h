#pragma once

#include <QApplication>
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

    bool      autoSave()          { return mAutoSave; }
    qlonglong autoSaveIntyerval() { return mAutoSaveInterval; }
    QString   fontFamily()        { return mFontFamily; }
    qlonglong fontSize()          { return mFontSize; }
    qlonglong theme()             { return mTheme; }
    bool      typingSounds()      { return mTypingSounds; }
    qlonglong voice()             { return mVoice; }
    QRect     windowLocation()    { return mWindow; }

    void setApplicaiton(QApplication* a)  { mApp = a; }
    void setAutoSave(bool a)              { mAutoSave = a; }
    void setAutoSaveInterval(qlonglong i) { mAutoSaveInterval = i; }
    void setFontFamily(const QString& f)  { mFontFamily = f; }
    void setFontSize(qlonglong s)         { mFontSize = s; }
    void setTheme(qlonglong t)            { mTheme = t; }
    void setTypingSounds(bool t)          { mTypingSounds = t; }
    void setVoice(qlonglong v)            { mVoice = v; }
    void setWindowLocation(QRect r)       { mWindow = r; }

private:
    QApplication* mApp              { nullptr };
    bool          mAutoSave         { false };
    qlonglong     mAutoSaveInterval { 5 * 60 };
    QString       mFontFamily       { "Segoe UI" };
    qlonglong     mFontSize         { 9 };
    qlonglong     mTheme            { 2 };
    bool          mTypingSounds     { false };
    qlonglong     mVoice            { 0 };
    bool          mWasDark          { false };
    QRect         mWindow;
};
