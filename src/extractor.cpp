#include "extractor.h"

#include <KLocalizedString>

#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>

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
    switch (extractionType()) {
    case ExtractionType::Drive: {
        extractArchiveToDrive();
        break;
    }
    case ExtractionType::Memory: {
        extractArchiveToMemory();
        break;
    }
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
    auto extractor = new MemoryExtractor(m_archiveFile);
    extractor->setCalculateProgress(true);
    extractor->getInfo();
    extractor->start();

    connect(extractor, &MemoryExtractor::started, this, &Extractor::started);
    QObject::connect(extractor, &MemoryExtractor::finished, this, [=](MemoryExtractorOutput *data) {
        const QVector<MemoryFile> files = data->getFiles();

        for(const auto &file : files) {
            auto fileInfo = file.fileInformation();
            m_memoryImages.emplace(fileInfo.value("FileName").toString(), file.buffer()->data());
        }

        Q_EMIT finishedMemory(m_memoryImages);
        m_memoryImages.clear();
        data->deleteLater();
        extractor->deleteLater();
    });

    connect(extractor, &DiskExtractor::progress,
            this, [=](QString, int, int, qint64 bytesProcessed, qint64 bytesTotal) {
        Q_EMIT progress(bytesProcessed * 100 / bytesTotal);
    });

    QObject::connect(extractor, &MemoryExtractor::error, this, [=](short err) {
        QMimeDatabase mimeDb;
        QString mimeName = mimeDb.mimeTypeForFile(m_archiveFile).name();
        if (mimeName == "application/vnd.comicbook-rar" || mimeName == "application/vnd.rar") {
            extractRarArchive();
            return;
        }

        QString errorMessage = i18n("Error %1: Could not extract the archive.\n%2",
                                    QString::number(err),
                                    QArchive::errorCodeToString(err));
        Q_EMIT error(errorMessage);
    });

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

Extractor::ExtractionType Extractor::extractionType() const
{
    return m_extractionType;
}

void Extractor::setExtractionType(ExtractionType type)
{
    m_extractionType = type;
}
