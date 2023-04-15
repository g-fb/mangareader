/*
 * SPDX-FileCopyrightText: 2021 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STARTUPWIDGET_H
#define STARTUPWIDGET_H

#include <QWidget>

class StartUpWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StartUpWidget(QWidget *parent = nullptr);
    void mouseMoveEvent(QMouseEvent *event);

Q_SIGNALS:
    void addMangaFolderClicked();
    void openMangaFolderClicked();
    void openMangaArchiveClicked();
    void openSettingsClicked();
    void openShortcutsConfigClicked();
    void mouseMoved(QMouseEvent *event);

};

#endif // STARTUPWIDGET_H
