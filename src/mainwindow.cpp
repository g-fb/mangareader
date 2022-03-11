/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "_debug.h"
#include "extractor.h"
#include "mainwindow.h"
#include "settings.h"
#include "settingswindow.h"
#include "startupwidget.h"
#include "view.h"
#include "worker.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KColorSchemeManager>
#include <KConfigDialog>
#include <KConfigGroup>
#include <KHamburgerMenu>
#include <KLocalizedString>
#include <KToolBar>
#include <kconfigwidgets_version.h>

#include <QtWidgets>
#include <QThread>
#include <QProcess>
#include <KFileItem>
#include <KIO/JobUiDelegate>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KIO/RenameFileDialog>

#include <QArchive>

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow{ parent }
    , m_view{ new View(this) }
    , m_treeDock{ new QDockWidget() }
    , m_treeView{ new QTreeView() }
    , m_treeModel{ new QFileSystemModel() }
    , m_bookmarksDock{ new QDockWidget() }
    , m_bookmarksView{ new QTableView() }
    , m_bookmarksModel{ new QStandardItemModel(0, 2, this) }
{
    setAcceptDrops(true);
    init();
    setupActions();
    setupGUI(QSize(1280, 720), ToolBar | Keys | Save | Create, "mangareaderui.rc");

    if (MangaReaderSettings::mainToolBarVisible())
        showToolBars();

    if (MangaReaderSettings::menuBarVisible())
        menuBar()->show();

    if (m_treeDock->property("isEmpty").toBool()) {
        m_treeDock->setVisible(false);
    } else {
        m_treeDock->setVisible(true);
    }

    if (m_bookmarksDock->property("isEmpty").toBool()) {
        m_bookmarksDock->setVisible(false);
    } else {
        m_bookmarksDock->setVisible(true);
    }

    // setup toolbar
    toolBar("mainToolBar")->setFloatable(false);
    toolBarMenuAction()->setEnabled(false);
    toolBarMenuAction()->setVisible(false);
    connect(toolBar("mainToolBar"), &QToolBar::visibilityChanged,
            this, &MainWindow::setToolBarVisible);
}

MainWindow::~MainWindow()
{
    m_thread->quit();
    m_thread->wait();
}

void MainWindow::init()
{
    m_config = KSharedConfig::openConfig("mangareader/mangareader.conf");

    // ==================================================
    // setup extractor
    // ==================================================
    m_extractor = new Extractor(this);

    connect(m_extractor, &Extractor::started, this, [=]() {
        m_progressBar->setVisible(true);
    });
    connect(m_extractor, &Extractor::progress, this, [=](int p) {
        m_progressBar->setValue(p);
    });
    connect(m_extractor, &Extractor::finished, this, [=]() {
        m_progressBar->setVisible(false);
        loadImages(m_extractor->extractionFolder(), true);
    });
    connect(m_extractor, &Extractor::error, this, [=](const QString &error) {
        showError(error);
    });
    connect(m_extractor, &Extractor::unrarNotFound, this, [=]() {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(m_extractor->unrarNotFoundMessage());
        msgBox.setStandardButtons(QMessageBox::Close);
        msgBox.setEscapeButton(QMessageBox::Close);
        auto btn = msgBox.addButton("Open Settings", QMessageBox::HelpRole);
        connect(btn, &QPushButton::clicked, this, &MainWindow::openSettings);
        msgBox.exec();
    });

    // ==================================================
    // setup central widget
    // ==================================================
    auto mainWidget = new QWidget(this);
    auto centralWidgetLayout = new QVBoxLayout(mainWidget);
    centralWidgetLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(mainWidget);

    m_startUpWidget = new StartUpWidget(this);
    connect(m_startUpWidget, &StartUpWidget::addMangaFolderClicked,
            this, [=]() {actionCollection()->action("addMangaFolder")->trigger();});
    connect(m_startUpWidget, &StartUpWidget::openMangaFolderClicked,
            this, &MainWindow::openMangaFolder);
    connect(m_startUpWidget, &StartUpWidget::openMangaArchiveClicked,
            this, &MainWindow::openMangaArchive);
    centralWidgetLayout->addWidget(m_startUpWidget);

    // ==================================================
    // setup progress bar
    // ==================================================
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setVisible(false);
    centralWidgetLayout->addWidget(m_progressBar);

    // ==================================================
    // setup view
    // ==================================================
    m_view->setVisible(false);
    connect(m_view, &View::addBookmark,
            this, &MainWindow::onAddBookmark);
    connect(m_view, &View::mouseMoved,
            this, &MainWindow::onMouseMoved);
    connect(m_view, &View::addBookmark, this, [=]() {
        if (!isFullScreen())
            m_bookmarksDock->setVisible(true);
        m_bookmarksDock->setProperty("isEmpty", false);
    });
    connect(m_view, &View::fileDropped, this, [=] (const QString &file) {
        loadImages(file, true);
    });
    centralWidgetLayout->addWidget(m_view);

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
    // setup KHamburgerMenu
    // ==================================================
    m_hamburgerMenu = KStandardAction::hamburgerMenu(nullptr, nullptr, actionCollection());
    toolBar()->addAction(m_hamburgerMenu);
    m_hamburgerMenu->hideActionsOf(toolBar());
    m_hamburgerMenu->setMenuBar(menuBar());

    // ==================================================
    // setup dock widgets
    // ==================================================
    setupMangaTreeDockWidget();
    m_treeDock->setVisible(false);
    addDockWidget(Qt::LeftDockWidgetArea, m_treeDock);

    setupBookmarksDockWidget();
    addDockWidget(Qt::LeftDockWidgetArea, m_bookmarksDock);
}

void MainWindow::setupMangaTreeDockWidget()
{
    KConfigGroup rootGroup = m_config->group("");
    QString mangaFolder = rootGroup.readEntry("Manga Folder");

    if (mangaFolder.isEmpty()) {
        return;
    }

    auto treeDockWidget = new QWidget(this);
    auto treeDockLayout = new QVBoxLayout(treeDockWidget);

    m_mangaFoldersMenu = new QMenu(this);

    m_selectMangaLibraryButton = new QPushButton(treeDockWidget);
    m_selectMangaLibraryButton->setText(i18n("Select Manga Library"));
    m_selectMangaLibraryButton->setMenu(m_mangaFoldersMenu);
    populateMangaFoldersMenu();

    treeDockLayout->addWidget(m_selectMangaLibraryButton);

    m_treeDock->setObjectName("treeDockWidget");
    m_treeDock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    m_treeDock->setProperty("h", 0);
    m_treeDock->setProperty("isEmpty", true);

    m_treeModel->setObjectName("mangaTree");
    m_treeModel->setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    m_treeModel->setNameFilters(QStringList() << "*.zip" << "*.7z" << "*.cbz" << "*.cbt" << "*.cbr" << "*.cb7" << "*.rar");
    m_treeModel->setNameFilterDisables(false);

    m_treeView->setModel(m_treeModel);
    m_treeView->setColumnHidden(1, true);
    m_treeView->setColumnHidden(2, true);
    m_treeView->setColumnHidden(3, true);
    m_treeView->header()->hide();
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_treeModel->setRootPath(mangaFolder);
    m_treeView->setRootIndex(m_treeModel->index(mangaFolder));
    m_treeDock->setProperty("isEmpty", false);
    m_treeDock->setWindowTitle(mangaFolder);

    auto action = new QAction();
    action->setShortcuts({Qt::Key_Enter, Qt::Key_Return});
    action->setShortcutContext(Qt::WidgetShortcut);
    connect(action, &QAction::triggered, this, [=]() {
        QString path = m_treeModel->filePath(m_treeView->selectionModel()->currentIndex());
        loadImages(path);
    });
    m_treeView->addAction(action);
    connect(m_treeView, &QTreeView::doubleClicked, this, [=](const QModelIndex &index) {
        // get path from index
        QString path = m_treeModel->filePath(index);
        loadImages(path);
    });
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::treeViewContextMenu);

    treeDockLayout->addWidget(m_treeView);
    m_treeDock->setWidget(treeDockWidget);
}

void MainWindow::setupBookmarksDockWidget()
{
    KConfigGroup bookmarksGroup = m_config->group("Bookmarks");
    if (bookmarksGroup.keyList().isEmpty()) {
        return;
    }

    m_bookmarksDock->setObjectName("bookmarksDockWidget");
    m_bookmarksDock->setWindowTitle(i18n("Bookmarks"));
    m_bookmarksDock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    m_bookmarksDock->setProperty("h", 0);
    m_bookmarksDock->setProperty("isEmpty", true);

    m_bookmarksView->setObjectName("bookmarksTableView");
    m_bookmarksView->setModel(m_bookmarksModel);
    m_bookmarksView->setWordWrap(false);
    m_bookmarksView->setTextElideMode(Qt::ElideRight);
    m_bookmarksView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_bookmarksView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bookmarksView->verticalHeader()->hide();

    m_bookmarksModel->setHorizontalHeaderItem(0, new QStandardItem(i18n("Manga")));
    m_bookmarksModel->setHorizontalHeaderItem(1, new QStandardItem(i18n("Page")));

    auto tableHeader = m_bookmarksView->horizontalHeader();
    tableHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    tableHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    if (bookmarksGroup.keyList().count() > 0) {
        populateBookmarkModel();
        m_bookmarksDock->setProperty("isEmpty", false);
    }

    auto openBookmark = [=](const QModelIndex &index) {
        QModelIndex cellIndex = m_bookmarksModel->index(index.row(), 1);
        m_startPage  = m_bookmarksModel->data(cellIndex, IndexRole).toInt();
        QString path = m_bookmarksModel->data(cellIndex, PathRole).toString();
        QString key  = m_bookmarksModel->data(cellIndex, KeyRole).toString();
        QFileInfo pathInfo(path);
        if (!pathInfo.exists()) {
            showError(i18n("The file or folder does not exist.\n%1", path));
            return;
        }
        if (key.startsWith(RECURSIVE_KEY_PREFIX)) {
            loadImages(path, true);
        } else {
            loadImages(path);
        }
    };

    auto action = new QAction();
    action->setShortcuts({Qt::Key_Enter, Qt::Key_Return});
    action->setShortcutContext(Qt::WidgetShortcut);
    connect(action, &QAction::triggered, this, [=]() {
        if (m_bookmarksView->selectionModel()->selectedRows().count() > 1) {
            return;
        }
        QModelIndex index = m_bookmarksView->selectionModel()->selectedRows().first();
        openBookmark(index);
    });
    m_bookmarksView->addAction(action);

    connect(m_bookmarksModel, &QAbstractItemModel::rowsRemoved, this, [=]() {
        if (m_bookmarksModel->rowCount() == 0) {
            m_bookmarksDock->setVisible(false);
            m_bookmarksDock->setProperty("isEmpty", true);
        }
    });

    connect(m_bookmarksView, &QTableView::doubleClicked, this, [=](const QModelIndex &index) {
        openBookmark(index);
    });

    connect(m_bookmarksView, &QTableView::customContextMenuRequested,
            this, &MainWindow::bookmarksViewContextMenu);

    auto deleteButton = new QPushButton();
    deleteButton->setText(i18n("Delete Selected Bookmarks"));
    connect(deleteButton, &QPushButton::clicked, this, [=]() {
        deleteBookmarks(m_bookmarksView);
    });

    auto bookmarksLayout = new QVBoxLayout();
    auto bookmarksWidget = new QWidget(this);
    bookmarksLayout->addWidget(m_bookmarksView);
    bookmarksLayout->addWidget(deleteButton);
    bookmarksWidget->setLayout(bookmarksLayout);
    m_bookmarksDock->setWidget(bookmarksWidget);
}

void MainWindow::populateBookmarkModel()
{
    KConfigGroup bookmarks = m_config->group("Bookmarks");
    const QStringList keys = bookmarks.keyList();
    for (const QString &key : keys) {
        QString pageIndex = bookmarks.readEntry(key);
        QString pageNumber = QString::number(pageIndex.toInt() + 1);
        QString path = key;
        if (path.startsWith(RECURSIVE_KEY_PREFIX)) {
            path = path.remove(0, RECURSIVE_KEY_PREFIX.length());
        }
        QFileInfo pathInfo(path);
        QList<QStandardItem *> rowData;
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(pathInfo.absoluteFilePath());
        QIcon icon = QIcon::fromTheme("folder");
        if (type.name().startsWith("application/")) {
            icon = QIcon::fromTheme("application-zip");
        }
        QString displayPrefix = (key.startsWith(RECURSIVE_KEY_PREFIX)) ? "(r) " : QString();

        auto col1 = new QStandardItem(pathInfo.fileName().prepend(displayPrefix));
        col1->setData(icon, Qt::DecorationRole);
        col1->setData(pathInfo.absoluteFilePath(), Qt::ToolTipRole);
        col1->setData(key, KeyRole);
        col1->setData(pathInfo.absoluteFilePath(), PathRole);
        col1->setData(key.startsWith(RECURSIVE_KEY_PREFIX), RecursiveRole);
        col1->setEditable(false);

        auto col2 = new QStandardItem(pageNumber);
        col2->setData(key, KeyRole);
        col2->setData(pageIndex, IndexRole);
        col2->setData(pathInfo.absoluteFilePath(), PathRole);
        col2->setData(key.startsWith(RECURSIVE_KEY_PREFIX), RecursiveRole);
        col2->setEditable(false);

        m_bookmarksModel->appendRow(rowData << col1 << col2);
    }
    m_bookmarksDock->setVisible(m_bookmarksModel->rowCount() > 0);
    m_bookmarksDock->setProperty("isEmpty", !(m_bookmarksModel->rowCount() > 0));
}

void MainWindow::openMangaArchive()
{
    QString file = QFileDialog::getOpenFileName(
                this,
                i18n("Open Archive"),
                QDir::homePath(),
                i18n("Archives (*.zip *.rar *.7z *.tar *.cbz *.cbr *.cb7 *.cbt)"));
    if (file.isEmpty()) {
        return;
    }
    loadImages(file, true);
}

void MainWindow::openMangaFolder()
{
    QString path = QFileDialog::getExistingDirectory(this, i18n("Open folder"), QDir::homePath());
    if (path.isEmpty()) {
        return;
    }
    loadImages(path, true);
}

void MainWindow::loadImages(const QString& path, bool recursive, bool updateCurrentPath)
{
    QMimeDatabase db;
    QString mimetype = db.mimeTypeForFile(path).name();
    if (!m_supportedMimeTypes.contains(mimetype)) {
        showError(i18n("Unsuported file type: %1\n"
                       "Only folders and .zip, .cbz, .rar, .cbr, "
                       ".7z, .cb7, .tar, .cbt archives are supported. ", mimetype));
        return;
    }
    m_startUpWidget->setVisible(false);
    m_view->setVisible(true);
    // when opening an archive the files are extracted
    // to a temporary folder which is used to load the images from
    // m_currentPath should not be set to this temporary folder
    // m_currentPath is the file/folder opened by the user
    if (updateCurrentPath)
        m_currentPath = path;
    const QFileInfo currentPathInfo(m_currentPath);
    setWindowTitle(currentPathInfo.fileName());

    m_isLoadedRecursive = recursive;
    const QFileInfo fileInfo(path);
    QString mangaPath = fileInfo.absoluteFilePath();
    if (fileInfo.isFile()) {
        // extract files to a temporary location
        // when finished call this function with the temporary location and recursive = true
        m_extractor->setArchiveFile(fileInfo.absoluteFilePath());
        m_extractor->extractArchive();
        return;
    }

    m_images.clear();

    // get images from path
    auto it = new QDirIterator(mangaPath, QDir::Files, QDirIterator::NoIteratorFlags);
    if (recursive) {
        delete it;
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
    m_view->reset();
    m_view->setStartPage(m_startPage);
    m_view->setManga(mangaPath);
    m_view->setImages(m_images);
    m_view->loadImages();
    m_startPage = 0;
}

void MainWindow::setupActions()
{
    auto *schemes = new KColorSchemeManager(this);
#if KCONFIGWIDGETS_VERSION >= QT_VERSION_CHECK(5, 89, 0)
    schemes->setAutosaveChanges(false);
#endif

    KConfigGroup cg(m_config, "UiSettings");
    auto schemeName = cg.readEntry("ColorScheme", QString());
    schemes->activateScheme(schemes->indexForScheme(schemeName));

    auto colorSchemeAction = schemes->createSchemeSelectionMenu(schemeName, this);
    colorSchemeAction->setPopupMode(QToolButton::InstantPopup);
    actionCollection()->addAction("colorSchemeChooser", colorSchemeAction);

    connect(colorSchemeAction->menu(), &QMenu::triggered, this, [=](QAction *triggeredAction) {
        KConfigGroup cg(m_config, "UiSettings");
        cg.writeEntry("ColorScheme", KLocalizedString::removeAcceleratorMarker(triggeredAction->text()));
        cg.sync();
    });

    auto focusMangaTree = new QAction();
    focusMangaTree->setText(i18n("Focus Manga Tree"));
    actionCollection()->addAction("focusTree", focusMangaTree);
    actionCollection()->setDefaultShortcuts(focusMangaTree, {Qt::Key_N, Qt::CTRL + Qt::Key_N});
    connect(focusMangaTree, &QAction::triggered, this, [=]() {
        m_treeView->setFocus();
    });

    auto focusBookmarksTable = new QAction();
    focusBookmarksTable->setText(i18n("Focus Manga Bookmarks"));
    actionCollection()->addAction("focusBookmarksTable", focusBookmarksTable);
    actionCollection()->setDefaultShortcuts(focusBookmarksTable, {Qt::Key_B, Qt::CTRL + Qt::Key_B});
    connect(focusBookmarksTable, &QAction::triggered, this, [=]() {
        m_bookmarksView->setFocus();
    });

    auto focusView = new QAction();
    focusView->setText(i18n("Focus Manga Viewer"));
    actionCollection()->addAction("focusView", focusView);
    actionCollection()->setDefaultShortcuts(focusView, {Qt::Key_V, Qt::CTRL + Qt::Key_V});
    connect(focusView, &QAction::triggered, this, [=]() {
        m_view->setFocus();
    });

    auto addMangaFolderAction = new QAction(this);
    addMangaFolderAction->setText(i18n("&Add Manga Library Folder"));
    addMangaFolderAction->setIcon(QIcon::fromTheme("folder-add"));
    actionCollection()->addAction("addMangaFolder", addMangaFolderAction);
    actionCollection()->setDefaultShortcut(addMangaFolderAction, Qt::CTRL + Qt::Key_A);
    connect(addMangaFolderAction, &QAction::triggered, this, [=]() {
        openSettings();
        m_settingsWindow->addMangaFolderButton()->click();
    });

    auto openMangaFolder = new QAction(this);
    openMangaFolder->setText(i18n("&Open Manga Folder"));
    openMangaFolder->setIcon(QIcon::fromTheme("folder-open"));
    actionCollection()->addAction("openMangaFolder", openMangaFolder);
    actionCollection()->setDefaultShortcut(openMangaFolder, Qt::CTRL + Qt::Key_O);
    connect(openMangaFolder, &QAction::triggered,
            this, &MainWindow::openMangaFolder);

    auto openMangaArchive = new QAction(this);
    openMangaArchive->setText(i18n("&Open Manga Archive"));
    openMangaArchive->setIcon(QIcon::fromTheme("application-zip"));
    actionCollection()->addAction("openMangaArchive", openMangaArchive);
    actionCollection()->setDefaultShortcut(openMangaArchive, Qt::CTRL + Qt::SHIFT + Qt::Key_O);
    connect(openMangaArchive, &QAction::triggered,
            this, &MainWindow::openMangaArchive);

    auto goToLayout = new QHBoxLayout();
    goToLayout->setSpacing(0);
    auto goTowidget = new QWidget();
    goTowidget->setLayout(goToLayout);
    auto goToSpinBox = new QSpinBox();
    auto action = new QAction();
    action->setShortcuts({Qt::Key_Enter, Qt::Key_Return});
    action->setShortcutContext(Qt::WidgetShortcut);
    connect(action, &QAction::triggered, this, [=]() {
        m_view->goToPage(goToSpinBox->value() - 1);
    });
    goToSpinBox->addAction(action);
    connect(m_view, &View::imagesLoaded, this, [=]() {
        goToSpinBox->setRange(1, m_view->imageCount());
    });
    auto goToButton = new QToolButton();
    goToButton->setText(i18n("Go to page"));
    connect(goToButton, &QToolButton::clicked, this, [=]() {
        m_view->goToPage(goToSpinBox->value() - 1);
    });
    goToLayout->addWidget(goToButton);
    goToLayout->addWidget(goToSpinBox);

    auto goToAction = new QWidgetAction(this);
    goToAction->setDefaultWidget(goTowidget);
    goToAction->setText(i18n("Go To Page"));
    actionCollection()->addAction("goToPage", goToAction);

    auto resetZoom = new QAction();
    resetZoom->setText(i18n("Zoom Reset"));
    resetZoom->setIcon(QIcon::fromTheme("view-zoom-original-symbolic"));
    connect(resetZoom, &QAction::triggered,
            m_view, &View::zoomReset);
    actionCollection()->setDefaultShortcut(resetZoom, Qt::CTRL + Qt::Key_0);
    actionCollection()->addAction("resetZoom", resetZoom);

    auto fitToHeightAction = new QAction();
    fitToHeightAction->setText(i18n("Fit height"));
    fitToHeightAction->setToolTip(i18n("Image height is resized to view's height.\n"
                                       "If the image height is smaller than the view height\n"
                                       "the image will only be upscaled if the scale up setting is enabled."));
    QIcon fallbackIcon = QIcon::fromTheme("zoom-fit-height");
    fitToHeightAction->setIcon(QIcon::fromTheme("fitheight", fallbackIcon));
    fitToHeightAction->setCheckable(true);
    fitToHeightAction->setChecked(MangaReaderSettings::fitHeight());
    connect(fitToHeightAction, &QAction::triggered,
            this, &MainWindow::toggleFitHeight);
    actionCollection()->addAction("fitToHeightAction", fitToHeightAction);

    auto fitToWidthAction = new QAction();
    fitToWidthAction->setText(i18n("Fit width"));
    fitToWidthAction->setToolTip(i18n("Image width is resized to max width setting.\n"
                                      "If the image width is smaller than the max width\n"
                                      "the image will only be upscaled if the scale up setting is enabled.\n"
                                      "Set max width to 9999 to have the image fill all available space."));
    fallbackIcon = QIcon::fromTheme("zoom-fit-width");
    fitToWidthAction->setIcon(QIcon::fromTheme("fitwidth", fallbackIcon));
    fitToWidthAction->setCheckable(true);
    fitToWidthAction->setChecked(MangaReaderSettings::fitWidth());
    connect(fitToWidthAction, &QAction::triggered,
            this, &MainWindow::toggleFitWidth);
    actionCollection()->addAction("fitToWidthAction", fitToWidthAction);

    KStandardAction::zoomIn(m_view, &View::zoomIn, actionCollection());
    KStandardAction::zoomOut(m_view, &View::zoomOut, actionCollection());
    KStandardAction::showMenubar(this, &MainWindow::toggleMenubar, actionCollection());
    KStandardAction::preferences(this, &MainWindow::openSettings, actionCollection());
    KStandardAction::donate(this, [=]() {
        QDesktopServices::openUrl(QUrl("https://github.com/sponsors/g-fb"));
    }, actionCollection());
    KStandardAction::quit(QApplication::instance(), &QApplication::quit, actionCollection());

    QAction *toggleFullScreenAction = KStandardAction::fullScreen(this, [=]() {toggleFullScreen();}, this, actionCollection());
    connect(m_view, &View::doubleClicked,
            toggleFullScreenAction, &QAction::trigger);
}

void MainWindow::toggleMenubar()
{
    if (menuBar()->isVisible()) {
        menuBar()->hide();
        MangaReaderSettings::setMenuBarVisible(false);
    } else {
        menuBar()->show();
        MangaReaderSettings::setMenuBarVisible(true);
    }
    MangaReaderSettings::self()->save();
}

void MainWindow::setToolBarVisible(bool visible)
{
    if (isFullScreen())
        return;

    QToolBar *tb = toolBar("mainToolBar");
    tb->setVisible(visible);
    MangaReaderSettings::setMainToolBarVisible(visible);
    MangaReaderSettings::self()->save();
}

void MainWindow::toggleFitHeight()
{
    MangaReaderSettings::setFitHeight(!MangaReaderSettings::fitHeight());
    MangaReaderSettings::self()->save();
    m_view->zoomReset();
}

void MainWindow::toggleFitWidth()
{
    MangaReaderSettings::setFitWidth(!MangaReaderSettings::fitWidth());
    MangaReaderSettings::self()->save();
    m_view->zoomReset();
}

auto MainWindow::populateMangaFoldersMenu() -> QMenu *
{
    m_mangaFoldersMenu->clear();
    const QStringList mangaFolders = MangaReaderSettings::mangaFolders();
    for (const QString &mangaFolder : mangaFolders) {
        QAction *action = m_mangaFoldersMenu->addAction(mangaFolder);
        connect(action, &QAction::triggered, this, [=]() {
            m_treeModel->setRootPath(mangaFolder);
            m_treeView->setRootIndex(m_treeModel->index(mangaFolder));
            m_treeDock->setWindowTitle(mangaFolder);
            m_config->group("").writeEntry("Manga Folder", mangaFolder);
            m_config->sync();
        });
    }
    m_selectMangaLibraryButton->setVisible(false);
    // no point in showing if there's only one option
    if (MangaReaderSettings::mangaFolders().count() > 1) {
        m_selectMangaLibraryButton->setVisible(true);
    }
    return m_mangaFoldersMenu;
}

void MainWindow::showError(const QString& error)
{
    QMessageBox errorWindow;
    errorWindow.setWindowTitle(i18n("Error"));
    errorWindow.setText(error);

    auto horizontalSpacer = new QSpacerItem(500, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto layout = qobject_cast<QGridLayout*>(errorWindow.layout());
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    errorWindow.exec();
}

void MainWindow::treeViewContextMenu(QPoint point)
{
    QModelIndex index = m_treeView->indexAt(point);
    QString path = m_treeModel->filePath(index);
    QFileInfo pathInfo(path);

    auto menu = new QMenu();
    menu->setMinimumWidth(200);

    auto load = new QAction(QIcon::fromTheme("arrow-down"), i18n("Load"));
    m_treeView->addAction(load);

    auto loadRecursive = new QAction(QIcon::fromTheme("arrow-down-double"), i18n("Load recursive"));
    m_treeView->addAction(loadRecursive);

    auto rename = new QAction(QIcon::fromTheme("edit-rename"), i18n("Rename"));
    m_treeView->addAction(rename);

    auto openPath = new QAction(QIcon::fromTheme("unknown"), i18n("Open"));
    m_treeView->addAction(openPath);

    auto openContainingFolder = new QAction(QIcon::fromTheme("folder-open"), i18n("Open containing folder"));
    m_treeView->addAction(openContainingFolder);

    menu->addAction(load);
    menu->addAction(loadRecursive);
    menu->addAction(rename);
    menu->addSeparator();
    menu->addAction(openPath);
    menu->addAction(openContainingFolder);

    connect(load, &QAction::triggered, this, [=]() {
        loadImages(path);
    });
    connect(loadRecursive, &QAction::triggered, this, [=]() {
        loadImages(path, true);
    });

    connect(rename, &QAction::triggered, this, [=]() {
        QUrl url(path);
        url.setScheme("file");
        KFileItem item(url);
        auto renameDialog = new KIO::RenameFileDialog(KFileItemList({item}), nullptr);
        renameDialog->open();
        connect(renameDialog, &KIO::RenameFileDialog::renamingFinished, this, [=](const QList<QUrl> &urls) {
            qDebug() << 22133;
            auto newName = urls.first().toLocalFile();
            if (m_currentPath == path && !pathInfo.isDir()) {
                m_currentPath = newName;
            }
            // Delete bookmarks for old name
            // and create new bookmarks for the new name
            // keys for normal and recursive bookmarks
            const QString &key = path;
            const QString &recursiveKey = RECURSIVE_KEY_PREFIX + path;

            // get the values for both bookmarks
            KConfigGroup bookmarksGroup = m_config->group("Bookmarks");
            QString bookmark = bookmarksGroup.readEntry(key);
            QString recursiveBookmark = bookmarksGroup.readEntry(recursiveKey);

            // delete and create new bookmarks
            if (!bookmark.isEmpty()) {
                bookmarksGroup.deleteEntry(key);
                bookmarksGroup.writeEntry(newName, bookmark);
            }
            if (!recursiveBookmark.isEmpty()) {
                bookmarksGroup.deleteEntry(recursiveKey);
                bookmarksGroup.writeEntry(RECURSIVE_KEY_PREFIX + newName, recursiveBookmark);
            }
            bookmarksGroup.config()->sync();

            m_bookmarksModel->removeRows(0, m_bookmarksModel->rowCount());
            populateBookmarkModel();
        });
    });

    connect(openPath, &QAction::triggered, this, [=]() {
        QUrl url(path);
        url.setScheme(QStringLiteral("file"));
        auto job = new KIO::OpenUrlJob(url);
        job->setUiDelegate(new KIO::JobUiDelegate());
        job->start();
    });

    connect(openContainingFolder, &QAction::triggered, this, [=]() {
        QUrl url(path);
        url.setScheme(QStringLiteral("file"));
        KIO::highlightInFileManager({url});
    });
    menu->exec(QCursor::pos());
}

void MainWindow::bookmarksViewContextMenu(QPoint point)
{
    QModelIndex index = m_bookmarksView->indexAt(point);
    QString path = m_bookmarksModel->data(index, PathRole).toString();

    auto contextMenu = new QMenu();
    auto action = new QAction(QIcon::fromTheme("unknown"), i18n("Open"));
    contextMenu->addAction(action);
    connect(action, &QAction::triggered, this, [=]() {
        QUrl url(path);
        url.setScheme(QStringLiteral("file"));
        auto job = new KIO::OpenUrlJob(url);
        job->setUiDelegate(new KIO::JobUiDelegate());
        job->start();
    });

    action = new QAction(QIcon::fromTheme("folder-open"), i18n("Open containing folder"));
    contextMenu->addAction(action);

    connect(action, &QAction::triggered, this, [=]() {
        QUrl url(path);
        url.setScheme(QStringLiteral("file"));
        KIO::highlightInFileManager({url});
    });

    contextMenu->addSeparator();

    action = new QAction(QIcon::fromTheme("delete"), i18n("Delete Selected Bookmark(s)"));
    contextMenu->addAction(action);
    connect(action, &QAction::triggered, this, [=]() {
        deleteBookmarks(m_bookmarksView);
    });
    contextMenu->popup(QCursor::pos());
}

void MainWindow::hideDockWidgets(Qt::DockWidgetAreas area)
{
    const QList<QDockWidget *> dockWidgets = findChildren<QDockWidget *>();
    for (QDockWidget *dockWidget : dockWidgets) {
        if ((dockWidgetArea(dockWidget) == area || area == Qt::AllDockWidgetAreas)
                && !dockWidget->isFloating()) {
            bool isEmpty = dockWidget->property("isEmpty").toBool();
            if (!isEmpty) {
                dockWidget->setVisible(false);
            }
        }
    }
}

void MainWindow::showDockWidgets(Qt::DockWidgetAreas area)
{
    QList<QDockWidget *> dockWidgets = findChildren<QDockWidget *>();
    for (int i = dockWidgets.size(); i > 0; i--) {
        if ((dockWidgetArea(dockWidgets.at(i-1)) == area || area == Qt::AllDockWidgetAreas)
                && !dockWidgets.at(i-1)->isFloating()) {
            QDockWidget *dockWidget = dockWidgets.at(i-1);
            bool isEmpty = dockWidget->property("isEmpty").toBool();
            if (!isEmpty) {
                dockWidget->setVisible(true);
            }
        }
    }
}

void MainWindow::hideToolBars(Qt::ToolBarAreas area)
{
    const QList<QToolBar *> toolBars = findChildren<QToolBar *>();
    for (QToolBar *toolBar : toolBars) {
        if ((toolBarArea(toolBar) == area || area == Qt::AllToolBarAreas) && !toolBar->isFloating()) {
            toolBar->hide();
        }
    }
}

void MainWindow::showToolBars(Qt::ToolBarAreas area)
{
    const QList<QToolBar *> toolBars = findChildren<QToolBar *>();
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

void MainWindow::onAddBookmark(int pageIndex)
{
    int pageNumber = pageIndex + 1;
    QFileInfo mangaInfo(m_currentPath);
    QString keyPrefix = (m_isLoadedRecursive) ? RECURSIVE_KEY_PREFIX : QString();
    QString key = mangaInfo.absoluteFilePath().prepend(keyPrefix);
    // get the bookmark from the config file
    m_config->reparseConfiguration();
    KConfigGroup bookmarksGroup = m_config->group("Bookmarks");
    QString bookmark = bookmarksGroup.readEntry(key);
    // if the bookmark from the config is the same
    // as the one to be saved return
    if (QString::number(pageIndex) == bookmark) {
        return;
    }

    bookmarksGroup.writeEntry(key, QString::number(pageIndex));
    bookmarksGroup.config()->sync();

    // check if there is a bookmark for this manga in the bookmarks tableView
    // if found update the page number
    for (int i = 0; i < m_bookmarksModel->rowCount(); i++) {
        QStandardItem *item = m_bookmarksModel->item(i, 1);
        if (key == item->data(KeyRole).toString()) {
            m_bookmarksView->model()->setData(item->index(), pageIndex, IndexRole);
            m_bookmarksView->model()->setData(item->index(), pageNumber);

            return;
        }
    }

    // determine icon for bookmark (folder or archive)
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(mangaInfo.absoluteFilePath());
    QIcon icon = QIcon::fromTheme("folder");
    if (type.name().startsWith("application/")) {
        icon = QIcon::fromTheme("application-zip");
    }

    // add bookmark to tableView
    QList<QStandardItem *> rowData;
    QString displayPrefix = (m_isLoadedRecursive) ? "(r) " : "";

    auto *col1 = new QStandardItem(mangaInfo.fileName().prepend(displayPrefix));
    col1->setData(icon, Qt::DecorationRole);
    col1->setData(mangaInfo.absoluteFilePath(), Qt::ToolTipRole);
    col1->setData(key, KeyRole);
    col1->setData(mangaInfo.absoluteFilePath(), PathRole);
    col1->setData(m_isLoadedRecursive, RecursiveRole);
    col1->setEditable(false);

    auto *col2 = new QStandardItem(QString::number(pageNumber));
    col2->setData(pageIndex, IndexRole);
    col2->setData(key, KeyRole);
    col2->setData(mangaInfo.absoluteFilePath(), PathRole);
    col2->setData(m_isLoadedRecursive, RecursiveRole);
    col2->setEditable(false);

    m_bookmarksModel->appendRow(rowData << col1 << col2);
}

void MainWindow::deleteBookmarks(QTableView *tableView)
{
    QItemSelection selection(tableView->selectionModel()->selection());
    QVector<int> rows;
    int prev = -1;
    // get the rows to be deleted
    const QModelIndexList indexes = selection.indexes();
    for (const QModelIndex &index : indexes) {
        int current = index.row();
        if (prev != current) {
            rows.append(index.row());
            prev = current;
        }
    }

    std::sort(rows.begin(), rows.end());

    // starting from beginning causes the index of the following items to change
    // removing item at index 0 will cause item at index 1 to become item at index 0
    // starting from the end prevents this
    // removing item at index 3 will cause an index change for indexes bigger than 3
    // but since we go in the opposite direction we don't care about those items
    for (int i = rows.count() - 1; i >= 0; i--) {
        // first delete from config file
        // deleting from table view first causes problems with index change
        QModelIndex cell_0 = tableView->model()->index(rows.at(i), 0);
        bool isRecursive = tableView->model()->data(cell_0, RecursiveRole).toBool();
        QString key   = tableView->model()->data(cell_0, Qt::ToolTipRole).toString();
        if (isRecursive) {
            key = key.prepend(RECURSIVE_KEY_PREFIX);
        }
        m_config->reparseConfiguration();
        KConfigGroup bookmarksGroup = m_config->group("Bookmarks");
        bookmarksGroup.deleteEntry(key);
        bookmarksGroup.config()->sync();
        tableView->model()->removeRow(rows.at(i));
    }
}

void MainWindow::openSettings()
{
    m_settingsWindow = new SettingsWindow(this, MangaReaderSettings::self());
    m_settingsWindow->show();

    connect(m_settingsWindow, &SettingsWindow::settingsChanged, m_view, &View::refreshPages);
    connect(m_settingsWindow, &SettingsWindow::settingsChanged, this, [=]() {
        QString mangaFolder = m_config->group("").readEntry("Manga Folder");
        if (MangaReaderSettings::mangaFolders().count() > 0) {
            // if current manga folder is empty or not in the manga folders list
            // set first folder in manga folders list as current
            if (mangaFolder.isEmpty() || !MangaReaderSettings::mangaFolders().contains(mangaFolder)) {
                QString firstMangaFolder = MangaReaderSettings::mangaFolders().at(0);
                m_treeModel->setRootPath(firstMangaFolder);
                m_treeView->setRootIndex(m_treeModel->index(firstMangaFolder));
                m_treeDock->setVisible(true);
                m_treeDock->setProperty("isEmpty", false);
                m_config->group("").writeEntry("Manga Folder", firstMangaFolder);
                m_config->sync();
            }
        } else {
            m_treeDock->setVisible(false);
            m_treeDock->setProperty("isEmpty", true);
            m_config->group("").deleteEntry("Manga Folder");
        }
        populateMangaFoldersMenu();
    });
    m_settingsWindow->show();
}


void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        if (MangaReaderSettings::mainToolBarVisible())
            showToolBars();
        if (MangaReaderSettings::menuBarVisible())
            menuBar()->show();

        showDockWidgets();
        int treeDockHeight = m_treeDock->property("h").toInt();
        int bookmarksDockHeight = m_bookmarksDock->property("h").toInt();
        resizeDocks({m_treeDock, m_bookmarksDock}, {treeDockHeight, bookmarksDockHeight}, Qt::Vertical);
    } else {
        setWindowState(windowState() | Qt::WindowFullScreen);
        hideToolBars();
        menuBar()->hide();

        hideDockWidgets();
        m_treeDock->setProperty("h", m_treeDock->height());
        m_bookmarksDock->setProperty("h", m_bookmarksDock->height());
    }
}

auto MainWindow::isFullScreen() -> bool
{
    return (windowState() == (Qt::WindowFullScreen | Qt::WindowMaximized))
            || (windowState() == Qt::WindowFullScreen);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QString fileName = e->mimeData()->urls().first().toLocalFile();
    loadImages(fileName);
}
