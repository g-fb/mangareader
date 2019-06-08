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
