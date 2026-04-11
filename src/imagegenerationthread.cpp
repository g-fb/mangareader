/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "imagegenerationthread.h"

#include "manga.h"

ImageGenerationThread::ImageGenerationThread(Manga *manga)
    : m_manga{manga}
{}

void ImageGenerationThread::startGeneration(ImageRequest *request)
{
    m_request = request;
    start(QThread::InheritPriority);
}

void ImageGenerationThread::endGeneration()
{
    m_request = nullptr;
}

ImageRequest *ImageGenerationThread::request() const
{
    return m_request;
}

void ImageGenerationThread::run()
{
    if (m_request) {
        m_request->image = m_manga->image(m_request);
    }
}
