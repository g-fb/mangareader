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
    auto treeView = new QTreeView(this);

    treeDock->setObjectName("treeDock");

    treeModel->setObjectName("mangaTree");
    treeModel->setRootPath(mangaDirInfo.absoluteFilePath());
    treeModel->setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    treeModel->setNameFilters(QStringList() << "*.zip" << "*.7z" << "*.cbz");
    treeModel->setNameFilterDisables(false);

    treeView->setModel(treeModel);
    treeView->setRootIndex(treeModel->index(mangaDirInfo.absoluteFilePath()));
    treeView->setColumnHidden(1, true);
    treeView->setColumnHidden(2, true);
    treeView->setColumnHidden(3, true);
    treeView->header()->hide();

    connect(treeView, &QTreeView::doubleClicked,
            this, &MainWindow::loadImages);

    treeDock->setWidget(treeView);
    addDockWidget(Qt::LeftDockWidgetArea, treeDock);
}

MainWindow::~MainWindow()
{
    m_thread->quit();
    m_thread->wait();
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

void MainWindow::loadImages(const QModelIndex &index)
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
    QDirIterator *it = new QDirIterator(fileInfo.absoluteFilePath(), QDir::Files, QDirIterator::NoIteratorFlags);
//    if (recursive) {
//      it = new QDirIterator(path, QDir::Files, QDirIterator::Subdirectories);
//    }
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
    QAction *addMangaFolder = new QAction(this);
    addMangaFolder->setText(i18n("&Add Manga Folder"));
    addMangaFolder->setIcon(QIcon::fromTheme("folder-open"));
    actionCollection()->addAction("addMangaFolder", addMangaFolder);
    actionCollection()->setDefaultShortcut(addMangaFolder, Qt::CTRL + Qt::Key_A);
    connect(addMangaFolder, &QAction::triggered,
            this, &MainWindow::addMangaFolder);
}
