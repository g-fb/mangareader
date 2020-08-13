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
#include "worker.h"

#include <QImage>
#include <QPainter>

Worker* Worker::sm_worker = nullptr;

void Worker::setImages(const QStringList &images)
{
    m_images = images;
}

void Worker::processImageRequest(int number)
{
    const QString filename = m_images.at(number);
    QImage image;
    if (image.load(filename)) {
        emit imageReady(image, number);
    }
}

void Worker::processImageResize(const QImage &image, const QSize &size, double ratio, int number)
{
    auto m_result = new QImage(size.width(), size.height(), QImage::Format_ARGB32);
    m_result->fill(0);

    QTransform transform;
    transform.scale(ratio, ratio);

    QPainter p(m_result);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setTransform(transform);

    if (!image.isNull()) {
        p.drawImage(0, 0, image, 0, 0);
    }
    p.end();

    emit imageResized(*m_result, number);
    delete m_result;
}

auto Worker::instance() -> Worker *
{
    if (!sm_worker) {
        sm_worker = new Worker();
    }
    return sm_worker;
}
