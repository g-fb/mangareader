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
    : QGraphicsItem(parent)
    , m_estimatedSize(width, height)
    , m_scaledSize(0, 0)
    , m_sourceSize(0, 0)
    , m_number(number)
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

QRectF Page::boundingRect() const
{
    return QRectF(0.0f, 0.0f, m_scaledSize.width(), m_scaledSize.height());
}

bool Page::isImageDeleted() const
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
    int totalWidth, totalHeight;
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
    int viewWidth = m_view->width() - (m_view->verticalScrollBar()->width() + 10);
    int imageWidth = m_sourceSize.width();
    int imageHeight = m_sourceSize.height();

    m_scaledSize = QSize(imageWidth, imageHeight);
    if (viewWidth > maxWidth) {
        if (imageWidth < maxWidth) {
            m_ratio = 1.0;
            m_scaledSize = QSize(imageWidth, imageHeight);
        } else {
            double wRatio = static_cast<double>(maxWidth) / imageWidth;
            m_ratio = wRatio;
            m_scaledSize = QSize(maxWidth, imageHeight * wRatio);
        }
    } else {
        if (imageWidth < viewWidth) {
            m_ratio = 1.0;
            m_scaledSize = QSize(imageWidth, imageHeight);
        } else {
            double wRatio = static_cast<double>(viewWidth) / imageWidth;
            m_ratio = wRatio;
            m_scaledSize = QSize(viewWidth, imageHeight * wRatio);
        }
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

int Page::number()
{
    return m_number;
}

QSize Page::estimatedSize()
{
    return m_estimatedSize;
}

QSize Page::scaledSize()
{
    return m_scaledSize;
}

QSize Page::sourceSize()
{
    return m_sourceSize;
}
