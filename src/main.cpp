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

#include "mainwindow.h"

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("mangareader");
    KAboutData aboutData(
        // The program name used internally. (componentName)
        QStringLiteral("mangareader"),
        // A displayable program name string. (displayName)
        i18n("App Name"),
        // The program version string. (version)
        QStringLiteral("1.0.0"),
        // Short description of what the app does. (shortDescription)
        i18n("App short description"),
        // The license this code is released under
        KAboutLicense::GPL,
        // Copyright Statement (copyrightStatement = QString())
        i18n("(c) 2019"),
        // Optional text shown in the About box.
        // Can contain any information desired. (otherText)
        i18n("About text..."),
        // The program homepage string. (homePageAddress = QString())
        QStringLiteral("http://example.com"),
        // The bug report email address
        // (bugsEmailAddress = QLatin1String("submit@bugs.kde.org")
        QStringLiteral("bugs@example.com"));

    aboutData.addAuthor(
        i18n("Author Name"),
        i18n("Developer"),
        QStringLiteral("name@example.org"),
        QStringLiteral("http://example.com")
    );
    KAboutData::setApplicationData(aboutData);

    MainWindow *w = new MainWindow();
    w->setWindowIcon(QIcon::fromTheme("mangareader"));
    w->show();

    return app.exec();
}

