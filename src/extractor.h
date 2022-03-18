#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <QObject>

class KArchive;
class KArchiveDirectory;

class Extractor : public QObject
{
    Q_OBJECT
public:
    explicit Extractor(QObject *parent = nullptr);
    ~Extractor();

    void extractArchive();
    void extractArchiveToDrive();
    void extractArchiveToMemory();
    void extractRarArchive();
    QString extractionFolder();
    QString unrarNotFoundMessage();
    const QString &archiveFile() const;
    void setArchiveFile(const QString &archiveFile);

Q_SIGNALS:
    void started();
    void finished();
    void finishedMemory(KArchive *, const QStringList &);
    void error(const QString &);
    void progress(int);
    void unrarNotFound();

private:
    void setupTmpExtractionFolder();
    void getImagesInArchive(const QString &prefix, const KArchiveDirectory *dir);
    QString  m_archiveFile;
    QString  m_tmpFolder;
    QStringList m_entries;
};

#endif // EXTRACTOR_H
