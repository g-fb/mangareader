/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VIEW_H
#define VIEW_H

#include <QGraphicsView>
#include <QObject>
#include <KXMLGUIClient>

#include "image.h"
#include "manga.h"

class Page;
class QGraphicsScene;
class QPropertyAnimation;
class MainWindow;

class View : public QGraphicsView, public KXMLGUIClient
{
    Q_OBJECT

public:
    View(MainWindow *parent);
    ~View();
    void reset();
    void openManga(const QString &path, bool recursive);
    void loadImages();
    void goToPage(int number);
    auto imageCount() -> int;
    void setStartPage(int number);
    void setFiles(const QList<Image> &images);

    void setLoadFromMemory(bool newLoadFromMemory);

    bool event(QEvent *event) override;

Q_SIGNALS:
    void imagesLoaded(int number);
    void requestImage(int number, const QString &name);
    void currentImageChanged(int number);
    void doubleClicked();
    void mouseMoved(QMouseEvent *event);
    void addBookmark(int number, bool recursive);
    void fileDropped(const QString &file);

public Q_SLOTS:
    void onImageReady(const QImage &image, int number);
    void onImageResized(const QImage &image, int number);
    void onScrollBarRangeChanged(int x, int y);
    void refreshPages();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void togglePageZoom(Page *page);

private:
    void setupActions();
    void createPages();
    void calculatePageSizes();
    void setPagesVisibility();
    void addRequest(int number);
    void delRequest(int number);
    void resizeEvent(QResizeEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

    QGraphicsScene  *m_scene{nullptr};
    std::unique_ptr<Manga> m_manga;
    QList<Image>     m_files;
    QList<Page*>     m_pages;
    QList<Page*>     m_visiblePages;
    QList<int>       m_start;
    QList<int>       m_end;
    QSet<int>        m_requestedPages;
    int              m_startPage{0};
    int              m_firstVisible{-1};
    float            m_firstVisibleOffset{0.0f};
    double           m_globalZoom{1.0};
    QTimer          *m_resizeTimer{nullptr};
    bool             m_loadFromMemory {false};
    QPropertyAnimation *m_scrollAnimation{nullptr};
    int              m_targetScrollValue{0};
};

#endif // VIEW_H
