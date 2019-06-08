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
#include "worker.h"

#include <QImage>
#include <QPainter>

Worker* Worker::sm_worker = 0;

void Worker::setImages(QStringList images)
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

void Worker::processImageResize(const QImage &image, const QSize& size, double ratio, int number)
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

Worker* Worker::instance()
{
    if (!sm_worker) {
        sm_worker = new Worker();
    }
    return sm_worker;
}
