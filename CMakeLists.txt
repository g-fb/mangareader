cmake_minimum_required(VERSION 3.10)
project(mangareader VERSION 1.6.0 LANGUAGES CXX)

find_package(ECM 5.66 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(KDEInstallDirs)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(ECMSetupVersion)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FetchContent)
include(FeatureSummary)

kde_enable_exceptions()

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX MANGAREADER
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/mangareader-version.h"
)

find_package(Qt5Core)
set_package_properties(Qt5Core PROPERTIES TYPE REQUIRED)

find_package(Qt5Widgets)
set_package_properties(Qt5Widgets PROPERTIES TYPE REQUIRED)

find_package(KF5Config)
set_package_properties(KF5Config PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kconfig/html/index.html")

find_package(KF5ConfigWidgets)
set_package_properties(KF5ConfigWidgets PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kconfigwidgets/html/index.html")

find_package(KF5CoreAddons)
set_package_properties(KF5CoreAddons PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kcoreaddons/html/index.html")

find_package(KF5I18n)
set_package_properties(KF5I18n PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/ki18n/html/index.html")

find_package(KF5XmlGui)
set_package_properties(KF5XmlGui PROPERTIES TYPE REQUIRED
    URL "https://api.kde.org/frameworks/kxmlgui/html/index.html")

find_package(QArchive)
set_package_properties(QArchive PROPERTIES TYPE OPTIONAL
    URL "https://github.com/antony-jr/QArchive")

# Only used if find_package(QArchive) fails.
# Wheter to use FetchContent or add_subdirectory to provide the QArchive dependency
# there is no internet access when building packages for OpenSuse
option(UseFetchContent
    "Only used if find_package(QArchive) fails. Wheter to use FetchContent or add_subdirectory to provide the QArchive dependency."
    ON)

if(NOT ${QArchive_FOUND})
    if(UseFetchContent)
        FetchContent_Declare(
            QArchive
            GIT_REPOSITORY "https://github.com/antony-jr/QArchive"
            GIT_TAG "master")
        FetchContent_MakeAvailable(QArchive)
    else()
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/QArchive)
            add_subdirectory(QArchive)
        else()
            message("QArchive folder doesn't exist. https://github.com/antony-jr/QArchive")
        endif()
    endif()
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_subdirectory(data)
add_subdirectory(src)
