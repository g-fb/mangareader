/*
 * Copyright 2019 George Florea Banus
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KSharedConfig>
#include <KXmlGuiWindow>

#include "ui_settings.h"

class QProgressBar;
class QStandardItemModel;
class QTableView;
class QTreeView;
class View;
class Worker;

class SettingsWidget: public QWidget, public Ui::SettingsWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent) : QWidget(parent) {
        setupUi(this);
    }
};


class MainWindow : public KXmlGuiWindow
{
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Qt::ToolBarArea mainToolBarArea();

private:
    void init();
    void setupActions();
    void addMangaFolder();
    void saveMangaFolders();
    void loadImages(QString path, bool recursive = false);
    void toggleFullScreen();
    void extractArchive(QString archivePath);
    void treeViewContextMenu(QPoint point);
    void hideDockWidgets(Qt::DockWidgetAreas area = Qt::AllDockWidgetAreas);
    void showDockWidgets(Qt::DockWidgetAreas area = Qt::AllDockWidgetAreas);
    void hideToolBars(Qt::ToolBarAreas area = Qt::AllToolBarAreas);
    void showToolBars(Qt::ToolBarAreas area = Qt::AllToolBarAreas);
    void onMouseMoved(QMouseEvent *event);
    void onAddBookmark(int pageNumber);
    void deleteBookmarks(QTableView *tableView, QString name);
    void openSettings();
    bool isFullScreen();
    QMenu *mangaFoldersMenu();

    QProgressBar       *m_progressBar;
    KSharedConfig::Ptr  m_config;
    QStringList         m_images;
    QString             m_currentManga;
    View               *m_view;
    Worker             *m_worker;
    QThread            *m_thread;
    Qt::ToolBarArea     m_mainToolBarArea;
    QTreeView          *m_treeView;
    QString             m_tmpFolder;
    QStandardItemModel *m_bookmarksModel;
    QTableView         *m_bookmarksView;
    QAction            *m_selectMangaFolder;
    SettingsWidget     *m_settingsWidget = nullptr;
};

#endif // MAINWINDOW_H
