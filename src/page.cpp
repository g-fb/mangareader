/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "settings.h"
#include "page.h"
#include "view.h"
#include "worker.h"

#include <QPainter>
#include <QRectF>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

Page::Page(QSize sourceSize, QGraphicsItem *parent)
    : QGraphicsItem{ parent }
    , m_view{ nullptr }
    , m_scaledSize{ 0, 0 }
    , m_sourceSize{ sourceSize }
    , m_maxWidth{}
    , m_ratio{}
{
}

Page::~Page()
{
    deleteImage();
}

void Page::setMaxWidth(int maxWidth)
{
    m_maxWidth = maxWidth;
}

void Page::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    if (!m_pixmap.isNull()) {
        auto w = m_pixmap.width();
        auto h = m_pixmap.height();

        // draw border arround the image
        if (MangaReaderSettings::pageSpacing() > 0) {
            QRectF border(QPointF(0, 0), QSizeF(w, h));
            border.adjust(-1, -1, 0, 0);

            painter->setPen(QPen(MangaReaderSettings::borderColor()));
            painter->drawRect(border);
        }

        // draw border only on the sides
        if (MangaReaderSettings::pageSpacing() == 0) {
            painter->setPen(QPen(MangaReaderSettings::borderColor()));
            painter->drawLine(0, 0, 0, h);
            painter->drawLine(w, 0, w, h);
        }

        // set default pen, else the pen's size is included when drawing the pixmap
        // resulting in a small gap between images
        painter->setPen(QPen());
        painter->drawPixmap(option->exposedRect, m_pixmap, option->exposedRect);
    }
}

const QString &Page::filename() const
{
    return m_filename;
}

void Page::setFilename(const QString &newFilename)
{
    m_filename = newFilename;
}

bool Page::isZoomToggled() const
{
    return m_isZoomToggled;
}

void Page::setIsZoomToggled(bool isZoomToggled)
{
    m_isZoomToggled = isZoomToggled;
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
    return m_pixmap.isNull();
}

void Page::deleteImage()
{
    m_pixmap = QPixmap();
    m_image = QImage();
}

void Page::setImage(const QImage &image)
{
    m_image = image;
    redrawImage();
}

void Page::redrawImage()
{
    calculateScaledSize();
    if (!m_image.isNull()) {
        Worker::instance()->processImageResize(m_image, m_scaledSize, m_number);
    }
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
    // reuse existing pixmap if of right size
    if (!m_pixmap.isNull() && m_pixmap.size() == image.size()) {
        QPainter p(&m_pixmap);
        p.drawImage(0, 0, image);
        p.end();
    } else {
        m_pixmap = QPixmap();
        m_pixmap = QPixmap::fromImage(image);
    }
    update();
}

void Page::setView(View *view)
{
    m_view = view;
}

auto Page::number() -> int
{
    return m_number;
}

void Page::setNumber(int newNumber)
{
    m_number = newNumber;
}

void Page::setScaledSize(QSize size)
{
    m_scaledSize = size;
}

auto Page::scaledSize() -> QSize
{
    return m_scaledSize;
}

auto Page::sourceSize() -> QSize
{
    return m_sourceSize;
}
