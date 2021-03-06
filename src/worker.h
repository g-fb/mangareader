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

    Worker(const Worker &) = delete;
    Worker &operator=(const Worker &) = delete;
    Worker(Worker &&) = delete;
    Worker &operator=(Worker &&) = delete;

    static auto instance() -> Worker *;

public Q_SLOTS:
    void processDriveImageRequest(int, const QString &);
    void processMemoryImageRequest(int, const QByteArray &);
    void processImageResize(const QImage &image, const QSize &size, int number);

Q_SIGNALS:
    void imageReady(const QImage &image, int number);
    void imageResized(const QImage &image, int number);
};

#endif // WORKER_H
