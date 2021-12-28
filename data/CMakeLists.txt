set(ICONS_FOLDER ${PROJECT_SOURCE_DIR}/data/icons)
set(ICON_FILES
    ${ICONS_FOLDER}/sc-apps-mangareader.svg
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

# used in src/CMakeLists.txt
set(DATA_ICONS ${ICON_FILES} PARENT_SCOPE)

ecm_install_icons(
    ICONS ${ICON_FILES}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
install(FILES com.georgefb.mangareader.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES com.georgefb.mangareader.desktop DESTINATION ${KDE_INSTALL_APPDIR})
