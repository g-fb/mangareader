/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <QString>
#include <QSize>

struct Image {
    QString path;
    QSize size;
};

#endif // IMAGE_H
