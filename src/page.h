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
    auto estimatedSize() -> QSize;
    auto scaledSize() -> QSize;
    auto sourceSize() -> QSize;
    auto isImageDeleted() const -> bool;
    auto number() -> int;
    auto zoom() const -> double;
    void setZoom(double zoom);

private:
    void calculateScaledSize();
    void calculateSourceSize();
    auto boundingRect() const -> QRectF override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

    View    *m_view;
    QSize    m_estimatedSize;
    QSize    m_scaledSize;
    QSize    m_sourceSize;
    int      m_maxWidth;
    int      m_number;
    double   m_zoom = 1.0;
    double   m_ratio;
    QPixmap *m_pixmap = nullptr;
    QImage  *m_image = nullptr;
};

#endif // PAGE_H
