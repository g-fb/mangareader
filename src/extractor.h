#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <QMimeType>
#include <QObject>
#include <QTemporaryDir>

#include <KArchive>

#include "image.h"

class KArchiveDirectory;

class Extractor : public QObject
{
    Q_OBJECT

public:
    explicit Extractor(QObject *parent = nullptr);

    bool open(const QString &path);
    QList<Image> filesList();
    void extractRarArchive();
    /**
     * Extracts `name` from the archive to `destination`
     */
    bool extractFile(const QString &name, const QString &destination);
    /**
     * Gets the size of the file `name` if it's an image
     */
    QSize imageSize(const QString &name);
    /**
     * Gets the data of a file
     */
    QByteArray getFileData(const QString &name);
    /*
     * Extracts the first image from an archive, before image is extracted
     * files are natural sorted and filtered to have only images
     */
    QImage extractFirstImage();
    /*
     * Extracts the first image from a rar archive, before image is extracted
     * files are natural sorted and filtered to have only images
     */
    QImage rarExtractFirstImage();
    /*
     * Takes all files from an archive and returns only supported images
     */
    QStringList filterImages(const QStringList &files);
    QString extractionFolder();
    QString unrarNotFoundMessage();

    bool isZip();
    bool isRar();
    bool isTar();
    bool is7Z();

    QString archiveFile() const;

Q_SIGNALS:
    void started();
    void finishedRar();
    void progress(int);
    void unrarNotFound();

private:
    QList<Image> getFiles(const QString &prefix, const KArchiveDirectory *dir);
    QString m_archiveFile;
    std::unique_ptr<KArchive> m_archive;
    std::unique_ptr<QTemporaryDir> m_tmpFolder;
    QMimeType m_archiveMimeType;
};

#endif // EXTRACTOR_H
