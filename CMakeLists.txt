cmake_minimum_required(VERSION 3.10)
project(mangareader LANGUAGES CXX)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

include(KDEInstallDirs)
include(ECMInstallIcons)
include(FetchContent)

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

FetchContent_Declare(
    QArchive
    GIT_REPOSITORY "https://github.com/antony-jr/QArchive"
    GIT_TAG "62d2e1690ddbcfda099e569b40b1c1f86f1f507e"
)
FetchContent_MakeAvailable(QArchive)

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
    PRIVATE
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
install(FILES data/com.georgefb.mangareader.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(PROGRAMS data/com.georgefb.mangareader.desktop DESTINATION ${KDE_INSTALL_APPDIR})
