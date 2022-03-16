#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <QObject>

using MemoryImages = std::map<QString, QByteArray>;

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
    void finishedMemory(const MemoryImages &);
    void error(const QString &);
    void progress(int);
    void unrarNotFound();

private:
    void setupTmpExtractionFolder();
    QString  m_archiveFile;
    QString  m_tmpFolder;
    MemoryImages m_memoryImages;
};

#endif // EXTRACTOR_H