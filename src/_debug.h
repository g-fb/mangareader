/*
 * SPDX-FileCopyrightText: 2021 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef __MANGAREADERDEBUG_H
#define __MANGAREADERDEBUG_H

#include <QDebug>

#define DEBUG qDebug() << Q_FUNC_INFO << ':'

#endif
