#include "extractor.h"


#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>

#include <KLocalizedString>
#include <KTar>
#include <KZip>
#ifdef WITH_K7ZIP
#include <K7Zip>
#endif

#include <QArchive>
#include <settings.h>

using QArchive::DiskExtractor;
using QArchive::MemoryExtractor;
using QArchive::MemoryExtractorOutput;
using QArchive::MemoryFile;

Extractor::Extractor(QObject *parent)
    : QObject{parent}
{

}

Extractor::~Extractor()
{
    QFileInfo file(m_tmpFolder);
    if (file.exists() && file.isDir() && file.isWritable()) {
        QDir dir(m_tmpFolder);
        dir.removeRecursively();
    }
}

void Extractor::setupTmpExtractionFolder()
{
    QFileInfo extractionFolder(MangaReaderSettings::extractionFolder());
    QFileInfo archivePathInfo(m_archiveFile);
    if (!extractionFolder.exists() || !extractionFolder.isWritable()) {
        Q_EMIT error(i18n("Extraction folder does not exist or is not writable.\n%1",
                          MangaReaderSettings::extractionFolder()));
        return;
    }
    // delete previous extracted folder
    QFileInfo file(m_tmpFolder);
    if (file.exists() && file.isDir() && file.isWritable()) {
        QDir dir(m_tmpFolder);
        dir.removeRecursively();
    }
    m_tmpFolder = extractionFolder.absoluteFilePath() + "/" + archivePathInfo.baseName();
    QDir dir(m_tmpFolder);
    if (!dir.exists()) {
        dir.mkdir(m_tmpFolder);
    }
}

void Extractor::extractArchive()
{
    if (MangaReaderSettings::useMemoryExtraction()) {
        extractArchiveToMemory();
    } else {
        extractArchiveToDrive();
    }
}

void Extractor::extractArchiveToDrive()
{
    setupTmpExtractionFolder();
    auto extractor = new DiskExtractor(this);
    extractor->setArchive(m_archiveFile);
    extractor->setOutputDirectory(m_tmpFolder);
    extractor->setCalculateProgress(true);
    extractor->start();

    connect(extractor, &DiskExtractor::started, this, &Extractor::started);
    connect(extractor, &DiskExtractor::finished, this, &Extractor::finished);

    connect(extractor, &DiskExtractor::progress,
            this, [=](QString, int, int, qint64 bytesProcessed, qint64 bytesTotal) {
        Q_EMIT progress(bytesProcessed * 100 / bytesTotal);
    });

    connect(extractor, &DiskExtractor::error, this, [=](short err) {
        QMimeDatabase mimeDB;
        QMimeType type = mimeDB.mimeTypeForFile(m_archiveFile);
        if (type.name() == "application/vnd.comicbook-rar" || type.name() == "application/vnd.rar") {
            extractRarArchive();
            return;
        }

        QString errorMessage = i18n("Error %1: Could not extract the archive.\n%2",
                                    QString::number(err),
                                    QArchive::errorCodeToString(err));
        Q_EMIT error(errorMessage);
    });
}

void Extractor::extractArchiveToMemory()
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
        setupTmpExtractionFolder();
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
    auto unrar = MangaReaderSettings::unrarPath().isEmpty()
            ? MangaReaderSettings::autoUnrarPath()
            : MangaReaderSettings::unrarPath();
    if (unrar.startsWith("file://")) {
#ifdef Q_OS_WIN32
        unrar.remove(0, QString("file:///").size());
#else
        unrar.remove(0, QString("file://").size());
#endif
    }
    QFileInfo fi(unrar);
    if (unrar.isEmpty() || !fi.exists()) {
        Q_EMIT unrarNotFound();
    }

    QStringList args;
    args << "e" << m_archiveFile << m_tmpFolder << "-o+";
    auto process = new QProcess();
    process->setProgram(unrar);
    process->setArguments(args);
    process->start();

    connect(process, &QProcess::started,
            this, &Extractor::started);
    connect(process, (void (QProcess::*)(int,QProcess::ExitStatus))&QProcess::finished,
            this, &Extractor::finished);

    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QRegularExpression re("[0-9]+[%]");
        QRegularExpressionMatch match = re.match(process->readAllStandardOutput());
        if (match.hasMatch()) {
            QString matched = match.captured(0);
            Q_EMIT progress(matched.remove("%").toInt());
        }
    });

    connect(process, &QProcess::errorOccurred,
            this, [=](QProcess::ProcessError err) {
        QString errorMessage;
        switch (err) {
        case QProcess::FailedToStart:
            errorMessage = "FailedToStart";
            break;
        case QProcess::Crashed:
            errorMessage = "Crashed";
            break;
        case QProcess::Timedout:
            errorMessage = "Timedout";
            break;
        case QProcess::WriteError:
            errorMessage = "WriteError";
            break;
        case QProcess::ReadError:
            errorMessage = "ReadError";
            break;
        default:
            errorMessage = "UnknownError";
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
    return m_tmpFolder;
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
