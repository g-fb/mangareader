cmake_minimum_required(VERSION 3.0)
project(mangareader)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_AUTOMOC ON)

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

set(mangareader_SRC
  src/main.cpp
  src/mainwindow.cpp
  src/view.cpp
  src/page.cpp
  src/worker.cpp
)
add_executable(${PROJECT_NAME} ${mangareader_SRC})

target_link_libraries(mangareader
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::XmlGui
    KF5::ConfigCore
)
