#ifndef STARTUPWIDGET_H
#define STARTUPWIDGET_H

#include <QWidget>

class StartUpWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StartUpWidget(QWidget *parent = nullptr);

Q_SIGNALS:
    void addMangaFolderClicked();
    void openMangaFolderClicked();
    void openMangaArchiveClicked();

};

#endif // STARTUPWIDGET_H
