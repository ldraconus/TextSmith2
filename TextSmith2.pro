QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += \
    c++23

INCLUDEPATH += \
    E:\QtSource\include

RC_FILE = appicon.rc

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Main.cpp \
    Novel.cpp \
    Preferences.cpp \
    Start.cpp

HEADERS += \
    Main.h \
    Novel.h \
    Preferences.h

FORMS += \
    Main.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Center.ico \
    Center.png \
    TextSmith2.ico \
    TextSmithIcon.png \
    appicon.rc \
    bold.ico \
    bold.png \
    decreaseIndent.ico \
    decreaseIndent.png \
    deleteItem.ico \
    deleteItem.png \
    fullJustify.ico \
    fullJustify.png \
    increseIndent.ico \
    increseIndent.png \
    italic.ico \
    italic.png \
    leftjustify.ico \
    leftjustify.png \
    moveItemDown.ico \
    moveItemDown.png \
    moveItemOut.ico \
    moveItemOut.png \
    moveItemUp.ico \
    moveItemUp.png \
    newItem.ico \
    newItem.png \
    rightJustify.ico \
    rightJustify.png \
    underline.ico \
    underline.png

RESOURCES += \
    gfx.qrc
