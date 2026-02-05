cmake_minimum_required(VERSION 3.21)
project(TextSmith2 LANGUAGES CXX)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

# C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt setup
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    PrintSupport
    TextToSpeech
)

# Include directories
include_directories(
    E:/QtSource/include
    E:/QtSource/libzip/build
    E:/QtSource/libzip/lib
    E:/QtSource/Spelling/hunspell
    E:/QtSource/Spelling/parsers
    E:/QtSource/Spelling/tools
    ${CMAKE_SOURCE_DIR}
)

# Sources
set(SOURCES
    EBookExporter.cpp
    ExportDialog.cpp
    FullScreenDialog.cpp
    HtmlExporter.cpp
    ItemDescriptionDialog.cpp
    Main.cpp
    MarkdownExporter.cpp
    Novel.cpp
    PdfExporter.cpp
    Preferences.cpp
    PreferencesDialog.cpp
    SearchReplace.cpp
    SoundPool.cpp
    Speech.cpp
    Start.cpp
    TextEdit.cpp

)

# Headers
set(HEADERS
    EBookExporter.h
    ExportDialog.h
    Exporter.h
    FullScreenDialog.h
    HtmlExporter.h
    ItemDescriptionDialog.h
    Main.h
    MarkdownExporter.h
    Novel.h
    PdfExporter.h
    Preferences.h
    PreferencesDialog.h
    Printer.h
    SearchReplace.h
    Speech.h
    SoundPool.h
    TextEdit.h
    TreeWidget.h
    Words.h

)

# UI files
set(FORMS
    ExportDialog.ui
    FullScreenDialog.ui
    ItemDescriptionDialog.ui
    Main.ui
    PreferencesDialog.ui
    dialog.ui
)

# Resource file
set(RESOURCES gfx.qrc)

# Add executable
qt_add_executable(TextSmith2
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    ${RESOURCES}
    appicon.rc
    TODO.txt
    RtfExporter.h RtfExporter.cpp
    EasySpelling.h EasySpelling.cpp
    SpellCheckDialog.ui
    dict.cpp 5th.cpp 5th.h
    PageSetupDialog.h PageSetupDialog.cpp PageSetupDialog.ui
    Printer.cpp

)

# Tell the linker where to find libzip.dll.a
#link_directories("E:/QtSource/libzip/build/lib")

# Link Qt + libzip
target_link_libraries(TextSmith2 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::TextToSpeech
    E:/QtSource/libzip/build/lib/libzip.dll.a
    E:/QtSource/zlib-1.3.1/build/libzlib.dll.a
    E:/QtSource/Spelling/build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug/libSpelling.a
)

# Disable deprecated Qt APIs before 6.0
target_compile_definitions(TextSmith2 PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
    HUNSPELL_STATIC
)

# Installer target (optional)
add_custom_target(Installer
    COMMAND ${CMAKE_SOURCE_DIR}/../../installer.sh
    DEPENDS TextSmith2
)
