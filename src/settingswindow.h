/*
 * SPDX-FileCopyrightText: 2021 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <KConfigDialog>
#include <KConfigSkeleton>

class QCheckBox;
class KColorButton;
class KEditListWidget;
class QLineEdit;
class QSpinBox;
class KUrlRequester;

class SettingsWindow : public KConfigDialog
{
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent, KConfigSkeleton *skeleton);

    QPushButton *addMangaFolderButton() const;

private:
    QPushButton *m_addMangaFolderButton{nullptr};
    QHash<QString, bool> changedSettings;
    QLineEdit *m_extractionFolder{nullptr};
    KUrlRequester *m_unrarPath{nullptr};
    QCheckBox *m_upscaleImages{nullptr};
    QSpinBox *m_maxWidth{nullptr};
    QSpinBox *m_pageSpacing{nullptr};
    KColorButton *m_backgroundColor{nullptr};
    KColorButton *m_borderColor{nullptr};
    KEditListWidget *m_mangaFolders{nullptr};
};

#endif // SETTINGSWINDOW_H
