#
# SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

cmake_minimum_required(VERSION 3.10)
project(mangareader VERSION 2.1.0 LANGUAGES CXX)

find_package(ECM 5.66 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(KDEInstallDirs)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(ECMSetupVersion)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX MANGAREADER
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/mangareader-version.h"
)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

find_package(Qt${QT_MAJOR_VERSION}Core)
set_package_properties(Qt${QT_MAJOR_VERSION}Core PROPERTIES TYPE REQUIRED)

find_package(Qt${QT_MAJOR_VERSION}Widgets)
set_package_properties(Qt${QT_MAJOR_VERSION}Widgets PROPERTIES TYPE REQUIRED)

find_package(KF${QT_MAJOR_VERSION}Archive)
set_package_properties(KF${QT_MAJOR_VERSION}Archive PROPERTIES TYPE OPTIONAL
    URL "https://api.kde.org/frameworks/karchive/html/index.html")

find_package(KF${QT_MAJOR_VERSION}Config)
set_package_properties(KF${QT_MAJOR_VERSION}Config PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kconfig/html/index.html")

find_package(KF${QT_MAJOR_VERSION}ConfigWidgets)
set_package_properties(KF${QT_MAJOR_VERSION}ConfigWidgets PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kconfigwidgets/html/index.html")

find_package(KF${QT_MAJOR_VERSION}CoreAddons)
set_package_properties(KF${QT_MAJOR_VERSION}CoreAddons PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kcoreaddons/html/index.html")

find_package(KF${QT_MAJOR_VERSION}I18n)
set_package_properties(KF${QT_MAJOR_VERSION}I18n PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/ki18n/html/index.html")

find_package(KF${QT_MAJOR_VERSION}KIO)
set_package_properties(KF${QT_MAJOR_VERSION}KIO PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kio/html/index.html")

find_package(KF${QT_MAJOR_VERSION}XmlGui)
set_package_properties(KF${QT_MAJOR_VERSION}XmlGui PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kxmlgui/html/index.html")

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_subdirectory(data)
add_subdirectory(src)
