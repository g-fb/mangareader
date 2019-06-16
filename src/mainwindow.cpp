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
#include "settings.h"

#include <KActionCollection>
#include <KConfigDialog>
#include <KConfigGroup>
#include <KToolBar>
#include <KLocalizedString>

#include <QtWidgets>
#include <QThread>

#include <QArchive>

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    // setup central widget
    auto centralWidget = new QWidget(this);
    auto centralWidgetLayout = new QVBoxLayout(centralWidget);
    centralWidgetLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centralWidget);

    m_config = KSharedConfig::openConfig("fbg/mangareader.conf");

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

    // ==================================================
    // setup progress bar
    // ==================================================
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setVisible(false);
    centralWidget()->layout()->addWidget(m_progressBar);

    // ==================================================
    // setup view
    // ==================================================
    m_view = new View(this);
    connect(m_view, &View::addBookmark,
            this, &MainWindow::onAddBookmark);
    centralWidget()->layout()->addWidget(m_view);

    // ==================================================
    // setup thread and worker
    // ==================================================
    m_worker = Worker::instance();
    m_thread = new QThread(this);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished,
            m_worker, &Worker::deleteLater);
    connect(m_thread, &QThread::finished,
            m_thread, &QThread::deleteLater);
    m_thread->start();

    // ==================================================
    // setup dock widgets
    // tree widget
    // ==================================================
    if (!mangaDirInfo.absoluteFilePath().isEmpty()) {
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
                    // get path from index
                    const QFileSystemModel *model = static_cast<const QFileSystemModel *>(index.model());
                    QString path = model->filePath(index);
                    m_currentManga = path;
                    loadImages(path);
                });
        connect(m_treeView, &QTreeView::customContextMenuRequested,
                this, &MainWindow::treeViewContextMenu);

        treeDock->setWidget(m_treeView);
        addDockWidget(Qt::LeftDockWidgetArea, treeDock);
    }

    // ==================================================
    // bookmarks dock widget
    // ==================================================
    auto bookmarksLayout = new QVBoxLayout();
    auto bookmarksWidget = new QWidget(this);
    auto dockWidget = new QDockWidget();
    dockWidget->setWindowTitle(i18n("Bookmarks"));
    dockWidget->setObjectName("bookmarksDockWidget");
    dockWidget->setMinimumHeight(300);

    m_bookmarksModel = new QStandardItemModel(0, 2, this);
    m_bookmarksModel->setHorizontalHeaderItem(0, new QStandardItem(i18n("Manga")));
    m_bookmarksModel->setHorizontalHeaderItem(1, new QStandardItem(i18n("Page")));

    KConfigGroup bookmarks = m_config->group("Bookmarks");
    for (QString path : bookmarks.keyList()) {
        QFileInfo pathInfo(path);
        auto value = bookmarks.readEntry(pathInfo.absoluteFilePath());
        QList<QStandardItem *> rowData;
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(pathInfo.absoluteFilePath());
        QIcon icon = QIcon::fromTheme("folder");
        if (type.name().startsWith("application/")) {
            icon = QIcon::fromTheme("application-zip");
        }
        auto col1 = new QStandardItem(pathInfo.baseName());
        col1->setData(icon, Qt::DecorationRole);
        col1->setData(pathInfo.absoluteFilePath(), Qt::ToolTipRole);
        col1->setEditable(false);

        auto col2 = new QStandardItem(value);
        col2->setEditable(false);

        m_bookmarksModel->appendRow(rowData << col1 << col2);
    }
    m_bookmarksView = new QTableView();
    m_bookmarksView->setObjectName("bookmarksTableView");
    m_bookmarksView->setModel(m_bookmarksModel);
    m_bookmarksView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bookmarksView->setWordWrap(false);
    m_bookmarksView->setTextElideMode(Qt::ElideRight);
    m_bookmarksView->verticalHeader()->hide();

    auto tableHeader = m_bookmarksView->horizontalHeader();
    tableHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    tableHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    connect(m_bookmarksView, &QTableView::doubleClicked, this, [ = ](const QModelIndex &index) {
        // get path from index
        QModelIndex pathIndex = m_bookmarksModel->index(index.row(), 0);
        QModelIndex pageNumberIndex = m_bookmarksModel->index(index.row(), 1);
        QString path = m_bookmarksModel->data(pathIndex, Qt::ToolTipRole).toString();
//        m_startPageNumber = model->data(pageNumberIndex, Qt::DisplayRole).toInt();
        m_currentManga = path;
        loadImages(path);
    });

    auto deleteButton = new QPushButton();
    deleteButton->setText(i18n("Delete Selected Bookmarks"));
    connect(deleteButton, &QPushButton::clicked, this, [ = ]() {
        deleteBookmarks(m_bookmarksView, "Bookmarks");
    });

    bookmarksLayout->addWidget(m_bookmarksView);
    bookmarksLayout->addWidget(deleteButton);
    bookmarksWidget->setLayout(bookmarksLayout);
    dockWidget->setWidget(bookmarksWidget);
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
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

void MainWindow::loadImages(QString path, bool recursive)
{
    const QFileInfo fileInfo(path);
    QString mangaPath = fileInfo.absoluteFilePath();
    if (fileInfo.isFile()) {
        // extract files to a temporary location
        // when finished call this function with the temporary location and recursive = true
        extractArchive(fileInfo.absoluteFilePath());
        return;
    }

    m_images.clear();

    // get images from path
    auto it = new QDirIterator(mangaPath, QDir::Files, QDirIterator::NoIteratorFlags);
    if (recursive) {
      it = new QDirIterator(mangaPath, QDir::Files, QDirIterator::Subdirectories);
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
    m_worker->setImages(m_images);
    m_view->reset();
    m_view->setManga(mangaPath);
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

    auto settings = new QAction(this);
    settings->setText(i18n("Settings"));
    settings->setIcon(QIcon::fromTheme("settings"));
    actionCollection()->addAction("settings", settings);
    actionCollection()->setDefaultShortcut(settings, Qt::Key_F12);
    connect(settings, &QAction::triggered,
            this, &MainWindow::openSettings);
}

bool MainWindow::isFullScreen()
{
    return (windowState() == (Qt::WindowFullScreen | Qt::WindowMaximized))
            || (windowState() == Qt::WindowFullScreen);
}

void MainWindow::extractArchive(QString archivePath)
{
    QFileInfo extractionFolder(MangaReaderSettings::extractionFolder());
    QFileInfo archivePathInfo(archivePath);
    if (!extractionFolder.exists() || !extractionFolder.isWritable()) {
        return;
    }
    // delete previous extracted folder
    QFileInfo file(m_tmpFolder);
    if (file.exists() && file.isDir() && file.isWritable()) {
        QDir dir(m_tmpFolder);
        dir.removeRecursively();
    }
    m_tmpFolder = extractionFolder.absoluteFilePath() + "/" + archivePathInfo.baseName().toLatin1();
    QDir dir(m_tmpFolder);
    if (!dir.exists()) {
        dir.mkdir(m_tmpFolder);
    }

    auto extractor = new QArchive::DiskExtractor(this);
    extractor->setArchive(archivePathInfo.absoluteFilePath());
    extractor->setOutputDirectory(m_tmpFolder);
    extractor->start();

    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    connect(extractor, &QArchive::DiskExtractor::finished,
            this, [ = ]() {
                m_progressBar->setVisible(false);
                loadImages(m_tmpFolder, true);
            });

    connect(extractor, &QArchive::DiskExtractor::progress,
            this, [ = ](QString file, int processedFiles, int totalFiles, int percent) {
                statusBar()->showMessage(QString::number(percent) + "% " + file);
                m_progressBar->setValue(percent);
            });
}

void MainWindow::treeViewContextMenu(QPoint point)
{
    QModelIndex index = m_treeView->indexAt(point);
    QFileSystemModel *model = static_cast<QFileSystemModel *>(m_treeView->model());
    QString path = model->filePath(index);
    QFileInfo pathInfo(path);

    auto menu = new QMenu();
    menu->setMinimumWidth(200);

    auto load = new QAction(QIcon::fromTheme("arrow-down"), i18n("Load"));
    m_treeView->addAction(load);

    auto loadRecursive = new QAction(QIcon::fromTheme("arrow-down-double"), i18n("Load recursive"));
    m_treeView->addAction(loadRecursive);

    auto openPath = new QAction(QIcon::fromTheme("unknown"), i18n("Open"));
    m_treeView->addAction(openPath);

    auto openContainingFolder = new QAction(QIcon::fromTheme("folder-open"), i18n("Open containing folder"));
    m_treeView->addAction(openContainingFolder);

    menu->addAction(load);
    menu->addAction(loadRecursive);
    menu->addSeparator();
    menu->addAction(openPath);
    menu->addAction(openContainingFolder);

    connect(load, &QAction::triggered,
            this, [ = ]() {
                // get path from index
                const QFileSystemModel *model = static_cast<const QFileSystemModel *>(index.model());
                QString path = model->filePath(index);
                loadImages(path);
            });
    connect(loadRecursive, &QAction::triggered,
            this, [ = ]() {
                // get path from index
                const QFileSystemModel *model = static_cast<const QFileSystemModel *>(index.model());
                QString path = model->filePath(index);
                loadImages(path, true);
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

void MainWindow::onAddBookmark(int pageNumber)
{
    // get the bookmark from the config file
    m_config->reparseConfiguration();
    KConfigGroup bookmarksGroup = m_config->group("Bookmarks");
    QString bookmark = bookmarksGroup.readEntry(m_currentManga);
    // if the bookmark from the config is the same
    // as the one to be saved return
    if (QString::number(pageNumber) == bookmark) {
        return;
    }

    bookmarksGroup.writeEntry(m_currentManga, QString::number(pageNumber));
    bookmarksGroup.config()->sync();

    // check if there is a bookmark for this manga in the bookmarks tableView
    // if found update the page number
    for (int i = 0; i < m_bookmarksModel->rowCount(); i++) {
        QStandardItem *itemPath = m_bookmarksModel->item(i);
        if (m_currentManga == itemPath->data(Qt::ToolTipRole)) {
//            m_bookmarksModel->removeRow(i);
            QStandardItem *itemNumber = m_bookmarksModel->item(i, 1);
            m_bookmarksView->model()->setData(itemNumber->index(), pageNumber);
            return;
        }
    }

    // determine icon for bookmark (folder or archive)
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(m_currentManga);
    QIcon icon = QIcon::fromTheme("folder");
    if (type.name().startsWith("application/")) {
        icon = QIcon::fromTheme("application-zip");
    }

    // add favorite to favorites tableView
    QList<QStandardItem *> rowData;

    QStandardItem *col1 = new QStandardItem(m_currentManga.split("/").takeLast());
    col1->setData(icon, Qt::DecorationRole);
    col1->setData(m_currentManga, Qt::ToolTipRole);
    col1->setEditable(false);

    QStandardItem *col2 = new QStandardItem(QString::number(pageNumber));
    col2->setEditable(false);

    m_bookmarksModel->appendRow(rowData << col1 << col2);
}

void MainWindow::deleteBookmarks(QTableView *tableView, QString name)
{
    QItemSelection selection(tableView->selectionModel()->selection());
    QVector<int> rows;
    int prev = -1;
    // get the rows to be deleted
    for (const QModelIndex &index : selection.indexes()) {
        int current = index.row();
        if (prev != current) {
            rows.append(index.row());
            prev = current;
        }
    }

    // starting from beginning causes the index of the following items to change
    // removing item at index 0 will cause item at index 1 to become item at index 0
    // starting from the end prevents this
    // removing item at index 3 will cause an index change for indexes bigger than 3
    // but since we go in the opposite direction we don't care about those items
    for (int i = rows.count() - 1; i >= 0; i--) {
        // first delete from config file
        // deleting from table view first causes problems with index change
        QModelIndex cell_0 = tableView->model()->index(rows.at(i), 0);
        QModelIndex cell_1 = tableView->model()->index(rows.at(i), 1);
        QString key   = tableView->model()->data(cell_0, Qt::ToolTipRole).toString();
        QString value = tableView->model()->data(cell_1).toString();
        m_config->reparseConfiguration();
        KConfigGroup bookmarksGroup = m_config->group("Bookmarks");
        QString bookmarks = bookmarksGroup.readEntry(key);
        bookmarksGroup.deleteEntry(key);
        bookmarksGroup.config()->sync();
        // delete from table view
        tableView->model()->removeRow(rows.at(i));
    }
}

void MainWindow::openSettings()
{
    if ( KConfigDialog::showDialog( "settings" ) ) {
        return;
    }
    auto settingsWidget = new SettingsWidget(this);
    auto dialog = new KConfigDialog(
             this, "settings", MangaReaderSettings::self() );
    dialog->setFaceType(KPageDialog::Plain);
    dialog->addPage(settingsWidget, i18n("Settings"));
    dialog->show();
    connect(dialog, &KConfigDialog::settingsChanged,
            m_view, &View::onSettingsChanged);
    connect(settingsWidget->selectExtractionFolder, &QPushButton::clicked, this, [ = ]() {
        QString path = QFileDialog::getExistingDirectory(
                    this,
                    i18n("Select extraction folder"),
                    MangaReaderSettings::extractionFolder());
        if (path.isEmpty()) {
            return;
        }

        settingsWidget->kcfg_ExtractionFolder->setText(path);
    });
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

