#include "manga.h"

#include <QCollator>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QThread>
#include <QTimer>
#include <QtConcurrent>

Manga::Manga(const QString &path, QObject *parent)
    : QObject{parent}
    , m_path{path}
    , m_imageGenerationThread{new ImageGenerationThread(this)}
{
    QObject::connect(m_imageGenerationThread, &ImageGenerationThread::finished, this, [this] {
        ImageRequest *request = m_imageGenerationThread->request();
        const QImage &img = request->image;
        Q_EMIT imageReady(img, request->pageNumber);

        m_imageGenerationThread->endGeneration();
        m_canGenerate = true;

        requestDone(request);
    }, Qt::QueuedConnection);

    connect(&m_extractor, &Extractor::finishedRar, this, [this]() {
        m_type = Type::Folder;
        m_path = m_extractor.extractionFolder();
        QMimeDatabase db;
        m_mimeType = db.mimeTypeForFile(m_path, QMimeDatabase::MatchContent);
        getFolderImages();
        if (!m_images.isEmpty()) {
            Q_EMIT imagesReady();
        }
    }, Qt::QueuedConnection);
}

Manga::~Manga()
{
    m_imageRequestsMutex.lock();
    qDeleteAll(m_imageRequestsStack);
    m_imageRequestsStack.clear();

    m_imageRequestsMutex.unlock();

    if (m_imageGenerationThread) {
        m_imageGenerationThread->wait();
    }
    delete m_imageGenerationThread;
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

    switch(m_type) {
    case Type::FileCbz:
    case Type::FileCb7:
    case Type::FileCbt:
        m_processArchiveFuture = QtConcurrent::run([this]() {
            m_extractor.open(m_path);
            m_images = m_extractor.filesList();
            QMetaObject::invokeMethod(this, [this]() {
                Q_EMIT imagesReady();
            }, Qt::QueuedConnection);
        });
        break;
    case Type::FileCbr:
        m_extractor.open(m_path);
        break;
    case Type::Folder:
        getFolderImages();
        if (!m_images.isEmpty()) {
            Q_EMIT imagesReady();
        }
        break;
    case Type::Unknown:
        qDebug() << "Unknown manga type";
        break;
    }
}

Manga::Type Manga::type() const
{
    return m_type;
}

QImage Manga::image(ImageRequest *request)
{
    QImage img;
    switch(m_type) {
    case Type::FileCbz:
    case Type::FileCbr:
    case Type::FileCb7:
    case Type::FileCbt:
        m_extractor.open(m_path);
        img.loadFromData(m_extractor.getFileData(request->path));
        break;
    case Type::Folder:
        img.load(request->path);
        break;
    case Type::Unknown:
        break;
    }

    return img.scaled(request->size.width(), request->size.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QList<Image> Manga::images() const
{
    return m_images;
}

void Manga::addRequests(QList<ImageRequest *> requests)
{
    QSet<int> requestedPages;
    {
        for (const ImageRequest *request : requests) {
            requestedPages.insert(request->pageNumber);
        }
    }

    m_imageRequestsMutex.lock();
    auto sIt = m_imageRequestsStack.begin();
    auto sEnd = m_imageRequestsStack.end();
    // Iterate through the pending request stack and remove any requests that
    // target pages already present in the new request set. This prevents
    // duplicate work and ensures that only the most recent request for a page
    // remains in the queue.
    while (sIt != sEnd) {
        if (requestedPages.contains((*sIt)->pageNumber)) {
            delete *sIt;
            sIt = m_imageRequestsStack.erase(sIt);
        } else {
            ++sIt;
        }
    }

    for (ImageRequest *request : requests) {
        m_imageRequestsStack.push_back(request);
    }
    m_imageRequestsMutex.unlock();

    sendRequest();
}

void Manga::sendRequest()
{
    ImageRequest *request = nullptr;
    m_imageRequestsMutex.lock();
    while (!m_imageRequestsStack.empty() && !request) {
        ImageRequest *r = m_imageRequestsStack.back();
        if (!r) {
            m_imageRequestsStack.pop_back();
            continue;
        }
        request = r;
    }
    if (!request) {
        m_imageRequestsMutex.unlock();
        return;
    }

    if (canGeneratePixmap()) {
        m_imageRequestsStack.remove(request);
        m_executingImageRequests.push_back(request);
        m_imageRequestsMutex.unlock();

        generatePixmap(request);
    } else {
        m_imageRequestsMutex.unlock();
        QTimer::singleShot(30, this, [this] { sendRequest(); });
    }
}

void Manga::generatePixmap(ImageRequest *request)
{
    m_canGenerate = false;
    m_imageGenerationThread->startGeneration(request);
}

void Manga::requestDone(ImageRequest *request)
{
    if (!request) {
        return;
    }

    m_imageRequestsMutex.lock();
    m_executingImageRequests.remove(request);
    m_imageRequestsMutex.unlock();

    delete request;
    request = nullptr;

    m_imageRequestsMutex.lock();
    bool hasPixmaps = !m_imageRequestsStack.empty();
    m_imageRequestsMutex.unlock();
    if (hasPixmaps) {
        sendRequest();
    }
}

bool Manga::canGeneratePixmap()
{
    return m_canGenerate;
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
            QImageReader reader(file);
            m_images.append({file, reader.size()});
        }
    }
    // natural sort images
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(m_images.begin(), m_images.end(), [&collator](const Image &a, const Image &b) {
        return collator.compare(a.path, b.path) < 0;
    });

    if (m_images.isEmpty()) {
        return {};
    }
    return m_images;
}
