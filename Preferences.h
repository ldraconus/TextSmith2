#pragma once

#include <QObject>

class Preferences {
public:
    Preferences()                                 { }
    explicit Preferences(const QString& filename) { read(filename); }

    bool read(const QString& filename);

    QString   fontFamily()   { return mFontFamily; }
    qlonglong fontSize()     { return mFontSize; }
    bool      typingSounds() { return mTypingSounds; }

private:
    QString   mFontFamily   { "Segoe UI" };
    qlonglong mFontSize     { 9 };
    bool      mTypingSounds { false };
};
