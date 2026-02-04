#ifndef IMAGEREQUEST_H
#define IMAGEREQUEST_H

#include <QImage>
#include <QSize>
#include <QString>

struct ImageRequest {
    int pageNumber;
    QSize size;
    QString path;
    QImage image;
};


#endif // IMAGEREQUEST_H
