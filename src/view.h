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

#ifndef VIEW_H
#define VIEW_H

#include <QGraphicsView>
#include <QObject>

class Page;
class QGraphicsScene;

class View : public QGraphicsView
{
    Q_OBJECT

public:
    View(QWidget *parent = nullptr);
    ~View() = default;
    void reset();
    void setManga(QString manga);
    void setImages(QStringList images);
    void loadImages();
    void goToPage(int number);
    void setStartPage(int number);

signals:
    void requestPage(int number);
    void doubleClicked();
    void mouseMoved(QMouseEvent *event);
    void addBookmark(int number);

public slots:
    void onImageReady(QImage image, int number);
    void onImageResized(QImage image, int number);
    void onScrollBarRangeChanged(int x, int y);
    void onSettingsChanged();

private:
    void resizeEvent(QResizeEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void createPages();
    void calculatePageSizes();
    void setPagesVisibility();
    void scrollContentsBy(int dx, int dy) override;
    void addRequest(int number);
    void delRequest(int number);
    bool hasRequest(int number) const;
    bool isInView  (int imgTop, int imgBot);

    QGraphicsScene  *m_scene;
    QString          m_manga;
    QStringList      m_images;
    QVector<Page*>   m_pages;
    QVector<int>     m_start;
    QVector<int>     m_end;
    QVector<int>     m_requestedPages;
    int              m_firstVisible = -1;
    double           m_firstVisibleOffset = 0.0f;
    int              m_startPage = 0;
};

#endif // VIEW_H
