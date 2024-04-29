#
# SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

cmake_minimum_required(VERSION 3.10)
project(mangareader VERSION 2.2.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_MIN_VERSION 6.6.0)
set(KF6_MIN_VERSION 6.0.0)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

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

find_package(Qt6Core ${QT_MIN_VERSION})
set_package_properties(Qt6Core PROPERTIES TYPE REQUIRED)

find_package(Qt6Widgets ${QT_MIN_VERSION})
set_package_properties(Qt6Widgets PROPERTIES TYPE REQUIRED)

find_package(KF6Archive ${KF6_MIN_VERSION})
set_package_properties(KF6Archive PROPERTIES TYPE OPTIONAL
    URL "https://api.kde.org/frameworks/karchive/html/index.html")

find_package(KF6Config ${KF6_MIN_VERSION})
set_package_properties(KF6Config PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kconfig/html/index.html")

find_package(KF6ConfigWidgets ${KF6_MIN_VERSION})
set_package_properties(KF6ConfigWidgets PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kconfigwidgets/html/index.html")

find_package(KF6CoreAddons ${KF6_MIN_VERSION})
set_package_properties(KF6CoreAddons PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kcoreaddons/html/index.html")

find_package(KF6I18n ${KF6_MIN_VERSION})
set_package_properties(KF6I18n PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/ki18n/html/index.html")

find_package(KF6KIO ${KF6_MIN_VERSION})
set_package_properties(KF6KIO PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kio/html/index.html")

find_package(KF6XmlGui ${KF6_MIN_VERSION})
set_package_properties(KF6XmlGui PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kxmlgui/html/index.html")

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_subdirectory(data)
add_subdirectory(src)
