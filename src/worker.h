/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WORKER_H
#define WORKER_H

#include <QObject>

class Worker : public QObject
{
    Q_OBJECT
public:
    Worker() = default;
    ~Worker() = default;

    static auto instance() -> Worker *;
    void setImages(const QStringList &images);

public slots:
    void processImageRequest(int);
    void processImageResize(const QImage &image, const QSize &size, int number);

signals:
    void imageReady(QImage image, int number);
    void imageResized(const QImage &image, int number);

private:
    QStringList m_images;
    static Worker *sm_worker;
};

#endif // WORKER_H
