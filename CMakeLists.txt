cmake_minimum_required(VERSION 3.21)
project(TextSmith2 LANGUAGES CXX)

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
    E:/QtSource/libzip/lib
    E:/QtSource/libzip/build
    ${CMAKE_SOURCE_DIR}
)

# libzip
link_directories(E:/QtSource/libzip/build/lib)

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
    Speech.cpp
    Start.cpp
    TextEdit.cpp
    entity.c
    html2md-main/src/html2md.cpp
    html2md-main/src/table.cpp
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
    TextEdit.h
    TreeWidget.h
    Words.h
    entity.h
    html2md-main/include/html2md.h
    html2md-main/include/table.h
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
)

# Link Qt + libzip
target_link_libraries(TextSmith2 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::TextToSpeech
    zip
    z
)

# Disable deprecated Qt APIs before 6.0
target_compile_definitions(TextSmith2 PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

# Installer target (optional)
add_custom_target(Installer
    COMMAND ${CMAKE_SOURCE_DIR}/../../installer.sh
    DEPENDS TextSmith2
)
