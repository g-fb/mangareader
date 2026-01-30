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

Q_SIGNALS:
    void open(QString mangaPath, bool resume = false, bool recursive = false);
    void renamed(QString oldName, QString newName);
    void openContextMenu();

private:
    void treeViewContextMenu(QPoint point);

    QTreeView        *m_treeView{};
    QFileSystemModel *m_treeModel{};
    FSProxyModel     *m_treeProxyModel{};
    QLineEdit        *m_searchField{};
    QString           m_mangaFolder;
};

#endif // MANGATREEVIEW_H
