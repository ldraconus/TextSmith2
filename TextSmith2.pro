QT += widgets core gui texttospeech printsupport

CONFIG += c++23

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix {
    INCLUDEPATH += \
        /home/chris/src/include \
        /home/chris/src/TextSmith2 \
        /home/chris/src/libzip/lib \
        /home/chris/src/libzip/build
}
win32 {
    INCLUDEPATH += \
        "E:/QtSource/include" \
        "E:/QtSource/libzip/build" \
        "E:/QtSource/libzip/lib" \
        "E:/QtSource/vcpkg/packages/hunspell_x64-windows/include"
}

LIBS += -L$path) -l($lib)
unix {
    LIBS += \
        -L/usr/local/lib/ -lz \
        -L/usr/local/lib/ -lzip \
        -L/usr/local/lib/ -lhunspell-1.7 \
        -L/usr/local/lib/ -lparsers
}
win32 {
    LIBS += \
        -LE:/QtSource/libzip/build/lib/ -lzip \
        -LE:/QtSource/zlib-1.3.1/build/ -lzlib \
        -LE:/QtSource/vcpkg/installed/x64-windows/lib/ -lhunspell-1.7
}

SOURCES += \
    5th.cpp \
    AboutDialog.cpp \
    EBookExporter.cpp \
    EasySpelling.cpp \
    EpubBuilder.cpp \
    ExportDialog.cpp \
    FullScreenDialog.cpp \
    HelpDialog.cpp \
    HtmlExporter.cpp \
    ItemDescriptionDialog.cpp \
    Main.cpp \
    MarkdownExporter.cpp \
    Novel.cpp \
    PageSetupDialog.cpp \
    PdfExporter.cpp \
    Preferences.cpp \
    PreferencesDialog.cpp \
    PrintDialog.cpp \
    Printer.cpp \
    RtfExporter.cpp \
    ScriptDialog.cpp \
    SearchReplace.cpp \
    SoundPool.cpp \
    Speech.cpp \
    Start.cpp \
    TextEdit.cpp \
    TextExporter.cpp \
    dict.cpp \
    mainwindow.cpp

HEADERS += \
    5th.h \
    AboutDialog.h \
    EBookExporter.h \
    EasySpelling.h \
    EpubBuilder.h \
    ExportDialog.h \
    Exporter.h \
    FullScreenDialog.h \
    HelpDialog.h \
    HtmlExporter.h \
    ItemDescriptionDialog.h \
    Main.h \
    MarkdownExporter.h \
    Novel.h \
    PageSetupDialog.h \
    PdfExporter.h \
    Preferences.h \
    PreferencesDialog.h \
    PrintDialog.h \
    Printer.h \
    RtfExporter.h \
    ScriptDialog.h \
    SearchReplace.h \
    SoundPool.h \
    Speech.h \
    TextEdit.h \
    TextExporter.h \
    TreeWidget.h \
    Words.h \
    mainwindow.h

FORMS += \
    AboutDialog.ui \
    ExportDialog.ui \
    FullScreenDialog.ui \
    HelpDialog.ui \
    ItemDescriptionDialog.ui \
    Main.ui \
    PageSetupDialog.ui \
    PreferencesDialog.ui \
    PrintDialog.ui \
    ScriptDialog.ui \
    SpeechDialog.ui \
    SpellCheckDialog.ui \
    dialog.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -L/path/to/library -lmylibrary

# Add header path
INCLUDEPATH += /path/to/library/include

DISTFILES += \
    .gitignore \
    Background.png \
    CMakeLists.txt \
    Center.ico \
    Center.png \
    CenterCk.ico \
    Closed.png \
    DarkClosed.png \
    DarkOpen.png \
    Documentation.html \
    Documentation.novel \
    Documentation.pdf \
    Gitub_Build.yaml.txt \
    Installer.ico \
    InstallerTag.png \
    KeyWhack.mp3 \
    KeyWhack.wav \
    LICENSE \
    LtCenter.ico \
    LtCenterCk.ico \
    Ltbold.ico \
    LtboldCk.ico \
    LtdecreaseIndent.ico \
    LtdeleteItem.ico \
    LtfullJustify.ico \
    LtfullJustifyCk.ico \
    LtincreseIndent.ico \
    Ltitalic.ico \
    LtitalicCk.ico \
    Ltleftjustify.ico \
    LtleftjustifyCk.ico \
    LtmoveItemDown.ico \
    LtmoveItemOut.ico \
    LtmoveItemUp.ico \
    LtnewItem.ico \
    LtrightJustify.ico \
    LtrightJustifyCk.ico \
    Ltunderline.ico \
    LtunderlineCk.ico \
    MarginDing.mp3 \
    Open.png \
    ReturnClunk.mp3 \
    ReturnRoll.mp3 \
    SpaceThunk.mp3 \
    TODO.txt \
    TextSmith2.ico \
    TextSmithIcon.png \
    appicon.rc \
    appicon.s \
    bold.ico \
    bold.png \
    boldCk.ico \
    decreaseIndent.ico \
    decreaseIndent.png \
    deleteItem.ico \
    deleteItem.png \
    fullJustify.ico \
    fullJustify.png \
    fullJustifyCk.ico \
    increseIndent.ico \
    increseIndent.png \
    index.aff \
    index.dic \
    installer.sh \
    italic.ico \
    italic.png \
    italicCk.ico \
    leftjustify.ico \
    leftjustify.png \
    leftjustifyCk.ico \
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
    rightJustifyCk.ico \
    triangle-right.svg \
    underline.ico \
    underline.png \
    underlineCk.ico

RESOURCES += \
    gfx.qrc
