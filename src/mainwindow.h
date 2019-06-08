#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KSharedConfig>
#include <KXmlGuiWindow>

class QProgressBar;
class View;
class Worker;

class MainWindow : public KXmlGuiWindow
{
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void init();
    void setupActions();
    void addMangaFolder();
    void loadImages(const QModelIndex &index);

    QProgressBar *m_progressBar;
    KSharedConfig::Ptr m_config;
    QStringList m_images;
    QString m_currentManga;
    View *m_view;
    Worker *m_worker;
    QThread *m_thread;
};

#endif // MAINWINDOW_H
