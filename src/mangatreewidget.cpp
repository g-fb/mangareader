#include "mangatreewidget.h"

#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTreeView>

#include <KFileItem>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KIO/RenameFileDialog>
#include <KLocalizedString>

using namespace Qt::StringLiterals;

MangaTreeWidget::MangaTreeWidget()
    : m_treeView{ new QTreeView() }
    , m_treeModel{ new QFileSystemModel() }
{
    m_treeModel->setObjectName("mangaTree");
    m_treeModel->setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    m_treeModel->setNameFilters(QStringList() << u"*.zip"_s << u"*.cbz"_s
                                              << u"*.rar"_s << u"*.cbr"_s
                                              << u"*.7z"_s  << u"*.cb7"_s
                                              << u"*.tar"_s << u"*.cbt"_s);
    m_treeModel->setNameFilterDisables(false);

    m_treeProxyModel = new QSortFilterProxyModel(this);
    m_treeProxyModel->setSourceModel(m_treeModel);
    m_treeProxyModel->setRecursiveFilteringEnabled(true);
    m_treeProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_treeProxyModel->setFilterKeyColumn(0);

    m_treeView->setModel(m_treeProxyModel);
    m_treeView->setColumnHidden(1, true);
    m_treeView->setColumnHidden(2, true);
    m_treeView->setColumnHidden(3, true);
    m_treeView->header()->hide();
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_treeModel->setRootPath(mangaFolder);
    m_treeView->setRootIndex(m_treeProxyModel->mapFromSource(m_treeModel->index(mangaFolder)));

    auto action = new QAction();
    action->setShortcuts({Qt::Key_Enter, Qt::Key_Return});
    action->setShortcutContext(Qt::WidgetShortcut);
    connect(action, &QAction::triggered, this, [this]() {
        const auto modelIndex = m_treeProxyModel->mapToSource(m_treeView->selectionModel()->currentIndex());
        const QString path = m_treeModel->filePath(modelIndex);
        const auto resume{false};
        const auto recursive{false};
        Q_EMIT open(path, resume, recursive);
    });
    m_treeView->addAction(action);

    connect(m_treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        const auto modelIndex = m_treeProxyModel->mapToSource(index);
        const QString path = m_treeModel->filePath(modelIndex);
        const auto resume{false};
        const auto recursive{false};
        Q_EMIT open(path, resume, recursive);
    });
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MangaTreeWidget::treeViewContextMenu);

    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText(i18n("Search"));
    connect(m_searchField, &QLineEdit::textChanged, m_treeProxyModel, [this]() {
        const auto regEx = QRegularExpression(m_searchField->text(), QRegularExpression::CaseInsensitiveOption);
        m_treeProxyModel->setFilterRegularExpression(regEx);
    });
    connect(m_treeModel, &QFileSystemModel::directoryLoaded, this, [this]() {
        // filter when model finished loading items
        // m_searchField->setText(u""_s);
    });

    auto *l = new QVBoxLayout(this);
    l->addWidget(m_searchField);
    l->addWidget(m_treeView);
}

QTreeView *MangaTreeWidget::treeView() const
{
    return m_treeView;
}

QFileSystemModel *MangaTreeWidget::treeModel() const
{
    return m_treeModel;
}

QString MangaTreeWidget::getMangaFolder() const
{
    return mangaFolder;
}

void MangaTreeWidget::setMangaFolder(const QString &newMangaFolder)
{
    const auto modelIndex = m_treeProxyModel->mapFromSource(m_treeModel->index(newMangaFolder));
    m_treeModel->setRootPath(newMangaFolder);
    m_treeView->setRootIndex(modelIndex);

    mangaFolder = newMangaFolder;
}

void MangaTreeWidget::treeViewContextMenu(QPoint point)
{
    QModelIndex index = m_treeView->indexAt(point);
    const auto modelIndex = m_treeProxyModel->mapToSource(index);
    QString path = m_treeModel->filePath(modelIndex);

    auto menu = new QMenu();
    menu->setMinimumWidth(200);

    auto load = new QAction(QIcon::fromTheme(u"arrow-down"_s), i18n("Load"));
    m_treeView->addAction(load);

    auto loadRecursive = new QAction(QIcon::fromTheme(u"arrow-down-double"_s), i18n("Load recursive"));
    m_treeView->addAction(loadRecursive);

    auto rename = new QAction(QIcon::fromTheme(u"edit-rename"_s), i18n("Rename"));
    m_treeView->addAction(rename);

    auto openPath = new QAction(QIcon::fromTheme(u"unknown"_s), i18n("Open"));
    m_treeView->addAction(openPath);

    auto openContainingFolder = new QAction(QIcon::fromTheme(u"folder-open"_s), i18n("Open containing folder"));
    m_treeView->addAction(openContainingFolder);

    menu->addAction(load);
    menu->addAction(loadRecursive);
    menu->addAction(rename);
    menu->addSeparator();
    menu->addAction(openPath);
    menu->addAction(openContainingFolder);

    connect(load, &QAction::triggered, this, [this, path]() {
        const auto resume{false};
        const auto recursive{false};
        Q_EMIT open(path, resume, recursive);
    });
    connect(loadRecursive, &QAction::triggered, this, [this, path]() {
        const auto resume{false};
        const auto recursive{true};
        Q_EMIT open(path, resume, recursive);
    });

    connect(rename, &QAction::triggered, this, [this, path]() {
        QUrl url(path);
        url.setScheme(u"file"_s);
        KFileItem item(url);
        auto renameDialog = new KIO::RenameFileDialog(KFileItemList({item}), nullptr);
        renameDialog->open();
        connect(renameDialog, &KIO::RenameFileDialog::renamingFinished, this, [this, path](const QList<QUrl> &urls) {
            auto newName = urls.first().toLocalFile();
            Q_EMIT renamed(path, newName);
        });
    });

    connect(openPath, &QAction::triggered, this, [path]() {
        QUrl url(path);
        url.setScheme(QStringLiteral("file"));
        auto job = new KIO::OpenUrlJob(url);
        job->setUiDelegate(KIO::createDefaultJobUiDelegate());
        job->start();
    });

    connect(openContainingFolder, &QAction::triggered, this, [path]() {
        QUrl url(path);
        url.setScheme(QStringLiteral("file"));
        KIO::highlightInFileManager({url});
    });
    menu->exec(QCursor::pos());
}
