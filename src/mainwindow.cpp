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
#include <QProcess>

#include <QArchive>

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
    , m_treeView(new QTreeView())
    , m_treeModel(new QFileSystemModel())
{
    // setup central widget
    auto centralWidget = new QWidget(this);
    auto centralWidgetLayout = new QVBoxLayout(centralWidget);
    centralWidgetLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centralWidget);

    m_config = KSharedConfig::openConfig("mangareader/mangareader.conf");

    init();
    setupActions();
    setupGUI(QSize(1280, 720), Default, "mangareaderui.rc");
    if (MangaReaderSettings::mangaFolders().count() < 2) {
        m_selectMangaFolder->setVisible(false);
    }

    connect(m_view, &View::mouseMoved,
            this, &MainWindow::onMouseMoved);

    showToolBars();
    showDockWidgets();
    menuBar()->show();
    statusBar()->hide();
    connect(qApp, &QApplication::aboutToQuit, this, [ = ]() {
        this->~MainWindow();
    });

    // rename dialog
    m_renameDialog = new QDialog(this, Qt::Dialog);
    m_renameDialog->setWindowTitle(i18n("Rename"));
    m_renameDialog->setMinimumWidth(600);

    auto vLayout = new QVBoxLayout();
    auto hLayout = new QHBoxLayout();
    hLayout->setObjectName("hLayout");
    hLayout->setMargin(0);
    auto widget = new QWidget();
    widget->setLayout(hLayout);
    auto label = new QLabel(i18n("New name:"));
    auto infoLabel = new QLabel();
    infoLabel->setObjectName("infoLabel");
    auto renameLineEdit = new QLineEdit();
    renameLineEdit->setObjectName("renameLineEdit");
    auto spacer = new QSpacerItem(500, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto okButton = new QPushButton(i18n("OK"));
    okButton->setObjectName("okButton");
    auto cancelButton = new QPushButton(i18n("Cancel"));
    cancelButton->setObjectName("cancelButton");

    vLayout->addWidget(label);
    vLayout->addWidget(renameLineEdit);
    vLayout->addWidget(infoLabel);
    vLayout->addWidget(widget);

    hLayout->addItem(spacer);
    hLayout->addWidget(okButton);
    hLayout->addWidget(cancelButton);

    m_renameDialog->setLayout(vLayout);

    connect(okButton, &QPushButton::clicked,
            this, &MainWindow::renameFile);
    connect(cancelButton, &QPushButton::clicked,
            m_renameDialog, &QDialog::reject);
}

MainWindow::~MainWindow()
{
    m_thread->quit();
    m_thread->wait();

    QFileInfo file(m_tmpFolder);
    if (file.exists() && file.isDir() && file.isWritable()) {
        QDir dir(m_tmpFolder);
        dir.removeRecursively();
    }
}

Qt::ToolBarArea MainWindow::mainToolBarArea()
{
    return m_mainToolBarArea;
}

void MainWindow::init()
{
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
    // ==================================================

    // ==================================================
    // tree dock widget
    // ==================================================
    KConfigGroup rootGroup = m_config->group("");
    QFileInfo mangaDirInfo(rootGroup.readEntry("Manga Folder"));
    m_treeDock = new QDockWidget();
    m_treeDock->setObjectName("treeDock");
    if (!mangaDirInfo.absoluteFilePath().isEmpty()) {
        setupMangaFoldersTree(mangaDirInfo);
    }
    addDockWidget(Qt::LeftDockWidgetArea, m_treeDock);

    // ==================================================
    // bookmarks dock widget
    // ==================================================
    KConfigGroup bookmarksGroup = m_config->group("Bookmarks");
    if (bookmarksGroup.keyList().count() > 0) {
        createBookmarksWidget();
    }
}

void MainWindow::setupMangaFoldersTree(QFileInfo mangaDirInfo)
{
    m_treeDock->setWindowTitle(mangaDirInfo.baseName());
    m_treeDock->show();

    m_treeModel->setObjectName("mangaTree");
    m_treeModel->setRootPath(mangaDirInfo.absoluteFilePath());
    m_treeModel->setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    m_treeModel->setNameFilters(QStringList() << "*.zip" << "*.7z" << "*.cbz" << "*.cbt" << "*.cbr" << "*.cb7" << "*.rar");
    m_treeModel->setNameFilterDisables(false);

    m_treeView->setModel(m_treeModel);
    m_treeView->setRootIndex(m_treeModel->index(mangaDirInfo.absoluteFilePath()));
    m_treeView->setColumnHidden(1, true);
    m_treeView->setColumnHidden(2, true);
    m_treeView->setColumnHidden(3, true);
    m_treeView->header()->hide();
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_treeView, &QTreeView::doubleClicked, this, [ = ](const QModelIndex &index) {
        // get path from index
        const QFileSystemModel *model = static_cast<const QFileSystemModel *>(index.model());
        QString path = model->filePath(index);
        loadImages(path);
    });
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::treeViewContextMenu);

    m_treeDock->setWidget(m_treeView);
    resizeDocks({m_treeDock}, {400}, Qt::Horizontal);
}

void MainWindow::createBookmarksWidget()
{
    auto bookmarksLayout = new QVBoxLayout();
    auto bookmarksWidget = new QWidget(this);
    auto dockWidget = new QDockWidget();
    dockWidget->setWindowTitle(i18n("Bookmarks"));
    dockWidget->setObjectName("bookmarksDockWidget");
    dockWidget->setMinimumHeight(300);

    m_bookmarksModel = new QStandardItemModel(0, 2, this);
    m_bookmarksModel->setHorizontalHeaderItem(0, new QStandardItem(i18n("Manga")));
    m_bookmarksModel->setHorizontalHeaderItem(1, new QStandardItem(i18n("Page")));

    populateBookmarkModel();

    m_bookmarksView = new QTableView();
    m_bookmarksView->setObjectName("bookmarksTableView");
    m_bookmarksView->setModel(m_bookmarksModel);
    m_bookmarksView->setWordWrap(false);
    m_bookmarksView->setTextElideMode(Qt::ElideRight);
    m_bookmarksView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_bookmarksView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bookmarksView->verticalHeader()->hide();

    auto tableHeader = m_bookmarksView->horizontalHeader();
    tableHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    tableHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    connect(m_bookmarksView, &QTableView::doubleClicked, this, [ = ](const QModelIndex &index) {
        // get path from index
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
    });

    connect(m_bookmarksView, &QTableView::customContextMenuRequested,
            this, &MainWindow::bookmarksViewContextMenu);

    auto deleteButton = new QPushButton();
    deleteButton->setText(i18n("Delete Selected Bookmarks"));
    connect(deleteButton, &QPushButton::clicked, this, [ = ]() {
        deleteBookmarks(m_bookmarksView);
    });

    bookmarksLayout->addWidget(m_bookmarksView);
    bookmarksLayout->addWidget(deleteButton);
    bookmarksWidget->setLayout(bookmarksLayout);
    dockWidget->setWidget(bookmarksWidget);
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
}

void MainWindow::populateBookmarkModel()
{
    KConfigGroup bookmarks = m_config->group("Bookmarks");
    for (const QString &key : bookmarks.keyList()) {
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
        QString displayPrefix = (key.startsWith(RECURSIVE_KEY_PREFIX)) ? "(r) " : QStringLiteral();

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
}

void MainWindow::addMangaFolder()
{
    QString path = QFileDialog::getExistingDirectory(
                this,
                i18n("Select manga folder"));
    if (path.isEmpty()) {
        return;
    }
    m_settingsWidget->kcfg_MangaFolders->insertItem(path);
    emit m_settingsWidget->kcfg_MangaFolders->changed();
}

void MainWindow::openMangaArchive()
{
    QString file = QFileDialog::getOpenFileName(
                this,
                i18n("Open Archive"),
                QDir::homePath(),
                i18n("Archives (*.zip *.rar *.7z *.cbz *.cbt *.cbr)"));
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

void MainWindow::loadImages(QString path, bool recursive, bool updateCurrentPath)
{
    if (updateCurrentPath)
        m_currentPath = path;

    m_isLoadedRecursive = recursive;
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
    m_view->setStartPage(m_startPage);
    m_view->setManga(mangaPath);
    m_view->setImages(m_images);
    m_view->loadImages();
    m_startPage = 0;
}

void MainWindow::setupActions()
{
    auto addMangaFolderAction = new QAction(this);
    addMangaFolderAction->setText(i18n("&Add Manga Folder"));
    addMangaFolderAction->setIcon(QIcon::fromTheme("folder-add"));
    actionCollection()->addAction("addMangaFolder", addMangaFolderAction);
    actionCollection()->setDefaultShortcut(addMangaFolderAction, Qt::CTRL + Qt::Key_A);
    connect(addMangaFolderAction, &QAction::triggered, this, [=]() {
        openSettings();
        addMangaFolder();
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

    m_mangaFoldersMenu = new QMenu();
    m_selectMangaFolder = new QAction(this);
    m_selectMangaFolder->setMenu(m_mangaFoldersMenu);
    m_selectMangaFolder->setText(i18n("Select Manga Folder"));
    populateMangaFoldersMenu();
    actionCollection()->addAction("selectMangaFolder", m_selectMangaFolder);
    connect(m_selectMangaFolder, &QAction::triggered, this, [ = ]() {
        QWidget *widget = toolBar("mainToolBar")->widgetForAction(m_selectMangaFolder);
        QToolButton *button = qobject_cast<QToolButton*>(widget);
        button->showMenu();
    });

    auto spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setVisible(true);
    auto spacerAction = new QWidgetAction(this);
    spacerAction->setDefaultWidget(spacer);
    spacerAction->setText(i18n("Spacer"));
    actionCollection()->addAction("spacer", spacerAction);

    auto goToLayout = new QHBoxLayout();
    auto goTowidget = new QWidget();
    goTowidget->setLayout(goToLayout);
    auto goToLabel = new QLabel(i18n("Go to page"));
    auto goToSpinBox = new QSpinBox();
    connect(goToSpinBox, &QSpinBox::editingFinished, this, [=]() {
        m_view->goToPage(goToSpinBox->value() - 1);
        m_view->setFocus();
    });
    connect(m_view, &View::imagesLoaded, this, [=]() {
        goToSpinBox->setRange(1, m_view->imageCount());
    });
    auto goToButton = new QPushButton(i18n("Go"));
    connect(goToButton, &QPushButton::clicked, this, [=]() {
        m_view->goToPage(goToSpinBox->value() - 1);
    });
    goToLayout->addWidget(goToLabel);
    goToLayout->addWidget(goToSpinBox);
    goToLayout->addWidget(goToButton);

    auto goToAction = new QWidgetAction(this);
    goToAction->setDefaultWidget(goTowidget);
    goToAction->setText(i18n("Go To Page"));
    actionCollection()->addAction("goToPage", goToAction);

    KStandardAction::showMenubar(this, &MainWindow::toggleMenubar, actionCollection());
    KStandardAction::preferences(this, &MainWindow::openSettings, actionCollection());
    KStandardAction::quit(qApp, &QCoreApplication::quit, actionCollection());

    QAction *action = KStandardAction::fullScreen(this, [ = ]() {toggleFullScreen();}, this, actionCollection());
    connect(m_view, &View::doubleClicked,
            action, &QAction::trigger);
}

void MainWindow::toggleMenubar()
{
    menuBar()->isHidden() ? menuBar()->show() : menuBar()->hide();
}

QMenu *MainWindow::populateMangaFoldersMenu()
{
    m_mangaFoldersMenu->clear();
    for (QString mangaFolder : MangaReaderSettings::mangaFolders()) {
        QAction *action = m_mangaFoldersMenu->addAction(mangaFolder);
        connect(action, &QAction::triggered, this, [ = ]() {
            m_treeModel->setRootPath(mangaFolder);
            m_treeView->setRootIndex(m_treeModel->index(mangaFolder));
            m_treeDock->setWindowTitle(QFileInfo(mangaFolder).baseName());
            m_config->group("").writeEntry("Manga Folder", mangaFolder);
            m_config->sync();
        });
    }
    m_selectMangaFolder->setVisible(false);
    // no point in showing if there's only one option
    if (MangaReaderSettings::mangaFolders().count() > 1) {
        m_selectMangaFolder->setVisible(true);
    }
    return m_mangaFoldersMenu;
}

void MainWindow::showError(QString error)
{
    QMessageBox errorWindow;
    errorWindow.setWindowTitle(i18n("Error"));
    errorWindow.setText(error);

    QSpacerItem* horizontalSpacer = new QSpacerItem(500, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = qobject_cast<QGridLayout*>(errorWindow.layout());
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    errorWindow.exec();
}

void MainWindow::extractArchive(QString archivePath)
{
    QFileInfo extractionFolder(MangaReaderSettings::extractionFolder());
    QFileInfo archivePathInfo(archivePath);
    if (!extractionFolder.exists() || !extractionFolder.isWritable()) {
        showError(i18n("Extraction folder does not exist or is not writable.\n%1",
                              MangaReaderSettings::extractionFolder()));
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

    // Extract rar archives with unrar
    QMimeDatabase mimeDB;
    QMimeType type = mimeDB.mimeTypeForFile(archivePathInfo.absoluteFilePath());
    if (type.name() == "application/vnd.comicbook-rar"
            || type.name() == "application/vnd.rar") {

        if (system("unrar -v") != 0) {
            return;
        }

        QString processName = "unrar";
        QStringList args;
        args << "e" << archivePathInfo.absoluteFilePath() << m_tmpFolder << "-o+";
        auto process = new QProcess();
        process->setProgram(processName);
        process->setArguments(args);
        process->start();

        connect(process, (void (QProcess::*)(int,QProcess::ExitStatus))&QProcess::finished,
                this, [=]() {
            m_progressBar->setVisible(false);
            loadImages(m_tmpFolder, true);
        });

        connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
            QRegularExpression re("[0-9]+[%]");
            QRegularExpressionMatch match = re.match(process->readAllStandardOutput());
            if (match.hasMatch()) {
                QString matched = match.captured(0);
                m_progressBar->setVisible(true);
                m_progressBar->setValue(matched.remove("%").toInt());
            }
        });

        connect(process, &QProcess::errorOccurred,
                this, [=](QProcess::ProcessError error) {
            QString errorMessage;
            switch (error) {
            case QProcess::FailedToStart:
                errorMessage = "FailedToStart";
            case QProcess::Crashed:
                errorMessage = "Crashed";
            case QProcess::Timedout:
                errorMessage = "Timedout";
            case QProcess::WriteError:
                errorMessage = "WriteError";
            case QProcess::ReadError:
                errorMessage = "ReadError";
            default:
                errorMessage = "UnknownError";
            }
            showError(i18n("Error: Could not open the archive. %1", errorMessage));
        });

        return;
    }

    auto extractor = new QArchive::DiskExtractor(this);
    extractor->setArchive(archivePathInfo.absoluteFilePath());
    extractor->setOutputDirectory(m_tmpFolder);
    extractor->start();

    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    connect(extractor, &QArchive::DiskExtractor::finished, this, [ = ]() {
        m_progressBar->setVisible(false);
        loadImages(m_tmpFolder, true, false);
    });

    connect(extractor, &QArchive::DiskExtractor::progress, this, [ = ](QString file, int processedFiles, int totalFiles, int percent) {
        Q_UNUSED(file)
        Q_UNUSED(processedFiles)
        Q_UNUSED(totalFiles)
        m_progressBar->setValue(percent);
    });
    connect(extractor, &QArchive::DiskExtractor::error, this, [ = ](short error) {
        showError(i18n("Error %1: Could not open the archive.\n%2",
                              QString::number(error),
                              QArchive::errorCodeToString(error)));
        m_progressBar->hide();
    });
}

void MainWindow::renameFile()
{
    QWidget *w1 = m_renameDialog->layout()->itemAt(1)->widget();
    QWidget *w2 = m_renameDialog->layout()->itemAt(2)->widget();
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(w1);
    QLabel *infoLabel = qobject_cast<QLabel *>(w2);
    QString path = m_renameDialog->property("path").toString();

    QFileInfo pathInfo(path);
    QFile file(path);
    QString newName = pathInfo.absolutePath() + "/" + lineEdit->text();

    if (m_currentPath == path && pathInfo.isDir()) {
        infoLabel->setText(i18n("Can't rename open folder."));
        return;
    }
    bool renameSuccessful = file.rename(newName);
    if (renameSuccessful) {
        if (m_currentPath == path && !pathInfo.isDir()) {
            m_currentPath = newName;
        }
        // Delete bookmarks for old name
        // and create new bookmarks for the new name
        // keys for normal and recursive bookmarks
        QString key = path;
        QString recursiveKey = RECURSIVE_KEY_PREFIX + path;

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

        m_renameDialog->accept();
    } else {
        infoLabel->setText(i18n("Renaming failed"));
    }
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

    connect(load, &QAction::triggered, this, [ = ]() {
        loadImages(path);
    });
    connect(loadRecursive, &QAction::triggered, this, [ = ]() {
        loadImages(path, true);
    });

    connect(rename, &QAction::triggered, this, [ = ]() {
        QWidget *w = m_renameDialog->layout()->itemAt(1)->widget();
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(w);
        lineEdit->setText(pathInfo.fileName());
        lineEdit->setFocus(Qt::ActiveWindowFocusReason);
        m_renameDialog->setProperty("path", path);
        m_renameDialog->exec();
    });

    connect(openPath, &QAction::triggered, this, [ = ]() {
        QString nativePath = QDir::toNativeSeparators(pathInfo.absoluteFilePath());
        QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath));
    });

    connect(openContainingFolder, &QAction::triggered, this, [ = ]() {
        QString nativePath = QDir::toNativeSeparators(pathInfo.absolutePath());
        QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath));
    });
    menu->exec(QCursor::pos());
}

void MainWindow::bookmarksViewContextMenu(QPoint point)
{
    QModelIndex index = m_bookmarksView->indexAt(point);
    QString path = m_bookmarksModel->data(index, PathRole).toString();

    QFileInfo pathInfo(path);

    auto contextMenu = new QMenu();
    auto action = new QAction(QIcon::fromTheme("unknown"), i18n("Open"));
    contextMenu->addAction(action);
    connect(action, &QAction::triggered, this, [=]() {
        QString nativePath = QDir::toNativeSeparators(pathInfo.absoluteFilePath());
        QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath));
    });

    action = new QAction(QIcon::fromTheme("folder-open"), i18n("Open containing folder"));
    contextMenu->addAction(action);
    connect(action, &QAction::triggered, this, [=]() {
        QString nativePath = QDir::toNativeSeparators(pathInfo.absolutePath());
        QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath));
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

void MainWindow::onAddBookmark(int pageIndex)
{
    int pageNumber = pageIndex + 1;
    QDockWidget *bookmarksWidget = findChild<QDockWidget *>("bookmarksDockWidget");
    if (!bookmarksWidget) {
        createBookmarksWidget();
    }
    QFileInfo mangaInfo(m_currentPath);
    QString keyPrefix = (m_isLoadedRecursive) ? RECURSIVE_KEY_PREFIX : QStringLiteral();
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
    for (const QModelIndex &index : selection.indexes()) {
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
        QModelIndex cell_1 = tableView->model()->index(rows.at(i), 1);
        bool isRecursive = tableView->model()->data(cell_0, RecursiveRole).toBool();
        QString key   = tableView->model()->data(cell_0, Qt::ToolTipRole).toString();
        if (isRecursive) {
            key = key.prepend(RECURSIVE_KEY_PREFIX);
        }
        QString value = tableView->model()->data(cell_1).toString();
        m_config->reparseConfiguration();
        KConfigGroup bookmarksGroup = m_config->group("Bookmarks");
        QString bookmarks = bookmarksGroup.readEntry(key);
        bookmarksGroup.deleteEntry(key);
        bookmarksGroup.config()->sync();
        tableView->model()->removeRow(rows.at(i));
    }
}

void MainWindow::openSettings()
{
    if (m_settingsWidget == nullptr) {
        m_settingsWidget = new SettingsWidget(nullptr);
    }
    if (KConfigDialog::showDialog("settings")) {
        return;
    }
    auto dialog = new KConfigDialog(this, "settings", MangaReaderSettings::self());
    dialog->setMinimumSize(700, 600);
    dialog->setFaceType(KPageDialog::Plain);
    dialog->addPage(m_settingsWidget, i18n("Settings"));
    dialog->show();

    // add button to open file dialog to select a folder
    // and add it to the manga folders list
    auto addMangaFolderButton = new QPushButton(i18n("Select and Add Manga Folder"));
    addMangaFolderButton->setIcon(QIcon::fromTheme("folder-add"));
    connect(addMangaFolderButton, &QPushButton::clicked, this, &MainWindow::addMangaFolder);
    auto widget = new QWidget();
    auto hLayout = new QHBoxLayout(widget);
    hLayout->setMargin(0);
    hLayout->addWidget(addMangaFolderButton);
    hLayout->addStretch(1);
    // add widget to the keditlistwidget's layout
    m_settingsWidget->kcfg_MangaFolders->layout()->addWidget(widget);

    connect(dialog, &KConfigDialog::settingsChanged,
            m_view, &View::onSettingsChanged);

    connect(m_settingsWidget->selectExtractionFolder, &QPushButton::clicked, this, [ = ]() {
        QString path = QFileDialog::getExistingDirectory(
                    this,
                    i18n("Select extraction folder"),
                    MangaReaderSettings::extractionFolder());
        if (path.isEmpty()) {
            return;
        }
        m_settingsWidget->kcfg_ExtractionFolder->setText(path);
    });

    connect(dialog, &KConfigDialog::settingsChanged, this, [ = ]() {
        QString mangaFolder = m_config->group("").readEntry("Manga Folder");
        if (MangaReaderSettings::mangaFolders().count() > 0) {
            if (mangaFolder.isEmpty()) {
                // "Manga Folder" key is empty so the tree not fully setup
                setupMangaFoldersTree(QFileInfo(MangaReaderSettings::mangaFolders().at(0)));
                m_config->group("").writeEntry("Manga Folder", MangaReaderSettings::mangaFolders().at(0));
                m_config->sync();
            }
            if (!MangaReaderSettings::mangaFolders().contains(mangaFolder)) {
                setupMangaFoldersTree(QFileInfo(MangaReaderSettings::mangaFolders().at(0)));
                m_config->group("").writeEntry("Manga Folder", MangaReaderSettings::mangaFolders().at(0));
                m_config->sync();
            }
        }
        if (MangaReaderSettings::mangaFolders().count() == 0) {
            m_treeDock->close();
            m_config->group("").deleteEntry("Manga Folder");
        }
        populateMangaFoldersMenu();
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
    } else {
        setWindowState(windowState() | Qt::WindowFullScreen);
        hideToolBars();
        hideDockWidgets();
        menuBar()->hide();
    }
}

bool MainWindow::isFullScreen()
{
    return (windowState() == (Qt::WindowFullScreen | Qt::WindowMaximized))
            || (windowState() == Qt::WindowFullScreen);
}
