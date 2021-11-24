cmake_minimum_required(VERSION 3.10)
project(mangareader LANGUAGES CXX)

find_package(ECM 5.66 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

include(KDEInstallDirs)
include(ECMInstallIcons)
include(FetchContent)
include(FeatureSummary)

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
            GIT_TAG "v2.1.1"
            )
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
