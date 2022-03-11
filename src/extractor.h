#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <QObject>

class Extractor : public QObject
{
    Q_OBJECT
public:
    explicit Extractor(QObject *parent = nullptr);
    ~Extractor();

    void extractArchive();
    void extractRarArchive();
    void setArchiveFile(const QString &archiveFile);
    QString extractionFolder();
    QString unrarNotFoundMessage();


signals:
    void started();
    void finished();
    void error(const QString &);
    void progress(int);
    void unrarNotFound();

private:
    void setupTmpExtractionFolder();
    QString  m_archiveFile;
    QString  m_tmpFolder;
};

#endif // EXTRACTOR_H
