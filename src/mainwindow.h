/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KSharedConfig>
#include <KXmlGuiWindow>

class KHamburgerMenu;
class QPushButton;
class StartUpWidget;
class QProgressBar;
class QStandardItemModel;
class QTableView;
class QTreeView;
class View;
class Worker;
class QFileInfo;
class QFileSystemModel;
class SettingsWindow;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum {
        IndexRole = Qt::UserRole,
        KeyRole,
        PathRole,
        RecursiveRole
    };

    void loadImages(const QString &path, bool recursive = false, bool updateCurrentPath = true);

private:
    static void showError(const QString &error);
    void init();
    void setupMangaTreeDockWidget();
    void setupBookmarksDockWidget();
    void setupActions();
    void openMangaFolder();
    void openMangaArchive();
    void toggleFullScreen();
    void extractArchive(const QString &archivePath);
    void extractRarArchive(const QString &archivePath);
    void treeViewContextMenu(QPoint point);
    void bookmarksViewContextMenu(QPoint point);
    void hideDockWidgets(Qt::DockWidgetAreas area = Qt::AllDockWidgetAreas);
    void showDockWidgets(Qt::DockWidgetAreas area = Qt::AllDockWidgetAreas);
    void hideToolBars(Qt::ToolBarAreas area = Qt::AllToolBarAreas);
    void showToolBars(Qt::ToolBarAreas area = Qt::AllToolBarAreas);
    void onMouseMoved(QMouseEvent *event);
    void onAddBookmark(int pageIndex);
    void deleteBookmarks(QTableView *tableView);
    void openSettings();
    void toggleMenubar();
    void setToolBarVisible(bool visible);
    void toggleFitHeight();
    void toggleFitWidth();
    auto isFullScreen() -> bool;
    auto populateMangaFoldersMenu() -> QMenu *;
    void populateBookmarkModel();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

    KSharedConfig::Ptr  m_config;
    KHamburgerMenu     *m_hamburgerMenu;
    QStringList         m_images;
    View               *m_view;
    QDockWidget        *m_treeDock;
    QTreeView          *m_treeView;
    QFileSystemModel   *m_treeModel;
    QDockWidget        *m_bookmarksDock;
    QTableView         *m_bookmarksView;
    QStandardItemModel *m_bookmarksModel;
    Worker             *m_worker{};
    QThread            *m_thread{};
    QMenu              *m_mangaFoldersMenu{};
    QProgressBar       *m_progressBar{};
    QString             m_tmpFolder;
    QString             m_currentPath;
    QPushButton        *m_selectMangaLibraryButton{nullptr};
    SettingsWindow     *m_settingsWindow;
    QDialog            *m_renameDialog{};
    StartUpWidget      *m_startUpWidget;
    int                 m_startPage{ 0 };
    bool                m_isLoadedRecursive{ false };
    const QString       RECURSIVE_KEY_PREFIX = ":recursive:";
    QStringList         m_supportedMimeTypes {"application/zip",
                                              "application/vnd.comicbook+zip",
                                              "application/x-7z-compressed",
                                              "application/x-cb7",
                                              "application/x-tar",
                                              "application/x-cbt",
                                              "application/vnd.rar",
                                              "application/vnd.comicbook-rar",
                                              "inode/directory"};
};

#endif // MAINWINDOW_H
