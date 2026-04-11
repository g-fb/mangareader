/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
