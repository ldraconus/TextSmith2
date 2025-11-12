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
    Start.cpp

HEADERS += \
    Main.h

FORMS += \
    Main.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Center.png \
    TextSmith2.ico \
    TextSmithIcon.png \
    appicon.rc \
    bold.png \
    decreaseIndent.png \
    deleteItem.png \
    fullJustify.png \
    increseIndent.png \
    italic.png \
    leftjustify.png \
    moveItemDown.png \
    moveItemOut.png \
    moveItemUp.png \
    rightJustify.png \
    underline.png

RESOURCES += \
    gfx.qrc
