QT       += core gui texttospeech

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += \
    c++23

INCLUDEPATH += \
    E:\QtSource\include

RC_FILE = appicon.rc

appicon.rc.depends = TextSmith2.ico

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    FullScreenDialog.cpp \
    ItemDescriptionDialog.cpp \
    Main.cpp \
    Novel.cpp \
    Preferences.cpp \
    PreferencesDialog.cpp \
    Start.cpp \
    TextEdit.cpp

HEADERS += \
    FullScreenDialog.h \
    ItemDescriptionDialog.h \
    Main.h \
    Novel.h \
    Preferences.h \
    PreferencesDialog.h \
    TextEdit.h

FORMS += \
    FullScreenDialog.ui \
    ItemDescriptionDialog.ui \
    Main.ui \
    PreferencesDialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

Installer.target = Installer
Installer.depends = \
    release
Installer.commands = \
    ../../installer.sh

QMAKE_EXTRA_TARGETS += Installer

DISTFILES += \
    Background.png \
    Center.ico \
    Center.png \
    Installer.ico \
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
    installer.sh \
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
