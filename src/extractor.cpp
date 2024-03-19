/*
 * SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "extractor.h"

#include "settings.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>
#include <QTemporaryDir>

#include <KLocalizedString>
#include <KTar>
#include <KZip>
#include <QRegularExpression>
#ifdef WITH_K7ZIP
#include <K7Zip>
#endif

Extractor::Extractor(QObject *parent)
    : QObject{parent}
{

}

Extractor::~Extractor()
{
    delete m_tmpFolder;
}

void Extractor::extractArchive()
{
    KArchive *archive {nullptr};
    QMimeDatabase db;
    const QMimeType mimetype = db.mimeTypeForFile(m_archiveFile, QMimeDatabase::MatchContent);
    if (mimetype.inherits(QStringLiteral("application/x-cbz"))
            || mimetype.inherits(QStringLiteral("application/zip"))
            || mimetype.inherits(QStringLiteral("application/vnd.comicbook+zip"))) {
        archive = new KZip(m_archiveFile);
#ifdef WITH_K7ZIP
    } else if (mimetype.inherits(QStringLiteral("application/x-7z-compressed"))
               || mimetype.inherits(QStringLiteral("application/x-cb7"))) {
        archive = new K7Zip(m_archiveFile);
#endif
    } else if (mimetype.inherits(QStringLiteral("application/x-tar"))
               || mimetype.inherits(QStringLiteral("application/x-cbt"))) {
        archive = new KTar(m_archiveFile);
    } else if (mimetype.inherits(QStringLiteral("application/x-rar"))
               || mimetype.inherits(QStringLiteral("application/x-cbr"))
               || mimetype.inherits(QStringLiteral("application/vnd.rar"))
               || mimetype.inherits(QStringLiteral("application/vnd.comicbook-rar"))) {
        extractRarArchive();
        return;
    } else {
        Q_EMIT error(i18n("Could not open archive: %1", m_archiveFile));
        return;
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        delete archive;
        archive = nullptr;
        Q_EMIT error(i18n("Could not open archive: %1", m_archiveFile));
        return;
    }

    const KArchiveDirectory *directory = archive->directory();
    if (!directory) {
        delete archive;
        archive = nullptr;
        Q_EMIT error(i18n("Could not open archive: %1", m_archiveFile));
        return;
    }

    getImagesInArchive(QString(), archive->directory());
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(m_entries.begin(), m_entries.end(), collator);

    Q_EMIT finishedMemory(archive, m_entries);
    m_entries.clear();
}

void Extractor::getImagesInArchive(const QString &prefix, const KArchiveDirectory *dir)
{
    const QStringList entryList = dir->entries();
    for (const QString &file : entryList) {
        const KArchiveEntry *e = dir->entry(file);
        if (e->isDirectory()) {
            getImagesInArchive(prefix + file + QStringLiteral("/"), static_cast<const KArchiveDirectory *>(e));
        } else if (e->isFile()) {
            m_entries.append(prefix + file);
        }
    }
}

void Extractor::extractRarArchive()
{
    delete m_tmpFolder;
    m_tmpFolder = new QTemporaryDir();

    auto unrar = MangaReaderSettings::unrarPath().isEmpty()
            ? MangaReaderSettings::autoUnrarPath()
            : MangaReaderSettings::unrarPath();
    if (unrar.startsWith(u"file://"_qs)) {
#ifdef Q_OS_WIN32
        unrar.remove(0, QString(u"file:///"_qs).size());
#else
        unrar.remove(0, QString(u"file://"_qs).size());
#endif
    }
    QFileInfo fi(unrar);
    if (unrar.isEmpty() || !fi.exists()) {
        Q_EMIT unrarNotFound();
    }

    QStringList args;
    args << u"e"_qs << m_archiveFile << m_tmpFolder->path() << u"-o+"_qs;
    auto process = new QProcess();
    process->setProgram(unrar);
    process->setArguments(args);
    process->start();

    connect(process, (void (QProcess::*)(int,QProcess::ExitStatus))&QProcess::finished,
            this, &Extractor::finished);

    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QRegularExpression re(u"[0-9]+[%]"_qs);
        QRegularExpressionMatch match = re.match(QString::fromUtf8(process->readAllStandardOutput()));
        if (match.hasMatch()) {
            QString matched = match.captured(0);
            Q_EMIT progress(matched.remove(u"%"_qs).toInt());
        }
    });

    connect(process, &QProcess::errorOccurred,
            this, [=](QProcess::ProcessError err) {
        QString errorMessage;
        switch (err) {
        case QProcess::FailedToStart:
            errorMessage = u"FailedToStart"_qs;
            break;
        case QProcess::Crashed:
            errorMessage = u"Crashed"_qs;
            break;
        case QProcess::Timedout:
            errorMessage = u"Timedout"_qs;
            break;
        case QProcess::WriteError:
            errorMessage = u"WriteError"_qs;
            break;
        case QProcess::ReadError:
            errorMessage = u"ReadError"_qs;
            break;
        default:
            errorMessage = u"UnknownError"_qs;
        }
        Q_EMIT error(i18n("Error: Could not open the archive. %1", errorMessage));
    });

    return;
}

const QString &Extractor::archiveFile() const
{
    return m_archiveFile;
}

void Extractor::setArchiveFile(const QString &archiveFile)
{
    m_archiveFile = archiveFile;
}

QString Extractor::extractionFolder()
{
    return m_tmpFolder->path();
}

QString Extractor::unrarNotFoundMessage()
{
#ifdef Q_OS_WIN32
    return QStringLiteral("UnRAR executable was not found.\n"
                          "It can be installed through WinRAR or independent. "
                          "When installed with WinRAR just restarting the application "
                          "should be enough to find the executable.\n"
                          "If installed independently you have to manually "
                          "set the path to the UnRAR executable in the settings.");
#else
    return QStringLiteral("UnRAR executable was not found.\n"
                          "Install the unrar package and restart the application, "
                          "unrar should be picked up automatically.\n"
                          "If unrar is still not found you can set "
                          "the path to the unrar executable manually in the settings.");
#endif
}

#include "moc_extractor.cpp"
