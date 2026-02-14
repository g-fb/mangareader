#ifndef MANGATREEVIEW_H
#define MANGATREEVIEW_H

#include <QObject>
#include <QSortFilterProxyModel>
#include <QWidget>

class QTreeView;
class QFileSystemModel;
class QLineEdit;

class FSProxyModel : public QSortFilterProxyModel
{
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

class MangaTreeWidget : public QWidget
{
    Q_OBJECT
public:
    MangaTreeWidget();

    QTreeView *treeView() const;

    QFileSystemModel *treeModel() const;

    QString getMangaFolder() const;
    void setMangaFolder(const QString &newMangaFolder);

    QModelIndex currentModelIndex(const QString &path) const;
    QModelIndex nextModelIndex(const QModelIndex &index) const;
    QModelIndex previousModelIndex(const QModelIndex &index) const;
    QString filePath(const QModelIndex &index) const;

    bool isEmpty() const;

Q_SIGNALS:
    void open(QString mangaPath, bool resume = false, bool recursive = false);
    void renamed(QString oldName, QString newName);
    void openContextMenu();

private:
    void treeViewContextMenu(QPoint point);

    QTreeView        *m_treeView{nullptr};
    QFileSystemModel *m_treeModel{nullptr};
    FSProxyModel     *m_treeProxyModel{nullptr};
    QLineEdit        *m_searchField{nullptr};
    QString           m_mangaFolder;
    bool              m_isEmpty{true};
};

#endif // MANGATREEVIEW_H
