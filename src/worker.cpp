/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

void Worker::processImageResize(const QImage &image, const QSize &size, int number)
{
    auto scaledImage = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    emit imageResized(scaledImage, number);
}

auto Worker::instance() -> Worker *
{
    if (!sm_worker) {
        sm_worker = new Worker();
    }
    return sm_worker;
}
