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

target_include_directories(mangareader PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(mangareader
    PRIVATE
        Qt5::Widgets
        KF5::CoreAddons
        KF5::I18n
        KF5::XmlGui
        KF5::ConfigCore
        QArchive
)

install(TARGETS mangareader DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES settings/mangareaderui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
install(FILES settings/viewui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/mangareader)
install(FILES settings/settings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
