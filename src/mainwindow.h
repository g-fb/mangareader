/*
 * Copyright 2019 Florea Banus George <georgefb899@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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

    void loadImages(const QString& path, bool recursive = false, bool updateCurrentPath = true);

private:
    static void showError(const QString& error);
    void init();
    void setupMangaTreeDockWidget();
    void setupBookmarksDockWidget();
    void setupRenameDialog();
    void setupActions();
    void openMangaFolder();
    void openMangaArchive();
    void toggleFullScreen();
    void extractArchive(const QString& archivePath);
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
    void renameFile();

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
};

#endif // MAINWINDOW_H
