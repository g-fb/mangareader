cmake_minimum_required(VERSION 3.0)
project(mangareader)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

include(KDEInstallDirs)
include(ECMInstallIcons)

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
)
find_package(KF5 REQUIRED COMPONENTS
    CoreAddons
    I18n
    XmlGui
    Config
)

add_subdirectory(QArchive/)

set(mangareader_SRC
    src/main.cpp
    src/mainwindow.cpp
    src/view.cpp
    src/page.cpp
    src/worker.cpp
)

ki18n_wrap_ui(mangareader_SRC
   settings.ui
)

kconfig_add_kcfg_files(mangareader_SRC GENERATE_MOC settings.kcfgc)
add_executable(${PROJECT_NAME} ${mangareader_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(mangareader
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::XmlGui
    KF5::ConfigCore
    QArchive
)

ecm_install_icons(ICONS data/512-apps-mangareader.svg DESTINATION ${KDE_INSTALL_ICONDIR})

install(TARGETS ${PROJECT_NAME} DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES mangareaderui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
install(FILES mangareader.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(PROGRAMS data/com.georgefb.mangareader.desktop DESTINATION ${KDE_INSTALL_APPDIR})
