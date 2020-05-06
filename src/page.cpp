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

#include "_debug.h"
#include "settings.h"
#include "page.h"
#include "view.h"
#include "worker.h"

#include <QPainter>
#include <QRectF>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

Page::Page(int width, int height, int number, QGraphicsItem *parent)
    : QGraphicsItem{ parent }
    , m_view{ nullptr }
    , m_estimatedSize{ width, height }
    , m_scaledSize{ 0, 0 }
    , m_sourceSize{ 0, 0 }
    , m_maxWidth{}
    , m_number{ number }
    , m_ratio{}
{
}

void Page::setMaxWidth(int maxWidth)
{
    m_maxWidth = maxWidth;
}

void Page::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    if (m_pixmap) {
        QRectF frame(QPointF(0,0), m_pixmap->size());
        qreal w = m_pixmap->width();
        qreal h = m_pixmap->height();

        QPointF pixpos = frame.center() - (QPointF(w, h) / 2);
        QRectF border(pixpos, QSizeF(w, h));
        border.adjust(-1, -1, 0, 0);
        painter->drawRect(border);
        painter->drawPixmap(option->exposedRect, *m_pixmap, option->exposedRect);
    }
}

auto Page::zoom() const -> double
{
    return m_zoom;
}

void Page::setZoom(double zoom)
{
    m_zoom = zoom;
}

auto Page::boundingRect() const -> QRectF
{
    return {0.0F, 0.0F, static_cast<qreal>(m_scaledSize.width()), static_cast<qreal>(m_scaledSize.height())};
}

auto Page::isImageDeleted() const -> bool
{
    return m_pixmap == nullptr;
}

void Page::deleteImage()
{
    delete m_pixmap;
    m_pixmap = nullptr;
    delete m_image;
    m_image = nullptr;
}

void Page::setImage(const QImage &image)
{
    delete m_image;
    m_image = new QImage(QImage(image));
    redrawImage();
}

void Page::redrawImage()
{
    calculateSourceSize();
    calculateScaledSize();
    if (m_image) {
        Worker::instance()->processImageResize(*m_image, m_scaledSize, m_ratio, m_number);
    }
}

void Page::calculateSourceSize()
{
    int totalWidth;
    int totalHeight;
    if (!m_image) {
        totalWidth = m_estimatedSize.width();
        totalHeight = m_estimatedSize.height();
    } else {
        totalWidth = m_image->width();
        totalHeight = m_image->height();
    }
    m_sourceSize = QSize(totalWidth, totalHeight);
}

void Page::calculateScaledSize()
{
    int maxWidth = MangaReaderSettings::maxWidth();
    bool fitWidth = MangaReaderSettings::fitWidth();
    bool fitHeight = MangaReaderSettings::fitHeight();
    bool upScale = MangaReaderSettings::upScale();
    int viewWidth = m_view->width() - (m_view->verticalScrollBar()->width() + 10);
    int viewHeight = m_view->height();
    int imageWidth = m_sourceSize.width();
    int imageHeight = m_sourceSize.height();

    int width = viewWidth < maxWidth ? viewWidth : maxWidth;

    if (fitHeight || fitWidth) {
        double hRatio = fitHeight ? static_cast<double>(viewHeight) / imageHeight : 9999.0;
        double wRatio = fitWidth ? static_cast<double>(width) / imageWidth : 9999.0;
        m_ratio = hRatio < wRatio ? hRatio : wRatio;
    } else {
        m_ratio = 1.0;
    }

    if (m_ratio > 1.0 && !upScale) {
        m_ratio = 1.0;
    }

    m_scaledSize = QSize(static_cast<qint64>(imageWidth * m_ratio), static_cast<qint64>(imageHeight * m_ratio));

    if (m_zoom != 1.0) {
        m_ratio = static_cast<double>(m_scaledSize.width() * m_zoom) / imageWidth;
        m_scaledSize = QSize(static_cast<qint64>(m_scaledSize.width() * m_zoom),
                             static_cast<qint64>(m_scaledSize.height() * m_zoom));
    }
}

void Page::redraw(const QImage &image)
{
    if (image.size() == m_scaledSize) {
        // reuse existing pixmap if of right size
        if (m_pixmap != nullptr && m_pixmap->size() == image.size()) {
            QPainter p(m_pixmap);
            p.drawImage(0, 0, image);
            p.end();
        } else {
            delete m_pixmap;
            m_pixmap = new QPixmap(QPixmap::fromImage(image));
        }
        update();
    }
}

void Page::setEstimatedSize(QSize estimatedSize)
{
    m_estimatedSize = estimatedSize;
}

void Page::setView(View *view)
{
    m_view = view;
}

auto Page::number() -> int
{
    return m_number;
}

auto Page::estimatedSize() -> QSize
{
    return m_estimatedSize;
}

auto Page::scaledSize() -> QSize
{
    return m_scaledSize;
}

auto Page::sourceSize() -> QSize
{
    return m_sourceSize;
}
