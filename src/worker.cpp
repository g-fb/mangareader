/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "worker.h"

#include <QImage>
#include <QPainter>

Worker::Worker()
{
    m_extractor = new Extractor(this);
    connect(m_extractor, &Extractor::finishedRar, this, [this]() {
        Q_EMIT rarExtracted(m_extractor->extractionFolder());
    });
}

void Worker::processDriveImageRequest(int number, const QString &path)
{
    const QString filename = path;
    QImage image;
    if (image.load(filename)) {
        Q_EMIT imageReady(image, number);
    }
}

void Worker::processMemoryImageRequest(int number, const QString &name)
{
    QImage image = QImage::fromData(m_extractor->getFileData(name));
    if (!image.isNull()) {
        Q_EMIT imageReady(image, number);
    }
}

void Worker::processImageResize(const QImage &image, const QSize &size, int number)
{
    auto scaledImage = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    Q_EMIT imageResized(scaledImage, number);
}

void Worker::processArchive(const QString path)
{
    m_extractor->open(path);
    if (!m_extractor->isRar()) {
        const auto images = m_extractor->filesList();
        Q_EMIT archiveProcessed(images);
    }
}

#include "moc_worker.cpp"
