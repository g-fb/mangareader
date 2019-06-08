/*
 * Copyright 2019 George Florea Banus
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KSharedConfig>
#include <KXmlGuiWindow>

class QProgressBar;
class View;
class Worker;

class MainWindow : public KXmlGuiWindow
{
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void init();
    void setupActions();
    void addMangaFolder();
    void loadImages(const QModelIndex &index);

    QProgressBar *m_progressBar;
    KSharedConfig::Ptr m_config;
    QStringList m_images;
    QString m_currentManga;
    View *m_view;
    Worker *m_worker;
    QThread *m_thread;
};

#endif // MAINWINDOW_H
