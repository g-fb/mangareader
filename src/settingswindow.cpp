/*
 * SPDX-FileCopyrightText: 2021 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "settingswindow.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include <KColorButton>
#include <KEditListWidget>
#include <KLocalizedString>
#include <KUrlRequester>

#include "settings.h"

using namespace Qt::StringLiterals;

SettingsWindow::SettingsWindow(QWidget *parent, KConfigSkeleton *skeleton)
    : KConfigDialog(parent, QStringLiteral("settings"), skeleton)
{
    setFaceType(KPageDialog::Plain);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(i18n("Settings"));
    resize(800, 700);

    auto formWidget = new QWidget(this);
    auto formLayout = new QFormLayout(formWidget);


    // unrar
#ifdef Q_OS_WIN32
    auto autoUnrarText = MangaReaderSettings::autoUnrarPath().isEmpty()
            ? i18n("UnRAR executable was not found.\n"
                   "It can be installed through WinRAR or independent. "
                   "When installed with WinRAR just restarting the application "
                   "should be enough to find the executable.\n"
                   "If installed independently you have to manually "
                   "set the path to the UnRAR executable bellow.")
            : MangaReaderSettings::autoUnrarPath();
#else
    auto autoUnrarText = MangaReaderSettings::autoUnrarPath().isEmpty()
            ? i18n("UnRAR executable was not found.\n"
                   "Install the unrar package and restart the application, "
                   "unrar should be picked up automatically.\n"
                   "If unrar is still not found set the path to the unrar executable manually bellow.")
            : MangaReaderSettings::autoUnrarPath();
#endif
    auto autoUnrarLabel = new QLabel(this);
    autoUnrarLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    autoUnrarLabel->setWordWrap(true);
    autoUnrarLabel->setText(autoUnrarText);
    formLayout->addRow(i18n("Unrar path"), autoUnrarLabel);

    m_unrarPath = new KUrlRequester(this);
    m_unrarPath->setObjectName(QStringLiteral("kcfg_UnrarPath"));
    m_unrarPath->setPlaceholderText(i18n("Manual unrar path..."));
    m_unrarPath->setToolTip(i18n("Path to the unrar executable. "
                                         "The executable is needed to extract .rar and .cbr files."));
    formLayout->addRow(QString(), m_unrarPath);

    auto unrarPathInfoLabel = new QLabel(this);
    unrarPathInfoLabel->setText(i18n("User set path has priority over the auto detected one."));
    formLayout->addRow(QString(), unrarPathInfoLabel);
    formLayout->addItem(new QSpacerItem(1, 6, QSizePolicy::Fixed, QSizePolicy::Fixed));
    // end unrar


    // start in fullscreen mode
    m_fullscreenStartup = new QCheckBox(this);
    m_fullscreenStartup->setObjectName(QStringLiteral("kcfg_FullscreenOnStartup"));
    m_fullscreenStartup->setText(i18n("Start in fullscreen mode"));
    m_fullscreenStartup->setChecked(MangaReaderSettings::fullscreenOnStartup());
    m_fullscreenStartup->setToolTip(i18n("Enter fullscreen mode when the application starts."));
    formLayout->addRow(QLatin1String(), m_fullscreenStartup);
    // end start in fullscreen mode


    // upscale images
    m_upscaleImages = new QCheckBox(this);
    m_upscaleImages->setObjectName(QStringLiteral("kcfg_UpScale"));
    m_upscaleImages->setText(i18n("Upscale images"));
    m_upscaleImages->setChecked(MangaReaderSettings::upScale());
    m_upscaleImages->setToolTip(i18n("When checked images are resized even if the the new size is bigger than the original size."));
    formLayout->addRow(QLatin1String(), m_upscaleImages);
    // end upscale images


    // resize timer
    auto resizeTimer = new QCheckBox(this);
    resizeTimer->setObjectName(QStringLiteral("kcfg_UseResizeTimer"));
    resizeTimer->setText(i18n("Delay image resizing"));
    resizeTimer->setChecked(MangaReaderSettings::useResizeTimer());
    resizeTimer->setToolTip(i18n("When checked image resizing is delayed by 100 miliseconds\nto prevent unnecesary resizing, possibly causing performance issues."));
    formLayout->addRow(QLatin1String(), resizeTimer);
    // end resize timer


    // max page width
    m_maxWidth = new QSpinBox(this);
    m_maxWidth->setObjectName(QStringLiteral("kcfg_MaxWidth"));
    m_maxWidth->setMinimum(200);
    m_maxWidth->setMaximum(9999);
    m_maxWidth->setValue(MangaReaderSettings::maxWidth());
    m_maxWidth->setToolTip(i18n("Maximum width a page/image can have. Only when fit width is enabled."));
    formLayout->addRow(i18n("Maximum page width"), m_maxWidth);
    // end max page width


    // page spacing
    m_pageSpacing = new QSpinBox(this);
    m_pageSpacing->setObjectName(QStringLiteral("kcfg_PageSpacing"));
    m_pageSpacing->setMinimum(0);
    m_pageSpacing->setMaximum(999);
    m_pageSpacing->setValue(MangaReaderSettings::pageSpacing());
    m_pageSpacing->setToolTip(i18n("Vertical distance between pages/images."));
    formLayout->addRow(i18n("Page spacing"), m_pageSpacing);
    // end page spacing


    // custom colors
    auto useCustomBackgroundColor = new QCheckBox(this);
    useCustomBackgroundColor->setObjectName(QStringLiteral("kcfg_UseCustomBackgroundColor"));
    useCustomBackgroundColor->setText(i18n("Use custom background color"));
    useCustomBackgroundColor->setChecked(MangaReaderSettings::useCustomBackgroundColor());
    useCustomBackgroundColor->setToolTip(i18n("When unchecked the background uses the system color."));
    connect(useCustomBackgroundColor, &QCheckBox::checkStateChanged, this, [=]() {
        m_backgroundColor->setEnabled(useCustomBackgroundColor->isChecked());
    });
    formLayout->addRow(QLatin1String(), useCustomBackgroundColor);
    // end custom colors


    // background color
    m_backgroundColor = new KColorButton(this);
    m_backgroundColor->setObjectName(QStringLiteral("kcfg_BackgroundColor"));
    m_backgroundColor->setColor(MangaReaderSettings::backgroundColor());
    m_backgroundColor->setAlphaChannelEnabled(true);
    m_backgroundColor->setEnabled(MangaReaderSettings::useCustomBackgroundColor());
    m_backgroundColor->setToolTip(i18n("Set a custom background for the view."));
    formLayout->addRow(i18n("Background color"), m_backgroundColor);
    // end background color


    // border color
    m_borderColor = new KColorButton(this);
    m_borderColor->setObjectName(QStringLiteral("kcfg_BorderColor"));
    m_borderColor->setColor(MangaReaderSettings::borderColor());
    m_borderColor->setToolTip(i18n("Set a custom color for the page/image border.\nTo disable the border set its alpha channel to 0."));
    m_borderColor->setAlphaChannelEnabled(true);
    formLayout->addRow(i18n("Border color"), m_borderColor);
    // end border color


    // manga folders
    m_mangaFolders = new KEditListWidget(this);
    m_mangaFolders->setObjectName(QStringLiteral("kcfg_MangaFolders"));
    m_mangaFolders->setItems(MangaReaderSettings::mangaFolders());
    m_mangaFolders->setToolTip(i18n("These folders can be loaded in the tree view."));
    formLayout->addRow(i18n("Manga folders"), m_mangaFolders);

    m_addMangaFolderButton = new QPushButton(i18n("Select and add manga folder"));
    m_addMangaFolderButton->setIcon(QIcon::fromTheme(u"folder-add"_s));
    connect(m_addMangaFolderButton, &QPushButton::clicked, this, [=]() {
        QString path = QFileDialog::getExistingDirectory(this, i18n("Select manga folder"));
        if (path.isEmpty()) {
            return;
        }
        m_mangaFolders->insertItem(path);
        Q_EMIT m_mangaFolders->changed();
    });
    auto widget = new QWidget();
    auto hLayout = new QHBoxLayout(widget);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(m_addMangaFolderButton);
    hLayout->addStretch(1);
    m_mangaFolders->layout()->addWidget(widget);
    // end manga folders

    addPage(formWidget, i18n("General"), QStringLiteral("okular"), i18n("General Options"));
}

QPushButton *SettingsWindow::addMangaFolderButton() const
{
    return m_addMangaFolderButton;
}

#include "moc_settingswindow.cpp"
