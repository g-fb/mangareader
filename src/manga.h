#ifndef MANGA_H
#define MANGA_H

#include <QFuture>
#include <QMimeType>
#include <QMutex>
#include <QObject>

#include "extractor.h"
#include "image.h"
#include "imagegenerationthread.h"
#include "imagerequest.h"

using namespace Qt::StringLiterals;

class Manga : public QObject
{
    Q_OBJECT
public:
    explicit Manga(const QString &path, QObject *parent = nullptr);
    ~Manga();

    enum class Type {
        Unknown,
        FileCbz,
        FileCbr,
        FileCb7,
        FileCbt,
        Folder,
    };

    void init();
    Type type() const;
    QImage image(ImageRequest *request);
    QList<Image> images() const;
    void addRequests(QList<ImageRequest *> requests);

Q_SIGNALS:
    void imagesReady();
    void imageReady(const QImage &image, int number);

private:
    void sendRequest();
    void generatePixmap(ImageRequest *request);
    void requestDone(ImageRequest *request);
    bool canGeneratePixmap();
    bool isZip();
    bool isRar();
    bool isTar();
    bool is7Z();
    bool isFolder();
    QList<Image> getFolderImages();

    QString m_path;
    QString m_extractionFolder;
    QMimeType m_mimeType;
    Type m_type{Type::Unknown};
    QList<Image> m_images;
    Extractor m_extractor;
    bool m_canGenerate{true};
    std::list<ImageRequest *> m_imageRequestsStack;
    std::list<ImageRequest *> m_executingImageRequests;
    QMutex m_imageRequestsMutex;
    ImageGenerationThread *m_imageGenerationThread{nullptr};
    QFuture<void> m_processArchiveFuture;

    const QStringList m_supportedMimeTypes{u"application/zip"_s,
                                           u"application/x-cbz"_s,
                                           u"application/vnd.comicbook+zip"_s,
                                           u"application/x-7z-compressed"_s,
                                           u"application/x-cb7"_s,
                                           u"application/x-tar"_s,
                                           u"application/x-cbt"_s,
                                           u"application/x-rar"_s,
                                           u"application/x-cbr"_s,
                                           u"application/vnd.rar"_s,
                                           u"application/vnd.comicbook-rar"_s,
                                           u"inode/directory"_s};
};

#endif // MANGA_H
