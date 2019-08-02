/*
 * Copyright 2019 Florea Banus George <georgefb899@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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
        QStringLiteral("mangareader"),
        i18n("Manga Reader"),
        QStringLiteral("1.0.1"),
        i18n("Manga reader for local files."),
        KAboutLicense::GPL_V3,
        i18n("(c) 2019"),
        QStringLiteral(),
        QStringLiteral("https://gitlab.com/g-fb/manga-reader"),
        QStringLiteral("https://gitlab.com/g-fb/manga-reader/issues"));

    aboutData.addAuthor(
        i18n("George Florea Bănuș"),
        i18n("Developer"),
        QStringLiteral("georgefb899@gmail.org"),
        QStringLiteral("http://georgefb.com")
    );
    KAboutData::setApplicationData(aboutData);

    MainWindow *w = new MainWindow();
    w->setWindowIcon(QIcon::fromTheme("mangareader"));
    w->show();

    return app.exec();
}

