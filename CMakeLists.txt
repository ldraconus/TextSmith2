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


if(MSVC)
    message(STATUS "Found Hunspell vi vcpkg")
    # Include directories
    include_directories(
        E:/QtSource/include
        E:/QtSource/libzip/build
        E:/QtSource/libzip/lib
        E:/QtSource/vcpkg/packages/hunspell_x64-windows/include
        ${CMAKE_SOURCE_DIR}
    )
else()
    include_directories(
        E:/QtSource/include
        E:/QtSource/libzip/build
        E:/QtSource/libzip/lib
        E:/QtSource/Spelling/hunspell
        E:/QtSource/Spelling/parsers
        E:/QtSource/Spelling/tools
        ${CMAKE_SOURCE_DIR}
    )
endif()

# Include directories

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
    SpeechDialog.ui

)

# Tell the linker where to find libzip.dll.a
#link_directories("E:/QtSource/libzip/build/lib")
if(MSVC)
# 1. Define where Vcpkg lives (Hardcoded based on your setup)
    set(VCPKG_INSTALLED_DIR "E:/QtSource/vcpkg/installed/x64-windows")

    message(STATUS "Checking for Hunspell in: ${VCPKG_INSTALLED_DIR}")

    # 2. Find the Header (looks for hunspell/hunspell.hxx)
    find_path(HUNSPELL_INCLUDE_DIR
        NAMES hunspell.hxx
        PATHS "${VCPKG_INSTALLED_DIR}/include/hunspell"
        NO_DEFAULT_PATH
    )

    # 3. Find the Library (looks for hunspell.lib or libhunspell.lib)
    find_library(HUNSPELL_LIBRARY
        NAMES hunspell-1.7.lib
        PATHS "${VCPKG_INSTALLED_DIR}/lib"
        NO_DEFAULT_PATH
    )

    if(HUNSPELL_INCLUDE_DIR AND HUNSPELL_LIBRARY)
        message(STATUS "  - Found Header: ${HUNSPELL_INCLUDE_DIR}")
        message(STATUS "  - Found Lib:    ${HUNSPELL_LIBRARY}")

        # 4. Create the target manually so it works like the auto-detected version
        add_library(hunspell::hunspell UNKNOWN IMPORTED)
        set_target_properties(hunspell::hunspell PROPERTIES
            IMPORTED_LOCATION "${VCPKG_INSTALLED_DIR}/lib/hunspell-1.7.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${HUNSPELL_INCLUDE_DIR}"
        )
        set(HUNSPELL_TARGET hunspell::hunspell)
        target_link_libraries(TextSmith2 PRIVATE "${VCPKG_INSTALLED_DIR}/lib/hunspell-1.7.lib")
    else()
        message(FATAL_ERROR "Could NOT find Hunspell in ${VCPKG_INSTALLED_DIR}. Check if the folder exists.")
    endif()
else()
    add_library(hunspell::hunspell UNKNOWN IMPORTED)
    set_target_properties(hunspell::hunspell PROPERTIES
        IMPORTED_LOCATION "E:/QtSource/Spelling/build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug/libSpelling.a"
        INTERFACE_INCLUDE_DIRECTORIES "E:/QtSource/Spelling/hunspell" "E:/QtSource/Spelling/parsers" "E:/QtSource/Spelling/tools"
    )
    set(HUNSPELL_TARGET hunspell::hunspell)
endif()

# Link Qt + libzip
target_link_libraries(TextSmith2 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::TextToSpeech
    E:/QtSource/libzip/build/lib/libzip.dll.a
    E:/QtSource/zlib-1.3.1/build/libzlib.dll.a
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
