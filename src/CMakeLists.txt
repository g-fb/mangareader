kconfig_add_kcfg_files(mangareader_SRC GENERATE_MOC settings/settings.kcfgc)
add_executable(mangareader)

target_sources(mangareader
    PRIVATE
        main.cpp
        mainwindow.cpp
        view.cpp
        page.cpp
        worker.cpp
        settingswindow.cpp
        settings/resources.qrc
        ${mangareader_SRC}
)

if(WIN32)
    set(ICONS_FOLDER ${PROJECT_SOURCE_DIR}/data/icons/windows)

    set(ICON_FILES
        ${PROJECT_SOURCE_DIR}/data/sc-apps-mangareader.svg
        ${ICONS_FOLDER}/16-apps-mangareader.png
        ${ICONS_FOLDER}/22-apps-mangareader.png
        ${ICONS_FOLDER}/32-apps-mangareader.png
        ${ICONS_FOLDER}/44-apps-mangareader.png
        ${ICONS_FOLDER}/48-apps-mangareader.png
        ${ICONS_FOLDER}/64-apps-mangareader.png
        ${ICONS_FOLDER}/128-apps-mangareader.png
        ${ICONS_FOLDER}/150-apps-mangareader.png
        ${ICONS_FOLDER}/256-apps-mangareader.png
        ${ICONS_FOLDER}/310-apps-mangareader.png
        ${ICONS_FOLDER}/512-apps-mangareader.png
    )

    ecm_add_app_icon(ICONS_SOURCES ICONS ${ICON_FILES})
    target_sources(mangareader PRIVATE ${ICONS_SOURCES})

    ecm_install_icons(
        ICONS ${ICON_FILES}
        DESTINATION ${KDE_INSTALL_ICONDIR}
        THEME hicolor
    )
endif()

target_include_directories(mangareader PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(mangareader
    PRIVATE
        Qt5::Widgets
        KF5::ConfigCore
        KF5::ConfigWidgets
        KF5::CoreAddons
        KF5::I18n
        KF5::XmlGui
        QArchive
)
target_include_directories(mangareader PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(mangareader
    PRIVATE
        Qt5::Widgets
        KF5::ConfigCore
        KF5::ConfigWidgets
        KF5::CoreAddons
        KF5::I18n
        KF5::XmlGui
        QArchive
)

install(TARGETS mangareader DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES settings/mangareaderui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
install(FILES settings/viewui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
install(FILES settings/settings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
