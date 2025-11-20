#pragma once

#include <QObject>
#include <QRect>

#include <Json5.h>
#include <Map.h>

class Preferences {
public:
    Preferences()                          { load(); }
    explicit Preferences(Json5Object& obj) { read(obj); }

    bool        load();
    bool        read(Json5Object& obj);
    bool        save();
    Json5Object write();

    QString   fontFamily()     { return mFontFamily; }
    qlonglong fontSize()       { return mFontSize; }
    bool      typingSounds()   { return mTypingSounds; }
    qlonglong voice()          { return mVoice; }
    QRect     windowLocation() { return mWindow; }

    void setFontFamily(const QString& f) { mFontFamily = f; }
    void setFontSize(qlonglong s)        { mFontSize = s; }
    void setTheme(qlonglong t)           { mTheme = t; }
    void setTypingSounds(bool t)         { mTypingSounds = t; }
    void setVoice(qlonglong v)           { mVoice = v; }
    void setWindowLocation(QRect r)      { mWindow = r; }

private:
    bool      mAutoSave         { false };
    qlonglong mAutoSaveInterval { 5 * 60 };
    QString   mFontFamily       { "Segoe UI" };
    qlonglong mFontSize         { 9 };
    qlonglong mTheme            { 2 };
    bool      mTypingSounds     { false };
    qlonglong mVoice            { 0 };
    QRect     mWindow;
};
