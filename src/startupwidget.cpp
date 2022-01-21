/*
 * SPDX-FileCopyrightText: 2021 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "startupwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <KLocalizedString>

#include <settings.h>

StartUpWidget::StartUpWidget(QWidget *parent)
    : QWidget{parent}
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    auto mainVLayout = new QVBoxLayout(this);
    setLayout(mainVLayout);

    auto bottomWidget = new QWidget(this);
    auto bottomHLayout = new QHBoxLayout(bottomWidget);

    auto image = new QLabel(this);
    image->setPixmap(QIcon::fromTheme("mangareader").pixmap(256));
    image->setAlignment(Qt::AlignCenter);

    mainVLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainVLayout->addWidget(image);
    mainVLayout->addWidget(bottomWidget);
    mainVLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainVLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));

    auto addMangaFolderButton = new QPushButton(i18n("Add Manga Library Folder"), this);
    addMangaFolderButton->setIcon(QIcon::fromTheme("folder"));
    addMangaFolderButton->setIconSize(QSize(32, 32));
    addMangaFolderButton->setVisible(MangaReaderSettings::mangaFolders().isEmpty());
    connect(addMangaFolderButton, &QPushButton::clicked,
            this, &StartUpWidget::addMangaFolderClicked);

    auto openMangaFolderButton = new QPushButton(i18n("Open Manga Folder"), this);
    openMangaFolderButton->setIcon(QIcon::fromTheme("folder"));
    openMangaFolderButton->setIconSize(QSize(32, 32));
    connect(openMangaFolderButton, &QPushButton::clicked,
            this, &StartUpWidget::openMangaFolderClicked);

    auto openMangaArchiveButton = new QPushButton(i18n("Open Manga Archive"), this);
    openMangaArchiveButton->setIcon(QIcon::fromTheme("application-x-archive"));
    openMangaArchiveButton->setIconSize(QSize(32, 32));
    connect(openMangaArchiveButton, &QPushButton::clicked,
            this, &StartUpWidget::openMangaArchiveClicked);

    bottomHLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    bottomHLayout->addWidget(addMangaFolderButton);
    bottomHLayout->addWidget(openMangaFolderButton);
    bottomHLayout->addWidget(openMangaArchiveButton);
    bottomHLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
}
