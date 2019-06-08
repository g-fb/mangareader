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

#ifndef WORKER_H
#define WORKER_H

#include <QObject>

class Worker : public QObject
{
    Q_OBJECT
public:
    Worker() = default;
    ~Worker() = default;

    static Worker* instance();
    void setImages(QStringList images);

public slots:
    void processImageRequest(int);
    void processImageResize(const QImage &image, const QSize& size, double ratio, int number);

signals:
    void imageReady(QImage image, int number);
    void imageResized(const QImage &image, int number);

private:
    QStringList m_images;
    static Worker *sm_worker;

};

#endif // WORKER_H
