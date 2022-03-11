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

void Worker::processDriveImageRequest(int number, const QString &path)
{
    const QString filename = path;
    QImage image;
    if (image.load(filename)) {
        Q_EMIT imageReady(image, number);
    }
}

void Worker::processMemoryImageRequest(int number, const QByteArray &data)
{
    QImage image = QImage::fromData(data);
    if (!image.isNull()) {
        emit imageReady(image, number);
    }
}

void Worker::processImageResize(const QImage &image, const QSize &size, int number)
{
    auto scaledImage = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    Q_EMIT imageResized(scaledImage, number);
}

auto Worker::instance() -> Worker *
{
    if (!sm_worker) {
        sm_worker = new Worker();
    }
    return sm_worker;
}
