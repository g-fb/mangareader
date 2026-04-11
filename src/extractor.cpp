/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "extractor.h"

#include <QCollator>
#include <QFileInfo>
#include <QImage>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <KArchive>
#include <KLocalizedString>
#include <KTar>
#include <KZip>
#include <QImageReader>
#ifdef WITH_K7ZIP
#include <K7Zip>
#endif

#include "settings.h"

using namespace Qt::StringLiterals;

Extractor::Extractor(QObject *parent)
    : QObject{parent}
{
    setObjectName(u"Extractor"_s);
}

Extractor::~Extractor()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished();
    }
}

bool Extractor::open(const QString &path)
{
    m_archive.reset();

    QMimeDatabase db;
    m_archiveFile = path;
    m_archiveMimeType = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);

    if (isZip()) {
        m_archive = std::make_unique<KZip>(path);
#ifdef WITH_K7ZIP
    } else if (is7Z()) {
        m_archive = std::make_unique<K7Zip>(path);
#endif
    } else if (isTar()) {
        m_archive = std::make_unique<KTar>(path);
    } else if (isRar()) {
        extractRarArchive();
        return true;
    } else {
        return false;
    }

    if (!m_archive->open(QIODevice::ReadOnly)) {
        qDebug() << i18n("Could not open archive: %1", m_archiveFile) << "\n" << m_archive->errorString();
        return false;
    }

    return true;
}

QList<Image> Extractor::filesList()
{
    if (m_archiveFile.isEmpty()) {
        qDebug() << i18n("No archive file set");
        return {};
    }

    if (m_archive == nullptr) {
        qDebug() << i18n("Unknown archive: %1", m_archiveFile);
        return {};
    }

    const KArchiveDirectory *directory = m_archive->directory();
    if (!directory) {
        qDebug() << i18n("Could not open archive: %1", m_archiveFile);
        return {};
    }

    auto files = getFiles(QString(), m_archive->directory());
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(files.begin(), files.end(), [&collator](const Image &a, const Image &b) {
        return collator.compare(a.path, b.path) < 0;
    });

    return files;
}

void Extractor::extractRarArchive()
{
    m_tmpFolder = std::make_unique<QTemporaryDir>();
    auto unrar = MangaReaderSettings::unrarPath().isEmpty()
                ? MangaReaderSettings::autoUnrarPath()
                : MangaReaderSettings::unrarPath();
    if (unrar.isEmpty()) {
        return;
    }

    QStringList args;
    args << u"e"_s << m_archiveFile << m_tmpFolder->path() << u"-o+"_s;
    m_process = std::make_unique<QProcess>();
    m_process->setProgram(unrar);
    m_process->setArguments(args);
    m_process->start();

    connect(m_process.get(), &QProcess::started, this, &Extractor::started);
    connect(m_process.get(), &QProcess::finished, this, &Extractor::finishedRar);

    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, [this]() {
        static QRegularExpression re(u"(\\d+)%"_s);
        while (m_process->canReadLine()) {
            QString line = QString::fromUtf8(m_process->readLine());
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                bool ok = false;
                int value = match.captured(1).toInt(&ok);
                if (ok) {
                    Q_EMIT progress(value);
                }
            }
        }
    });

    connect(m_process.get(), &QProcess::errorOccurred, this, [](QProcess::ProcessError err) {
        QString errorMessage;
        switch (err) {
        case QProcess::FailedToStart:
            errorMessage = u"FailedToStart"_s;
            break;
        case QProcess::Crashed:
            errorMessage = u"Crashed"_s;
            break;
        case QProcess::Timedout:
            errorMessage = u"Timedout"_s;
            break;
        case QProcess::WriteError:
            errorMessage = u"WriteError"_s;
            break;
        case QProcess::ReadError:
            errorMessage = u"ReadError"_s;
            break;
        default:
            errorMessage = u"UnknownError"_s;
        }
        qDebug() << i18n("Error: Could not open the archive. %1", errorMessage);
    });

    return;
}

bool Extractor::extractFile(const QString &name, const QString &destination)
{
    QFileInfo file{destination};
    if (!file.isDir() || !file.exists()) {
        return false;
    }

    auto archiveFile = m_archive->directory()->file(name);
    auto result = archiveFile->copyTo(destination);

    return result;
}

QSize Extractor::imageSize(const QString &name)
{
    QFileInfo fi(name);
    std::unique_ptr<QIODevice> dev;
    QImageReader imageReader;
    imageReader.setAutoTransform(true);
    imageReader.setFormat(fi.suffix().toUtf8());

    const KArchiveFile *entry = m_archive->directory()->file(name);
    if (entry == nullptr) {
        qDebug() << "KArchiveFile is nullptr:" << name;
        return {};
    }

    dev.reset(entry->createDevice());
    if (dev.get() == nullptr) {
        qDebug() << "QIODevice is nullptr:" << name;
        return {};
    }

    imageReader.setDevice(dev.get());
    if (!imageReader.canRead()) {
        // in case of wrong extension remove the set format and try again
        imageReader.setFormat({});
        imageReader.setDevice(dev.get());

        if (!imageReader.canRead()) {
            qDebug() << "QImageReader can't read file:" << name << imageReader.errorString();
            return {};
        }
    }

    auto pageSize = imageReader.size();
    if (imageReader.transformation() & QImageIOHandler::TransformationRotate90) {
        pageSize.transpose();
    }
    return pageSize;
}

QByteArray Extractor::getFileData(const QString &name)
{
    auto archiveFile = m_archive->directory()->file(name);

    if (archiveFile == nullptr) {
        qDebug() << "archiveFile is nullptr" << name << m_archiveFile;
        return {};
    }

    return archiveFile->data();
}

QImage Extractor::extractFirstImage()
{
    if (m_archiveFile.isEmpty()) {
        qDebug() << i18n("No archive file set");
        return QImage();
    }
    qDebug() << "Extracting first image:" << m_archiveFile;
    // archive is passed to MangaLoader and deleted there
    if (m_archive == nullptr && isRar()) {
        return rarExtractFirstImage();
    }
    if (m_archive == nullptr) {
        qDebug() << i18n("Unknown archive: %1", m_archiveFile);
        return QImage();
    }

    const KArchiveDirectory *directory = m_archive->directory();
    if (!directory) {
        qDebug() << i18n("Could not open archive: %1", m_archiveFile);
        return QImage();
    }

    auto files = getFiles(QString(), m_archive->directory());
    if (files.isEmpty()) {
        return QImage();
    }

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(files.begin(), files.end(), [&collator](const Image &a, const Image &b) {
        return collator.compare(a.path, b.path) < 0;
    });

    auto *file = directory->file(files.first().path);

    return QImage::fromData(file->data());
}

QImage Extractor::rarExtractFirstImage()
{
    m_tmpFolder = std::make_unique<QTemporaryDir>();
    auto unrar = QStandardPaths::findExecutable(u"unrar"_s);
    if (unrar.isEmpty()) {
        return QImage();
    }

    // get list od files in the archive
    QStringList args;
    args << u"lb"_s << m_archiveFile;
    QProcess process;
    process.setProgram(unrar);
    process.setArguments(args);
    process.start();
    process.waitForFinished();
    auto files = QString::fromLocal8Bit(process.readAllStandardOutput()).split(u"\n"_s);
    process.terminate();

    if (files.isEmpty()) {
        return QImage();
    }

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(files.begin(), files.end(), collator);

    auto images = filterImages(files);

    if (images.isEmpty()) {
        return QImage();
    }

    args.clear();
    // extract file from archive
    args << u"x"_s << u"-n%1"_s.arg(images.first()) << m_archiveFile << m_tmpFolder->path();
    process.setProgram(unrar);
    process.setArguments(args);
    process.start();
    process.waitForFinished();

    return QImage(m_tmpFolder->path() + u"/"_s + images.first());
}

QStringList Extractor::filterImages(const QStringList &files)
{
    QStringList images;
    // clang-format off
    static const QStringList extensions{
        u".jpg"_s, u".jpeg"_s, u".png"_s, u".gif"_s,
        u".jxl"_s, u".webp"_s, u".heif"_s, u".avif"_s
    };
    for (const auto &file : files) {
        if (file.startsWith(u".DS_Store"_s, Qt::CaseInsensitive)) {
            continue;
        }

        for (const auto &extension : extensions) {
            if (file.endsWith(extension, Qt::CaseInsensitive)) {
                images.append(file);
            }
        }
    }
    // clang-format on
    return images;
}

QList<Image> Extractor::getFiles(const QString &prefix, const KArchiveDirectory *dir)
{
    QList<Image> files;
    const QStringList entryList = dir->entries();
    for (const QString &file : entryList) {
        const KArchiveEntry *e = dir->entry(file);
        if (e->isDirectory()) {
            if (e->name() == u"__MACOSX") {
                continue;
            }
            auto _files = getFiles(prefix + file + u"/"_s, static_cast<const KArchiveDirectory *>(e));
            files.append(_files);
        } else if (e->isFile()) {
            QString imagePath = prefix + file;
            QSize pageSize = imageSize(imagePath);
            if (pageSize.isValid()) {
                files.append({imagePath, pageSize});
            }
        }
    }

    return files;
}

QString Extractor::archiveFile() const
{
    return m_archiveFile;
}

// clang-format off
bool Extractor::isZip()
{
    return m_archiveMimeType.inherits(u"application/x-cbz"_s)
    || m_archiveMimeType.inherits(u"application/zip"_s)
        || m_archiveMimeType.inherits(u"application/vnd.comicbook+zip"_s);
}

bool Extractor::isRar()
{
    return m_archiveMimeType.inherits(u"application/x-cbr"_s)
    || m_archiveMimeType.inherits(u"application/x-rar"_s)
        || m_archiveMimeType.inherits(u"application/vnd.rar"_s)
        || m_archiveMimeType.inherits(u"application/vnd.comicbook-rar"_s);
}

bool Extractor::isTar()
{
    return m_archiveMimeType.inherits(u"application/x-cbt"_s)
    || m_archiveMimeType.inherits(u"application/x-tar"_s);
}

bool Extractor::is7Z()
{
    return m_archiveMimeType.inherits(u"application/x-cb7"_s)
    || m_archiveMimeType.inherits(u"application/x-7z-compressed"_s);
}
// clang-format on

QString Extractor::extractionFolder()
{
    return m_tmpFolder->path();
}

QString Extractor::unrarNotFoundMessage()
{
#ifdef Q_OS_WIN32
    return u"UnRAR executable was not found.\n"
           "It can be installed through WinRAR or independent. "
           "When installed with WinRAR just restarting the application "
           "should be enough to find the executable.\n"
           "If installed independently you have to manually "
           "set the path to the UnRAR executable in the settings."_s;
#else
    return u"UnRAR executable was not found.\n"
           "Install the unrar package and restart the application, "
           "unrar should be picked up automatically.\n"
           "If unrar is still not found you can set "
           "the path to the unrar executable manually in the settings."_s;
#endif
}

#include "moc_extractor.cpp"
