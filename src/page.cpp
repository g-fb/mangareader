/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "settings.h"

#include <QPainter>
#include <QRectF>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

#include <cmath>

#include "page.h"
#include "view.h"
#include "worker.h"

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
    if (m_pixmap.isNull()) {
        return;
    }

    const QRectF pixRect(0, 0, m_pixmap.width(), m_pixmap.height());

    painter->save();
    painter->setPen(QPen(MangaReaderSettings::borderColor(), 1));

    if (MangaReaderSettings::vPageSpacing() > 0) {
        painter->drawRect(pixRect.adjusted(-0.5, -0.5, 0.5, 0.5));
    } else {
        painter->drawLine(-1, 0, -1, m_pixmap.height());
        painter->drawLine(m_pixmap.width() + 1, 0, m_pixmap.width() + 1, m_pixmap.height());
    }

    painter->restore();
    painter->drawPixmap(option->exposedRect, m_pixmap, option->exposedRect);
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
    qreal borderWidth = (MangaReaderSettings::vPageSpacing() >= 0) ? 1.0 : 0.0;
    return QRectF(0.0, 0.0, m_scaledSize.width(), m_scaledSize.height())
        .adjusted(-borderWidth, -borderWidth, borderWidth, borderWidth);
}

auto Page::isImageDeleted() const -> bool
{
    return m_pixmap.isNull();
}

void Page::deleteImage()
{
    m_pixmap = QPixmap{};
    m_image = QImage{};
}

const QImage &Page::image() const
{
    return m_image;
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
        QMetaObject::invokeMethod(Worker::instance(), &Worker::processImageResize, Qt::QueuedConnection, m_image, m_scaledSize, m_number);
    }
}

void Page::calculateScaledSize()
{
    if (!m_view) {
        return;
    }

    const int maxWidth = MangaReaderSettings::maxWidth();
    const bool fitWidth = MangaReaderSettings::fitWidth();
    const bool fitHeight = MangaReaderSettings::fitHeight();
    const bool upScale = MangaReaderSettings::upScale();
    const int hSpacing = MangaReaderSettings::hPageSpacing();

    const int viewportWidth = m_view->viewport()->width();
    const int viewportHeight = m_view->viewport()->height();
    const int totalBorderWidth = 2;
    int availableWidth = viewportWidth;
    if (MangaReaderSettings::show2PagesPerRow()) {
        availableWidth = (viewportWidth - hSpacing) / 2;
    }

    int targetWidth = std::min(availableWidth - totalBorderWidth - 2, maxWidth);

    double ratio = 1.0;
    if (fitHeight || fitWidth) {
        double hRatio = fitHeight ? static_cast<double>(viewportHeight - totalBorderWidth) / m_sourceSize.height() : 1e6;
        double wRatio = fitWidth ? static_cast<double>(targetWidth) / m_sourceSize.width() : 1e6;
        ratio = std::min(hRatio, wRatio);
    }

    if (ratio > 1.0 && !upScale) {
        ratio = 1.0;
    }
    m_ratio = ratio * m_zoom;

    m_scaledSize = m_sourceSize * m_ratio;
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
