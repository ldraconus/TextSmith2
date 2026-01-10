QT       += core gui texttospeech

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += \
    c++23

INCLUDEPATH += \
    E:\QtSource\include \
    E:\QtSource\libzip\lib \
    E:\QtSource\libzip\build

LIBS += -LE:\QtSource\libzip\build\lib -lzip -lz

RC_FILE = appicon.rc

appicon.rc.depends = TextSmith2.ico

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    EBookExporter.cpp \
    ExportDialog.cpp \
    FullScreenDialog.cpp \
    HtmlExporter.cpp \
    ItemDescriptionDialog.cpp \
    Main.cpp \
    MarkdownExporter.cpp \
    Novel.cpp \
    PdfExporter.cpp \
    Preferences.cpp \
    PreferencesDialog.cpp \
    SearchReplace.cpp \
    Speech.cpp \
    Start.cpp \
    TextEdit.cpp \
    entity.c \
    html2md-main/src/html2md.cpp \
    html2md-main/src/table.cpp

HEADERS += \
    EBookExporter.h \
    ExportDialog.h \
    Exporter.h \
    FullScreenDialog.h \
    HtmlExporter.h \
    ItemDescriptionDialog.h \
    Main.h \
    MarkdownExporter.h \
    Novel.h \
    PdfExporter.h \
    Preferences.h \
    PreferencesDialog.h \
    Printer.h \
    SearchReplace.h \
    Speech.h \
    TextEdit.h \
    TreeWidget.h \
    Words.h \
    entity.h \
    html2md-main/include/html2md.h \
    html2md-main/include/table.h

FORMS += \
    ExportDialog.ui \
    FullScreenDialog.ui \
    ItemDescriptionDialog.ui \
    Main.ui \
    PreferencesDialog.ui \
    dialog.ui

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
    .clangd \
    Background.png \
    CMakeLists.txt \
    Center.ico \
    Center.png \
    Closed.png \
    DarkClosed.png \
    DarkOpen.png \
    Installer.ico \
    Open.png \
    TODO.txt \
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
