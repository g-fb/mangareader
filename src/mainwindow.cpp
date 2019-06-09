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

#include "_debug.h"
#include "mainwindow.h"
#include "view.h"
#include "worker.h"

#include <KActionCollection>
#include <KConfigGroup>
#include <KToolBar>
#include <KLocalizedString>

#include <QtWidgets>
#include <QThread>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    // setup central widget
    auto centralWidget = new QWidget(this);
    auto centralWidgetLayout = new QVBoxLayout(centralWidget);
    centralWidgetLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centralWidget);

    m_config = KSharedConfig::openConfig("fbg/mangareaderrc");

    init();
    setupActions();
    setupGUI(Default, "mangareaderui.rc");

    connect(m_view, &View::doubleClicked,
            this, &MainWindow::toggleFullScreen);
    connect(m_view, &View::mouseMoved,
            this, &MainWindow::onMouseMoved);

    showToolBars();
    showDockWidgets();
    menuBar()->show();
    statusBar()->show();
}

MainWindow::~MainWindow()
{
    m_thread->quit();
    m_thread->wait();
}

Qt::ToolBarArea MainWindow::mainToolBarArea()
{
    return m_mainToolBarArea;
}

void MainWindow::init()
{
    KConfigGroup rootGroup = m_config->group("");
    QFileInfo mangaDirInfo(rootGroup.readEntry("Manga Folder"));
    // setup progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setVisible(false);
    centralWidget()->layout()->addWidget(m_progressBar);

    // setup view
    m_view = new View(this);
    centralWidget()->layout()->addWidget(m_view);

    m_worker = Worker::instance();
    m_thread = new QThread(this);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished,
            m_worker, &Worker::deleteLater);
    connect(m_thread, &QThread::finished,
            m_thread, &QThread::deleteLater);
    m_thread->start();

    // setup dock widgets
    // tree widget
    auto treeDock = new QDockWidget(mangaDirInfo.baseName(), this);
    auto treeModel = new QFileSystemModel(this);
    m_treeView = new QTreeView(this);

    treeDock->setObjectName("treeDock");

    treeModel->setObjectName("mangaTree");
    treeModel->setRootPath(mangaDirInfo.absoluteFilePath());
    treeModel->setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    treeModel->setNameFilters(QStringList() << "*.zip" << "*.7z" << "*.cbz");
    treeModel->setNameFilterDisables(false);

    m_treeView->setModel(treeModel);
    m_treeView->setRootIndex(treeModel->index(mangaDirInfo.absoluteFilePath()));
    m_treeView->setColumnHidden(1, true);
    m_treeView->setColumnHidden(2, true);
    m_treeView->setColumnHidden(3, true);
    m_treeView->header()->hide();
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_treeView, &QTreeView::doubleClicked,
            this, [ = ](const QModelIndex &index) {
                loadImages(index);
            });
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::treeViewContextMenu);

    treeDock->setWidget(m_treeView);
    addDockWidget(Qt::LeftDockWidgetArea, treeDock);
}

void MainWindow::addMangaFolder()
{
    QString path = QFileDialog::getExistingDirectory(this, i18n("Choose a directory"), "");
    if (path.isEmpty()) {
        return;
    }

    KConfigGroup rootGroup = m_config->group("");
    rootGroup.writeEntry("Manga Folder", path);
    rootGroup.config()->sync();
}

void MainWindow::loadImages(const QModelIndex &index, bool recursive)
{
    // get path from index
    const QFileSystemModel *model = static_cast<const QFileSystemModel *>(index.model());
    QString pathToLoad = model->filePath(index);
    const QFileInfo fileInfo(pathToLoad);
    if (!fileInfo.isDir()) {
        return;
    }

    m_images.clear();

    // get images from path
    auto it = new QDirIterator(fileInfo.absoluteFilePath(), QDir::Files, QDirIterator::NoIteratorFlags);
    if (recursive) {
      it = new QDirIterator(fileInfo.absoluteFilePath(), QDir::Files, QDirIterator::Subdirectories);
    }
    while (it->hasNext()) {
        QString file = it->next();
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(file);
        // only get images
        if (type.name().startsWith("image/")) {
            m_images.append(file);
        }
    }
    // natural sort images
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(m_images.begin(), m_images.end(), collator);

    if (m_images.count() < 1) {
        return;
    }
    m_currentManga = fileInfo.absoluteFilePath();
    m_worker->setImages(m_images);
    m_view->reset();
    m_view->setManga(m_currentManga);
    m_view->setImages(m_images);
    m_view->loadImages();
}

void MainWindow::setupActions()
{
    auto addMangaFolder = new QAction(this);
    addMangaFolder->setText(i18n("&Add Manga Folder"));
    addMangaFolder->setIcon(QIcon::fromTheme("folder-open"));
    actionCollection()->addAction("addMangaFolder", addMangaFolder);
    actionCollection()->setDefaultShortcut(addMangaFolder, Qt::CTRL + Qt::Key_A);
    connect(addMangaFolder, &QAction::triggered,
            this, &MainWindow::addMangaFolder);

    KStandardAction::quit(qApp, &QCoreApplication::quit, actionCollection());

    auto spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setVisible(true);
    auto spacerAction = new QWidgetAction(this);
    spacerAction->setDefaultWidget(spacer);
    spacerAction->setText(i18n("Spacer"));
    actionCollection()->addAction("spacer", spacerAction);

    auto fullScreen = new QAction(this);
    fullScreen->setText(i18n("FullScreen"));
    fullScreen->setIcon(QIcon::fromTheme("view-fullscreen"));
    actionCollection()->addAction("fullscreen", fullScreen);
    actionCollection()->setDefaultShortcut(fullScreen, Qt::Key_F11);
    connect(fullScreen, &QAction::triggered,
            this, &MainWindow::toggleFullScreen);
}

bool MainWindow::isFullScreen()
{
    return (windowState() == (Qt::WindowFullScreen | Qt::WindowMaximized))
            || (windowState() == Qt::WindowFullScreen);
}

void MainWindow::treeViewContextMenu(QPoint point)
{
    QModelIndex index = m_treeView->indexAt(point);
    QFileSystemModel *model = static_cast<QFileSystemModel *>(m_treeView->model());
    QString path = model->filePath(index);
    QFileInfo pathInfo(path);

    QMenu *menu = new QMenu();
    menu->setMinimumWidth(200);

    QAction *load = new QAction(QIcon::fromTheme("arrow-down"), i18n("Load"));
    m_treeView->addAction(load);

    QAction *loadRecursive = new QAction(QIcon::fromTheme("arrow-down-double"), i18n("Load recursive"));
    m_treeView->addAction(loadRecursive);

    QAction *openPath = new QAction(QIcon::fromTheme("unknown"), i18n("Open"));
    m_treeView->addAction(openPath);

    QAction *openContainingFolder = new QAction(QIcon::fromTheme("folder-open"), i18n("Open containing folder"));
    m_treeView->addAction(openContainingFolder);

    menu->addAction(load);
    menu->addAction(loadRecursive);
    menu->addSeparator();
    menu->addAction(openPath);
    menu->addAction(openContainingFolder);

    connect(load, &QAction::triggered,
            this, [ = ]() {
                loadImages(index);
            });
    connect(loadRecursive, &QAction::triggered,
            this, [ = ]() {
                loadImages(index, true);
            });

    connect(openPath, &QAction::triggered,
            this, [ = ]() {
                QString nativePath = QDir::toNativeSeparators(pathInfo.absoluteFilePath());
                QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath));
            });

    connect(openContainingFolder, &QAction::triggered,
            this, [ = ]() {
                QString nativePath = QDir::toNativeSeparators(pathInfo.absolutePath());
                QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath));
            });
    menu->exec(QCursor::pos());
}

void MainWindow::hideDockWidgets(Qt::DockWidgetAreas area)
{
    QList<QDockWidget *> dockWidgets = findChildren<QDockWidget *>();
    for (QDockWidget *dockWidget : dockWidgets) {
        if ((dockWidgetArea(dockWidget) == area || area == Qt::AllDockWidgetAreas) && !dockWidget->isFloating()) {
            dockWidget->hide();
        }
    }
}

void MainWindow::showDockWidgets(Qt::DockWidgetAreas area)
{
    QList<QDockWidget *> dockWidgets = findChildren<QDockWidget *>();
    for (int i = dockWidgets.size(); i > 0; i--) {
        if ((dockWidgetArea(dockWidgets.at(i-1)) == area || area == Qt::AllDockWidgetAreas)
                && !dockWidgets.at(i-1)->isFloating()) {
            dockWidgets.at(i-1)->show();
        }
    }
}

void MainWindow::hideToolBars(Qt::ToolBarAreas area)
{
    QList<QToolBar *> toolBars = findChildren<QToolBar *>();
    for (QToolBar *toolBar : toolBars) {
        if ((toolBarArea(toolBar) == area || area == Qt::AllToolBarAreas) && !toolBar->isFloating()) {
            toolBar->hide();
        }
    }
}

void MainWindow::showToolBars(Qt::ToolBarAreas area)
{
    QList<QToolBar *> toolBars = findChildren<QToolBar *>();
    for (QToolBar *toolBar : toolBars) {
        if ((toolBarArea(toolBar) == area || area == Qt::AllToolBarAreas) && !toolBar->isFloating()) {
            toolBar->show();
        }
    }
}

void MainWindow::onMouseMoved(QMouseEvent *event)
{
    if(!isFullScreen()) {
        return;
    }
    if (event->y() < 50) {
        showDockWidgets(Qt::TopDockWidgetArea);
        showToolBars(Qt::TopToolBarArea);
    } else if (event->y() > m_view->height() - 50) {
        showDockWidgets(Qt::BottomDockWidgetArea);
        showToolBars(Qt::BottomToolBarArea);
    } else if (event->x() < 50) {
        showDockWidgets(Qt::LeftDockWidgetArea);
        showToolBars(Qt::LeftToolBarArea);
    } else if (event->x() > m_view->width() - 50) {
        showDockWidgets(Qt::RightDockWidgetArea);
        showToolBars(Qt::RightToolBarArea);
    } else {
        hideDockWidgets();
        hideToolBars();
    }
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        showToolBars();
        showDockWidgets();
        menuBar()->show();
        statusBar()->show();
    } else {
        setWindowState(windowState() | Qt::WindowFullScreen);
        hideToolBars();
        hideDockWidgets();
        menuBar()->hide();
        statusBar()->hide();
    }
}

