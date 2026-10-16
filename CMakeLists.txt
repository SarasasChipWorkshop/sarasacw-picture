# SPDX-FileCopyrightText: 2022 - 2025 Gary Wang <git@blumia.net>
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(sarasacw-picture VERSION 1.6.0) # don't forget to update NEWS file and AppStream metadata.

include(GNUInstallDirs)
include(FeatureSummary)

option (EXIV2_METADATA_SUPPORT "Better image metadata support via libexiv2" ON)
option (TRANSLATION_RESOURCE_EMBEDDING "Embedding .qm translation files inside resource" OFF)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)

set (QT_MINIMUM_VERSION "6.5")

find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} REQUIRED
    COMPONENTS Widgets Svg SvgWidgets LinguistTools
    OPTIONAL_COMPONENTS DBus
)

if (EXIV2_METADATA_SUPPORT)
    find_package(exiv2)
    set_package_properties(exiv2 PROPERTIES
        URL "https://www.exiv2.org"
        DESCRIPTION "image metadata support"
        TYPE RECOMMENDED
        PURPOSE "Bring better image metadata support"
    )
endif ()

set (SCWPIC_CPP_FILES
    app/main.cpp
    app/mainwindow.cpp
    app/actionmanager.cpp
    app/graphicsview.cpp
    app/graphicsscene.cpp
    app/bottombuttongroup.cpp
    app/navigatorview.cpp
    app/settings.cpp
    app/settingsdialog.cpp
    app/aboutdialog.cpp
    app/metadatamodel.cpp
    app/metadatadialog.cpp
    app/exiv2wrapper.cpp
    app/playlistmanager.cpp
    app/fileopeneventhandler.cpp
    app/playlistscout.cpp
    app/playlistscouts/qtscout.cpp
    app/playlistscouts/explorerscout.cpp
)

set (SCWPIC_HEADER_FILES
    app/mainwindow.h
    app/actionmanager.h
    app/graphicsview.h
    app/graphicsscene.h
    app/bottombuttongroup.h
    app/navigatorview.h
    app/settings.h
    app/settingsdialog.h
    app/aboutdialog.h
    app/metadatamodel.h
    app/metadatadialog.h
    app/exiv2wrapper.h
    app/playlistmanager.h
    app/fileopeneventhandler.h
    app/playlistscout.h
    app/playlistscouts/qtscout.h
    app/playlistscouts/explorerscout.h
)

set (SCWPIC_UI_FILES
    app/settingsdialog.ui
    app/aboutdialog.ui
    app/metadatadialog.ui
    app/mainwindow.ui
)

set (SCWPIC_QRC_FILES
    assets/resources.qrc
)

set (SCWPIC_RC_FILES
    # yeah, it's empty.
)

set (EXE_NAME scwpic)

# Translation
file (GLOB SCWPIC_TS_FILES locales/app/*.ts)
set (SCWPIC_CPP_FILES_FOR_I18N ${SCWPIC_CPP_FILES})

find_program (METAGLOT_EXECUTABLE metaglot)
if (NOT METAGLOT_EXECUTABLE)
    message (FATAL_ERROR "metaglot is required to render the localized metadata, install it from https://github.com/SarasasChipWorkshop/metaglot")
endif ()

if (WIN32)
    # Setup Windows RC files
    set (SCWPIC_RC_TEMPLATE_FILE ${CMAKE_CURRENT_BINARY_DIR}/sarasacw-picture.rc.in)
    execute_process (
        COMMAND ${METAGLOT_EXECUTABLE} render rc
                --manifest ${CMAKE_SOURCE_DIR}/dist/sarasacw-picture.rc.in.toml
                --po ${CMAKE_SOURCE_DIR}/locales/winrc/*.po
                --template ${CMAKE_SOURCE_DIR}/dist/sarasacw-picture.rc.in.liquid
                --output ${SCWPIC_RC_TEMPLATE_FILE}
        RESULT_VARIABLE METAGLOT_RESULT
    )
    if (NOT METAGLOT_RESULT EQUAL 0)
        message (FATAL_ERROR "Failed to generate Windows RC file template via MetaGlot (exit code: ${METAGLOT_RESULT}).")
    endif ()

    configure_file (${SCWPIC_RC_TEMPLATE_FILE} ${CMAKE_CURRENT_BINARY_DIR}/sarasacw-picture.rc @ONLY)
    list (APPEND SCWPIC_RC_FILES ${CMAKE_CURRENT_BINARY_DIR}/sarasacw-picture.rc)
endif ()

add_executable (${EXE_NAME}
    ${SCWPIC_HEADER_FILES}
    ${SCWPIC_CPP_FILES}
    ${SCWPIC_UI_FILES}
    ${SCWPIC_QRC_FILES}
    ${SCWPIC_RC_FILES}
)

set(ADD_TRANSLATIONS_ADDITIONAL_ARGS)

if (Qt6_VERSION VERSION_GREATER_EQUAL "6.9.0")
    set(ADD_TRANSLATIONS_ADDITIONAL_ARGS MERGE_QT_TRANSLATIONS)
endif()

if (TRANSLATION_RESOURCE_EMBEDDING)
    qt_add_translations(${EXE_NAME} ${ADD_TRANSLATIONS_ADDITIONAL_ARGS} TS_FILES ${SCWPIC_TS_FILES})
else()
    qt_add_translations(${EXE_NAME} ${ADD_TRANSLATIONS_ADDITIONAL_ARGS} TS_FILES ${SCWPIC_TS_FILES} QM_FILES_OUTPUT_VARIABLE SCWPIC_QM_FILES)
endif()

# YYC MARK:
# We use this to disable that Qt UIC generate code like `QMetaObject::connectSlotsByName(foobar);`
# for the header file of `.ui` file, which cause this application outputs log at runtime like
# `qt.core.qmetaobject.connectslotsbyname: QMetaObject::connectSlotsByName: No matching signal for on_actionXxx_triggered()`,
# because we only use UIC for building user interface and all signals and actions are managed by ourselves.
set_target_properties(${EXE_NAME} PROPERTIES AUTOUIC_OPTIONS "-a")

# YYC MARK:
# This is essential, otherwise Qt UIC generated UI header file
# can not find our header files if there are custom widgets inside UI file.
target_include_directories(${EXE_NAME} PRIVATE app/)

target_sources(${EXE_NAME} PRIVATE ${SCWPIC_QM_FILES})

target_link_libraries (${EXE_NAME} Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::SvgWidgets)

if (exiv2_FOUND)
    if(NOT TARGET Exiv2::exiv2lib AND TARGET exiv2lib)
        # for exiv2 0.27.x and (macOS?) conan build
        add_library(Exiv2::exiv2lib ALIAS exiv2lib)
    endif()
    target_link_libraries (${EXE_NAME}
        Exiv2::exiv2lib
    )
    target_compile_definitions(${EXE_NAME} PRIVATE
        HAVE_EXIV2_VERSION="${exiv2_VERSION}"
    )
endif ()

if (TARGET Qt6::DBus)
    target_link_libraries (${EXE_NAME}
        Qt${QT_VERSION_MAJOR}::DBus
    )
    target_compile_definitions(${EXE_NAME} PRIVATE
        HAVE_QTDBUS
    )
endif()

# Extra build settings
if (WIN32)
    target_compile_definitions(${EXE_NAME} PRIVATE
        FLAG_PORTABLE_MODE_SUPPORT=1
    )
endif ()

# Helper macros for parsing and setting project version from `git describe --long` result
macro (scwpic_set_version_via_describe _describe_long)
    string (
        REGEX REPLACE
        "^([0-9a-z.]*)-[0-9]+-g[0-9a-f]*$"
        "\\1"
        _tag_parts
        "${_describe_long}"
    )
    list (GET _tag_parts 0 _matched_tag_version)
    if ("${_matched_tag_version}" MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        string (
            REGEX REPLACE
            "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$"
            "\\1;\\2;\\3"
            _ver_parts
            "${_matched_tag_version}"
        )
        list (GET _ver_parts 0 CPACK_PACKAGE_VERSION_MAJOR)
        list (GET _ver_parts 1 CPACK_PACKAGE_VERSION_MINOR)
        list (GET _ver_parts 2 CPACK_PACKAGE_VERSION_PATCH)
    endif ()
endmacro ()

# Version setup
target_compile_definitions(${EXE_NAME} PRIVATE SCWPIC_VERSION_STRING="${CMAKE_PROJECT_VERSION}")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
    find_package(Git)
    set_package_properties(Git PROPERTIES TYPE OPTIONAL PURPOSE "Determine exact build version.")
    if (GIT_FOUND)
        execute_process (
            COMMAND ${GIT_EXECUTABLE} describe --tags --always --long
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE _git_describe_long
        )
        string (REGEX REPLACE "\n" "" _git_describe_long "${_git_describe_long}")
        scwpic_set_version_via_describe(${_git_describe_long})
        target_compile_definitions(${EXE_NAME} PRIVATE
            GIT_DESCRIBE_VERSION_STRING="${_git_describe_long}"
        )
    endif ()
endif ()

# Install settings
if (WIN32)
    set_target_properties(${EXE_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Setup Inno Setup script file
    set (INNO_SETUP_VERSION "7" CACHE STRING "Major version of Inno Setup that packages the installer; decides which language files count as official in the metaglot inno rendering.")
    set_property (CACHE INNO_SETUP_VERSION PROPERTY STRINGS 6 7)
    set (SCWPIC_INNO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/inno-metadata)
    execute_process (
        COMMAND ${METAGLOT_EXECUTABLE} render inno
                --inno-version ${INNO_SETUP_VERSION}
                --manifest ${CMAKE_SOURCE_DIR}/dist/scwpic.iss.in.toml
                --po ${CMAKE_SOURCE_DIR}/locales/inno/*.po
                --iss-template ${CMAKE_SOURCE_DIR}/dist/scwpic.iss.in.liquid
                --isl-template ${CMAKE_SOURCE_DIR}/dist/scwpic.isl.liquid
                --iss-path scwpic.iss.in
                --isl-path "Strings/{{ lang | default: name }}.isl"
                --output ${SCWPIC_INNO_OUTPUT_DIR}
        RESULT_VARIABLE METAGLOT_INNO_RESULT
    )
    if (NOT METAGLOT_INNO_RESULT EQUAL 0)
        message (FATAL_ERROR "Failed to render Inno Setup script via MetaGlot (exit code: ${METAGLOT_INNO_RESULT}).")
    endif ()
    configure_file (${SCWPIC_INNO_OUTPUT_DIR}/scwpic.iss.in ${SCWPIC_INNO_OUTPUT_DIR}/scwpic.iss @ONLY)

    # Setup MSIX metadata.
    set (SCWPIC_MSIX_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/msix-metadata)
    execute_process (
        COMMAND ${METAGLOT_EXECUTABLE} render msix
                --manifest ${CMAKE_SOURCE_DIR}/dist/AppxManifest.xml.in.toml
                --po ${CMAKE_SOURCE_DIR}/locales/msix/*.po
                --manifest-template ${CMAKE_SOURCE_DIR}/dist/AppxManifest.xml.in.liquid
                --resources-template ${CMAKE_SOURCE_DIR}/dist/Resources.resjson.liquid
                --manifest-path AppxManifest.xml.in
                --resources-path "Strings/{{ lang }}/Resources.resjson"
                --output ${SCWPIC_MSIX_OUTPUT_DIR}
        RESULT_VARIABLE METAGLOT_MSIX_RESULT
    )
    if (NOT METAGLOT_MSIX_RESULT EQUAL 0)
        message (FATAL_ERROR "Failed to render MSIX metadata via MetaGlot (exit code: ${METAGLOT_MSIX_RESULT}).")
    endif ()
    configure_file (${SCWPIC_MSIX_OUTPUT_DIR}/AppxManifest.xml.in ${SCWPIC_MSIX_OUTPUT_DIR}/AppxManifest.xml @ONLY)
elseif (APPLE)
    # Include icon to project
    set_source_files_properties(dist/macos-icons/sarasacw-picture.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${EXE_NAME} PRIVATE dist/macos-icons/sarasacw-picture.icns)

    # Setup Info.plist related directory.
    set (SCWPIC_PLIST_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/plist-metadata)
    # Render Info.plist and its strings.
    execute_process (
        COMMAND ${METAGLOT_EXECUTABLE} render plist
                --manifest ${CMAKE_SOURCE_DIR}/dist/Info.plist.in.toml
                --po ${CMAKE_SOURCE_DIR}/locales/plist/*.po
                --plist-template ${CMAKE_SOURCE_DIR}/dist/Info.plist.in.liquid
                --strings-template ${CMAKE_SOURCE_DIR}/dist/InfoPlist.strings.liquid
                --plist-path Info.plist.in
                --strings-path "Resources/{{ lang }}.lproj/InfoPlist.strings"
                --output ${SCWPIC_PLIST_OUTPUT_DIR}
        RESULT_VARIABLE METAGLOT_PLIST_RESULT
    )
    if (NOT METAGLOT_PLIST_RESULT EQUAL 0)
        message (FATAL_ERROR "Failed to render Info.plist metadata via MetaGlot (exit code: ${METAGLOT_PLIST_RESULT}).")
    endif ()
    # Include these to project.
    file (GLOB SCWPIC_PLIST_STRINGS_FILES ${SCWPIC_PLIST_OUTPUT_DIR}/Resources/*.lproj/InfoPlist.strings)
    foreach (plist_strings_file ${SCWPIC_PLIST_STRINGS_FILES})
        get_filename_component (plist_lproj_dir ${plist_strings_file} DIRECTORY)
        get_filename_component (plist_lproj_name ${plist_lproj_dir} NAME)
        set_source_files_properties (${plist_strings_file} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/${plist_lproj_name}"
        )
        target_sources (${EXE_NAME} PRIVATE ${plist_strings_file})
    endforeach ()
    # See https://cmake.org/cmake/help/v3.15/prop_tgt/MACOSX_BUNDLE_INFO_PLIST.html
    set_target_properties(${EXE_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${SCWPIC_PLIST_OUTPUT_DIR}/Info.plist.in
        MACOSX_BUNDLE_INFO_STRING "${PROJECT_VERSION}, Copyright (C) 2026 Sarasa's Chip Workshop"
        MACOSX_BUNDLE_GUI_IDENTIFIER org.sarasaschipworkshop.picture
        MACOSX_BUNDLE_ICON_FILE sarasacw-picture.icns # contains the .icns file name, *without* the path.
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    )
elseif (UNIX)
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX /usr)
    endif ()

    # install icon
    install (
        DIRECTORY dist/freedesktop-icons/
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor"
    )

    # generate and install desktop entry file
    set (SCWPIC_DESKTOP_FILE ${CMAKE_CURRENT_BINARY_DIR}/org.sarasaschipworkshop.picture.desktop)
    execute_process (
        COMMAND ${METAGLOT_EXECUTABLE} render desktop
                --manifest ${CMAKE_SOURCE_DIR}/dist/org.sarasaschipworkshop.picture.desktop.toml
                --po ${CMAKE_SOURCE_DIR}/locales/desktop/*.po
                --template ${CMAKE_SOURCE_DIR}/dist/org.sarasaschipworkshop.picture.desktop.liquid
                --output ${SCWPIC_DESKTOP_FILE}
        RESULT_VARIABLE METAGLOT_DESKTOP_RESULT
    )
    if (NOT METAGLOT_DESKTOP_RESULT EQUAL 0)
        message (FATAL_ERROR "Failed to generate .desktop via MetaGlot (exit code: ${METAGLOT_DESKTOP_RESULT}).")
    endif ()

    install (
        FILES ${SCWPIC_DESKTOP_FILE}
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    )

    # generate and install app metadata file for appstream (and some other stuff using this metadata like snapcraft)
    set (SCWPIC_METAINFO_FILE ${CMAKE_CURRENT_BINARY_DIR}/org.sarasaschipworkshop.picture.metainfo.xml)
    execute_process (
        COMMAND ${METAGLOT_EXECUTABLE} render appstream
                --manifest ${CMAKE_SOURCE_DIR}/dist/org.sarasaschipworkshop.picture.metainfo.xml.toml
                --po ${CMAKE_SOURCE_DIR}/locales/appstream/*.po
                --template ${CMAKE_SOURCE_DIR}/dist/org.sarasaschipworkshop.picture.metainfo.xml.liquid
                --output ${SCWPIC_METAINFO_FILE}
        RESULT_VARIABLE METAGLOT_METAINFO_RESULT
    )
    if (NOT METAGLOT_METAINFO_RESULT EQUAL 0)
        message (FATAL_ERROR "Failed to generate .metainfo.xml via MetaGlot (exit code: ${METAGLOT_METAINFO_RESULT}).")
    endif ()

    install (
        FILES ${SCWPIC_METAINFO_FILE}
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
    )
endif()

set (INSTALL_TARGETS_DEFAULT_ARGS
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Devel
)

install (
    TARGETS ${EXE_NAME}
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

if (TRANSLATION_RESOURCE_EMBEDDING)
    target_compile_definitions(${EXE_NAME}
        PRIVATE TRANSLATION_RESOURCE_EMBEDDING
    )
elseif (WIN32)
    set (QM_FILE_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}/translations")
else()
    set (QM_FILE_INSTALL_DIR "${CMAKE_INSTALL_FULL_DATADIR}/sarasacw-picture/translations")
    target_compile_definitions(${EXE_NAME}
        PRIVATE QM_FILE_INSTALL_ABSOLUTE_DIR=${QM_FILE_INSTALL_DIR}
    )
endif()

if (DEFINED QM_FILE_INSTALL_DIR)
    install(
        FILES ${SCWPIC_QM_FILES}
        DESTINATION ${QM_FILE_INSTALL_DIR}
    )
endif()
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

# CPACK: General Settings
set (CPACK_GENERATOR "TBZ2")
set (CPACK_PACKAGE_NAME "sarasacw-picture")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Yet another image viewer")
set (CPACK_PACKAGE_VENDOR "Sarasa's Chip Workshop")
set (CPACK_PACKAGE_CONTACT "https://github.com/SarasasChipWorkshop/sarasacw-picture/issues/")
if (WIN32)
    # ...
elseif (APPLE)
    # ...
elseif (UNIX)
    set (CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set (CPACK_DEBIAN_PACKAGE_RECOMMENDS "kimageformat6-plugins")
endif()

include(CPack)
