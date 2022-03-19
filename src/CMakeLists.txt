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
        Qt5::Widgets
        KF5::Archive
        KF5::ConfigCore
        KF5::ConfigWidgets
        KF5::CoreAddons
        KF5::I18n
        KF5::KIOCore
        KF5::KIOFileWidgets
        KF5::KIOWidgets
        KF5::KIONTLM
        KF5::XmlGui
)
target_include_directories(mangareader PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (KArchive_HAVE_LZMA)
    target_compile_definitions(mangareader PRIVATE -DWITH_K7ZIP=1)
endif()

install(TARGETS mangareader DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES settings/mangareaderui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
install(FILES settings/viewui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
install(FILES settings/settings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
