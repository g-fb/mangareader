#include "settingswindow.h"

#include "_debug.h"
#include "settings.h"

#include <KColorButton>
#include <KEditListWidget>
#include <KLocalizedString>

#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

SettingsWindow::SettingsWindow(QWidget *parent, KConfigSkeleton *skeleton)
    : KConfigDialog(parent, QStringLiteral("settings"), skeleton)
{
    setFaceType(KPageDialog::Plain);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(i18n("Settings"));
    resize(750, 600);

    auto formWidget = new QWidget(this);
    auto formLayout = new QFormLayout(formWidget);

    // folder extraction
    auto extractionFolderWidget = new QWidget(this);
    auto extractionFolderLayout = new QHBoxLayout(extractionFolderWidget);
    m_extractionFolder = new QLineEdit(this);
    m_extractionFolder->setObjectName(QStringLiteral("kcfg_ExtractionFolder"));
    m_extractionFolder->setText(MangaReaderSettings::extractionFolder());

    auto selectExtractionFolderButton = new QPushButton(extractionFolderWidget);
    selectExtractionFolderButton->setIcon(QIcon::fromTheme("folder"));
    connect(selectExtractionFolderButton, &QPushButton::clicked, this, [=]() {
        QString path = QFileDialog::getExistingDirectory(
                    this, i18n("Select extraction folder"),
                    MangaReaderSettings::extractionFolder());
        if (path.isEmpty()) {
            return;
        }
        m_extractionFolder->setText(path);
    });

    extractionFolderLayout->addWidget(m_extractionFolder);
    extractionFolderLayout->setMargin(0);
    extractionFolderLayout->addWidget(selectExtractionFolderButton);
    formLayout->addRow(i18n("Extraction folder"), extractionFolderWidget);
    // end folder extraction


    // upscale images
    m_upscaleImages = new QCheckBox(this);
    m_upscaleImages->setObjectName(QStringLiteral("kcfg_UpScale"));
    m_upscaleImages->setText(i18n("Upscale images"));
    m_upscaleImages->setChecked(MangaReaderSettings::upScale());
    formLayout->addRow(QLatin1String(), m_upscaleImages);
    // end upscale images


    // max page width
    m_maxWidth = new QSpinBox(this);
    m_maxWidth->setObjectName(QStringLiteral("kcfg_MaxWidth"));
    m_maxWidth->setMinimum(200);
    m_maxWidth->setMaximum(9999);
    m_maxWidth->setValue(MangaReaderSettings::maxWidth());
    formLayout->addRow(i18n("Maximum page width"), m_maxWidth);
    // end max page width


    // page spacing
    m_pageSpacing = new QSpinBox(this);
    m_pageSpacing->setObjectName(QStringLiteral("kcfg_PageSpacing"));
    m_pageSpacing->setMinimum(0);
    m_pageSpacing->setMaximum(999);
    m_pageSpacing->setValue(MangaReaderSettings::pageSpacing());
    formLayout->addRow(i18n("Page spacing"), m_pageSpacing);
    // end page spacing


    // background color
    m_backgroundColor = new KColorButton(this);
    m_backgroundColor->setObjectName(QStringLiteral("kcfg_BackgroundColor"));
    m_backgroundColor->setColor(MangaReaderSettings::backgroundColor());
    formLayout->addRow(i18n("Background color"), m_backgroundColor);
    // end background color


    // border color
    m_borderColor = new KColorButton(this);
    m_borderColor->setObjectName(QStringLiteral("kcfg_BorderColor"));
    m_borderColor->setColor(MangaReaderSettings::borderColor());
    formLayout->addRow(i18n("Border color"), m_borderColor);
    // end border color


    // manga folders
    m_mangaFolders = new KEditListWidget(this);
    m_mangaFolders->setObjectName(QStringLiteral("kcfg_MangaFolders"));
    m_mangaFolders->setItems(MangaReaderSettings::mangaFolders());
    formLayout->addRow(i18n("Manga folders"), m_mangaFolders);

    m_addMangaFolderButton = new QPushButton(i18n("Select and add Manga Folder"));
    m_addMangaFolderButton->setIcon(QIcon::fromTheme("folder-add"));
    connect(m_addMangaFolderButton, &QPushButton::clicked, this, [=]() {
        QString path = QFileDialog::getExistingDirectory(this, i18n("Select manga folder"));
        if (path.isEmpty()) {
            return;
        }
        m_mangaFolders->insertItem(path);
        emit m_mangaFolders->changed();
    });
    auto widget = new QWidget();
    auto hLayout = new QHBoxLayout(widget);
    hLayout->setMargin(0);
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
