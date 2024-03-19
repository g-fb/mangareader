/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "worker.h"

#include <QImage>
#include <QPainter>

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
        Q_EMIT imageReady(image, number);
    }
}

void Worker::processImageResize(const QImage &image, const QSize &size, int number)
{
    auto scaledImage = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    Q_EMIT imageResized(scaledImage, number);
}

auto Worker::instance() -> Worker *
{
    static Worker w;
    return &w;
}

#include "moc_worker.cpp"
