#ifndef MANGA_H
#define MANGA_H

#include <QMimeType>
#include <QObject>

#include "image.h"

class QThread;
class Worker;

using namespace Qt::StringLiterals;

class Manga : public QObject
{
    Q_OBJECT
public:
    explicit Manga(const QString &path, QObject *parent = nullptr);
    ~Manga();
    void init();

    enum class Type {
        Unknown,
        FileCbz,
        FileCbr,
        FileCb7,
        FileCbt,
        Folder,
    };

    Type type() const;

    QList<Image> images() const;

    void processImageRequest(int, const QString &);

Q_SIGNALS:
    void imagesReady();
    void imageReady(const QImage &image, int number);

private:
    bool isZip();
    bool isRar();
    bool isTar();
    bool is7Z();
    bool isFolder();
    QList<Image> getFolderImages();

    QString m_path;
    QList<Image> m_images;
    QMimeType m_mimeType;
    Type m_type{Type::Unknown};

    QThread *m_thread{nullptr};
    Worker * m_worker{nullptr};

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
