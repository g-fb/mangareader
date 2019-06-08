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

#ifndef PAGE_H
#define PAGE_H

#include <QGraphicsItem>

class QPixmap;
class View;

class Page : public QGraphicsItem
{
public:
    Page(int width, int height, int number, QGraphicsItem *parent = nullptr);
    ~Page() = default;
    void setView(View *view);
    void setMaxWidth(int maxWidth);
    void setImage(const QImage &image);
    void redrawImage();
    void redraw(const QImage &image);
    void deleteImage();
    void setEstimatedSize(QSize estimatedSize);
    QSize estimatedSize();
    QSize scaledSize();
    QSize sourceSize();
    bool isImageDeleted() const;
    int number();

private:
    void calculateScaledSize();
    void calculateSourceSize();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

    View    *m_view;
    QSize    m_estimatedSize;
    QSize    m_scaledSize;
    QSize    m_sourceSize;
    int      m_number;
    int      m_maxWidth;
    double   m_ratio;
    QPixmap *m_pixmap = nullptr;
    QImage  *m_image = nullptr;
};

#endif // PAGE_H
