/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QUrl>

#include <KAboutData>
#include <KLocalizedString>

#include "mainwindow.h"
#include "mangareader-version.h"

int main(int argc, char *argv[])
{
    /**
     * enable dark mode for title bar on Windows
     */
#if defined(Q_OS_WIN)
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "windows:darkmode=1");
    }
#endif

    QApplication app(argc, argv);

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    QApplication::setStyle(QStringLiteral("Breeze"));
#endif

    KLocalizedString::setApplicationDomain("mangareader");

    KAboutData aboutData;
    aboutData.setDisplayName(i18n("Manga Reader"));
    aboutData.setComponentName(QStringLiteral("mangareader"));
    aboutData.setVersion(MANGAREADER_VERSION_STRING);
    aboutData.setShortDescription(i18n("Manga reader for local files."));
    aboutData.setLicense(KAboutLicense::GPL_V3);
    aboutData.setCopyrightStatement(i18n("(c) 2019-2022"));
    aboutData.setHomepage(QStringLiteral("https://github.com/g-fb/mangareader"));
    aboutData.setBugAddress(QStringLiteral("https://github.com/g-fb/mangareader/issues").toUtf8());
    aboutData.addAuthor(
        i18n("George Florea Bănuș"),
        i18n("Developer"),
        QStringLiteral("georgefb899@gmail.org"),
        QStringLiteral("https://georgefb.com")
    );
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("file"), i18n("File or folder to open"));
    parser.process(app);
    aboutData.setupCommandLine(&parser);
    aboutData.processCommandLine(&parser);

    const QStringList args = parser.positionalArguments();

    auto w = new MainWindow();
    w->setWindowIcon(QIcon::fromTheme(u"mangareader"_qs));
    w->show();

    if (args.count() > 0 && !args.at(0).isEmpty()) {
        QUrl url = QUrl::fromUserInput(args.at(0), QDir::currentPath());
        w->setCurrentPath(url.toLocalFile());
        w->loadImages(url.toLocalFile());
    }

    return QApplication::exec();
}

