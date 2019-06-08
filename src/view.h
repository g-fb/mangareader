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

signals:
    void requestPage(int number);

public slots:
    void onImageReady(QImage image, int number);
    void onImageResized(QImage image, int number);
    void onScrollBarRangeChanged(int x, int y);

private:
    void resizeEvent(QResizeEvent *e) override;
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
};

#endif // VIEW_H
