#ifndef MANGATREEVIEW_H
#define MANGATREEVIEW_H

#include <QObject>
#include <QWidget>

class QTreeView;
class QFileSystemModel;

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

    QTreeView          *m_treeView{};
    QFileSystemModel   *m_treeModel{};
    QString mangaFolder;
};

#endif // MANGATREEVIEW_H
