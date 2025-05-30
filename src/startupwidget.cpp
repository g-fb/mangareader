/*
 * SPDX-FileCopyrightText: 2021 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "startupwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>

#include <KLocalizedString>
#include <KColorSchemeManager>
#include <KActionMenu>
#include <KConfigGroup>

#include "settings.h"

using namespace Qt::StringLiterals;

StartUpWidget::StartUpWidget(QWidget *parent)
    : QWidget{parent}
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    auto mainVLayout = new QVBoxLayout(this);
    setLayout(mainVLayout);

    auto firstButtonsRow = new QWidget(this);
    auto firstButtonsRowLayout = new QHBoxLayout(firstButtonsRow);
    auto secondButtonsRow = new QWidget(this);
    auto secondButtonsRowLayout = new QHBoxLayout(secondButtonsRow);

    auto image = new QLabel(this);
    QIcon qrcIcon = QIcon(u":/icons/mangareader"_s);
#ifdef Q_OS_WIN32
    image->setPixmap(qrcIcon.pixmap(256));
#else
    image->setPixmap(QIcon::fromTheme(u"mangareader"_s, qrcIcon).pixmap(256));
#endif
    image->setAlignment(Qt::AlignCenter);

    mainVLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainVLayout->addWidget(image);
    mainVLayout->addWidget(firstButtonsRow);
    mainVLayout->addWidget(secondButtonsRow);
    mainVLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainVLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));

    auto addMangaFolderButton = new QPushButton(i18n("Add Manga Library Folder"), this);
    addMangaFolderButton->setIcon(QIcon::fromTheme(u"folder"_s));
    addMangaFolderButton->setIconSize(QSize(32, 32));
    addMangaFolderButton->setVisible(MangaReaderSettings::mangaFolders().isEmpty());
    connect(addMangaFolderButton, &QPushButton::clicked,
            this, &StartUpWidget::addMangaFolderClicked);

    auto openMangaFolderButton = new QPushButton(i18n("Open Manga Folder"), this);
    openMangaFolderButton->setIcon(QIcon::fromTheme(u"folder"_s));
    openMangaFolderButton->setIconSize(QSize(32, 32));
    connect(openMangaFolderButton, &QPushButton::clicked,
            this, &StartUpWidget::openMangaFolderClicked);

    auto openMangaArchiveButton = new QPushButton(i18n("Open Manga Archive"), this);
    auto fallbackIcon = QIcon::fromTheme(u"package-x-generic"_s);
    openMangaArchiveButton->setIcon(QIcon::fromTheme(u"application-x-archive"_s, fallbackIcon));
    openMangaArchiveButton->setIconSize(QSize(32, 32));
    connect(openMangaArchiveButton, &QPushButton::clicked,
            this, &StartUpWidget::openMangaArchiveClicked);

    firstButtonsRowLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    firstButtonsRowLayout->addWidget(addMangaFolderButton);
    firstButtonsRowLayout->addWidget(openMangaFolderButton);
    firstButtonsRowLayout->addWidget(openMangaArchiveButton);
    firstButtonsRowLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    auto settingsButton = new QPushButton(i18n("Settings"), this);
    settingsButton->setIcon(QIcon::fromTheme(u"configure"_s));
    connect(settingsButton, &QPushButton::clicked,
            this, &StartUpWidget::openSettingsClicked);

    auto configureShortcutsButton = new QPushButton(i18n("Configure Shortcuts"), this);
    configureShortcutsButton->setIcon(QIcon::fromTheme(u"input-keyboard"_s));
    connect(configureShortcutsButton, &QPushButton::clicked,
            this, &StartUpWidget::openShortcutsConfigClicked);


    secondButtonsRowLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    secondButtonsRowLayout->addWidget(settingsButton);
    secondButtonsRowLayout->addWidget(configureShortcutsButton);
    secondButtonsRowLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
}

void StartUpWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);

    Q_EMIT mouseMoved(event);
}

#include "moc_startupwidget.cpp"
