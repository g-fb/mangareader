#
# SPDX-FileCopyrightText: 2021 George Florea Bănuș <georgefb899@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

kconfig_add_kcfg_files(SETTINGS_SRCS GENERATE_MOC settings/settings.kcfgc)
# DATA_ICONS is defined in data/CMakeLists.txt
ecm_add_app_icon(ICONS_SRCS ICONS ${DATA_ICONS})

add_executable(mangareader)
target_sources(mangareader
    PRIVATE
        extractor.cpp
        main.cpp
        mainwindow.cpp
        view.cpp
        page.cpp
        worker.cpp
        settingswindow.cpp
        settings/resources.qrc
        startupwidget.cpp
        ${SETTINGS_SRCS}
        ${ICONS_SRCS}
)

target_link_libraries(mangareader
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${QT_MAJOR_VERSION}::Archive
        KF${QT_MAJOR_VERSION}::ConfigCore
        KF${QT_MAJOR_VERSION}::ConfigWidgets
        KF${QT_MAJOR_VERSION}::CoreAddons
        KF${QT_MAJOR_VERSION}::I18n
        KF${QT_MAJOR_VERSION}::KIOWidgets
        KF${QT_MAJOR_VERSION}::XmlGui
)
target_include_directories(mangareader PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (KArchive_HAVE_LZMA)
    target_compile_definitions(mangareader PRIVATE -DWITH_K7ZIP=1)
endif()

install(TARGETS mangareader DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES settings/mangareaderui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
install(FILES settings/viewui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
message("dir: ${KDE_INSTALL_KCFGDIR}")
install(FILES settings/settings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
