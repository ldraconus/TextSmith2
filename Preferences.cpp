#include "Preferences.h"
#include "Novel.h"

#include <QSettings>

#include <Json5.h>
#include <StringList.h>

bool Preferences::load() {
    QSettings settings("SoftwareOnHand", "TextSmith");
    mAutoSave = settings.value("AutoSave", "false").toBool();
    mAutoSaveInterval = settings.value("AutoSave Interval", "300").toLongLong();
    mFontFamily = settings.value("FontFamily", "Sehoe UI").toString();
    mFontSize = settings.value("FontSize", "9").toInt();
    mTheme = settings.value("Theme", 3).toInt();
    mTypingSounds = settings.value("TypingSounds", "false").toBool();
    mVoice = settings.value("Voice", "0").toInt();
    mWindow = settings.value("Window").toRect();
    return true;
}

bool Preferences::read(Json5Object& obj) {
    mAutoSave = Item::hasBool(obj, "AutoSave", false);
    mAutoSaveInterval = Item::hasNum(obj, "AutoSaveInterval", 300);
    mFontFamily = Item::hasStr(obj, "FontFamily", "Segoe UI");
    mFontSize = Item::hasNum(obj, "FontSize", 9);
    mTypingSounds = Item::hasBool(obj, "TypingSounds", false);
    mVoice = Item::hasBool(obj, "Voice", false);
    Json5Array arr = Item::hasArr(obj, "Window", {});
    QRect geom;
    geom.setX(Item::hasNum(arr, 0, 0));
    geom.setY(Item::hasNum(arr, 1, 0));
    geom.setWidth(Item::hasNum(arr, 2, -1));
    geom.setHeight(Item::hasNum(arr, 3, -1));
    mWindow = geom;
   return true;
}

bool Preferences::save() {
    QSettings settings("SoftwareOnHand", "TextSmith");
    settings.setValue("AutoSave", mAutoSave);
    settings.setValue("AutoSaveInterval", mAutoSaveInterval);
    settings.setValue("FontFamily", mFontFamily);
    settings.setValue("FontSize", mFontSize);
    settings.setValue("TypingSounds", mTypingSounds);
    settings.setValue("Voice", mVoice);
    settings.setValue("Window", mWindow);
    return true;
}

Json5Object Preferences::write() {
    Json5Object obj;
    obj["AutoSave"] = mAutoSave;
    obj["AutoSaveInterval"] = mAutoSaveInterval;
    obj["FontFamily"] = mFontFamily;
    obj["FontSize"] = mFontSize;
    obj["TypingSounds"] = mTypingSounds;
    obj["Voice"] = mVoice;
    Json5Array arr;
    arr[0] = qlonglong(mWindow.x());
    arr[1] = qlonglong(mWindow.y());
    arr[2] = qlonglong(mWindow.width());
    arr[3] = qlonglong(mWindow.height());
    obj["Window"] = arr;

    return obj;
}
