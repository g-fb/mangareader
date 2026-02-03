#include "manga.h"

#include <QCollator>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QThread>

#include "worker.h"

Manga::Manga(const QString &path, QObject *parent)
    : QObject{parent}
    , m_path{path}
    , m_worker{new Worker}
{
}
void Manga::init()
{
    QFileInfo fi{m_path};
    QMimeDatabase db;
    m_mimeType = db.mimeTypeForFile(m_path, QMimeDatabase::MatchContent);
    if (isZip()) {
        m_type = Type::FileCbz;
    } else if (isRar()) {
        m_type = Type::FileCbr;
    } else if (is7Z()) {
        m_type = Type::FileCb7;
    } else if (isTar()) {
        m_type = Type::FileCbt;
    } else if(isFolder()) {
        m_type = Type::Folder;
    } else {
        m_type = Type::Unknown;
    }

    // ==================================================
    // setup thread and worker
    // ==================================================
    m_thread = new QThread(this);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished,
            m_thread, &QThread::deleteLater);
    m_thread->start();

    switch(m_type) {
    case Type::FileCbz:
    case Type::FileCbr:
    case Type::FileCb7:
    case Type::FileCbt:
        QMetaObject::invokeMethod(m_worker, &Worker::processArchive, Qt::QueuedConnection, m_path);
        break;
    case Type::Folder:
        getFolderImages();
        Q_EMIT imagesReady();
        break;
    case Type::Unknown:
        qDebug() << "Unknown manga type";
        break;
    }

    connect(m_worker, &Worker::imageReady, this, &Manga::imageReady);
    connect(m_worker, &Worker::archiveProcessed, this, [this](const QList<Image> &images) {
        m_images = images;
        Q_EMIT imagesReady();
    }, Qt::QueuedConnection);
}

Manga::~Manga()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void Manga::processImageRequest(int pageNumber, const QString &path)
{
    switch(m_type) {
    case Type::FileCbz:
    case Type::FileCbr:
    case Type::FileCb7:
    case Type::FileCbt:
        QMetaObject::invokeMethod(m_worker, &Worker::processMemoryImageRequest, Qt::QueuedConnection, pageNumber, path);
        break;
    case Type::Folder:
        QMetaObject::invokeMethod(m_worker, &Worker::processDriveImageRequest, Qt::QueuedConnection, pageNumber, path);
        break;
    case Type::Unknown:
        break;
    }
}

// clang-format off
bool Manga::isZip()
{
    return m_mimeType.inherits(u"application/x-cbz"_s)
        || m_mimeType.inherits(u"application/zip"_s)
        || m_mimeType.inherits(u"application/vnd.comicbook+zip"_s);
}

bool Manga::isRar()
{
    return m_mimeType.inherits(u"application/x-cbr"_s)
        || m_mimeType.inherits(u"application/x-rar"_s)
        || m_mimeType.inherits(u"application/vnd.rar"_s)
        || m_mimeType.inherits(u"application/vnd.comicbook-rar"_s);
}

bool Manga::isTar()
{
    return m_mimeType.inherits(u"application/x-cbt"_s)
        || m_mimeType.inherits(u"application/x-tar"_s);
}

bool Manga::is7Z()
{
    return m_mimeType.inherits(u"application/x-cb7"_s)
        || m_mimeType.inherits(u"application/x-7z-compressed"_s);
}

bool Manga::isFolder()
{
    return m_mimeType.inherits(u"inode/directory"_s);
}
// clang-format on

QList<Image> Manga::getFolderImages()
{
    bool recursive{true};
    // get images from path
    QDirIterator::IteratorFlags flags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;


    QMimeDatabase db;
    QDirIterator it(m_path, QDir::Files, flags);
    while (it.hasNext()) {
        QString file = it.next();

        // only get images
        if (db.mimeTypeForFile(file).name().startsWith(u"image/"_s)) {
            QImageReader reader;
            reader.setDevice(new QFile(file));
            m_images.append({file, reader.size()});
        }
    }
    // natural sort images
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(m_images.begin(), m_images.end(), [&collator](const Image &a, const Image &b) {
        return collator.compare(a.path, b.path) < 0;
    });

    if (m_images.count() < 1) {
        return {};
    }
    return m_images;
}

QList<Image> Manga::images() const
{
    return m_images;
}

Manga::Type Manga::type() const
{
    return m_type;
}

