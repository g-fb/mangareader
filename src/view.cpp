/*
 * SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mainwindow.h"
#include "page.h"
#include "view.h"
#include "worker.h"
#include "settings.h"

#include <KActionCollection>
#include <KArchive>
#include <KLocalizedString>
#include <KXMLGUIFactory>

#include <QApplication>
#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>

View::View(MainWindow *parent)
    : QGraphicsView{ parent }
{
    KXMLGUIClient::setComponentName(QStringLiteral("mangareader"), i18n("View"));
    setXMLFile(QStringLiteral("viewui.rc"));

    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setInterval(100);
    m_resizeTimer->setSingleShot(true);
    connect(m_resizeTimer, &QTimer::timeout, this, [=]() {
        for (Page *p : qAsConst(m_pages)) {
            p->redrawImage();
        }
        calculatePageSizes();
    });

    setupActions();
    parent->guiFactory()->addClient(this);

    setAcceptDrops(true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    if (MangaReaderSettings::useCustomBackgroundColor()) {
        setBackgroundBrush(MangaReaderSettings::backgroundColor());
    } else {
        setBackgroundBrush(QPalette().base());
    }
    setCacheMode(QGraphicsView::CacheBackground);

    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    connect(qApp, &QApplication::paletteChanged, this, [=]() {
        if (MangaReaderSettings::useCustomBackgroundColor()) {
            setBackgroundBrush(MangaReaderSettings::backgroundColor());
        } else {
            setBackgroundBrush(QPalette().base());
        }
    });

    connect(this, &View::requestDriveImage,
            Worker::instance(), &Worker::processDriveImageRequest);

    connect(this, &View::requestMemoryImage,
            Worker::instance(), &Worker::processMemoryImageRequest);

    connect(Worker::instance(), &Worker::imageReady,
            this, &View::onImageReady);

    connect(Worker::instance(), &Worker::imageResized,
            this, &View::onImageResized);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged,
            this, &View::onScrollBarRangeChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [=]() {
        QPoint topCenter = QPoint(m_scene->width()/2, 1);
        Page *p = qgraphicsitem_cast<Page *>(itemAt(topCenter));
        if (p) {
            Q_EMIT currentImageChanged(p->number());
        }
    });
}

void View::setupActions()
{
    KActionCollection *collection = actionCollection();
    collection->setComponentDisplayName("View");
    collection->addAssociatedWidget(this);

    auto scrollToStart = new QAction(i18n("Scroll To Start"));
    scrollToStart->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollToStart, &QAction::triggered, this, [=]() {
        verticalScrollBar()->setValue(verticalScrollBar()->minimum());
    });
    collection->setDefaultShortcut(scrollToStart, Qt::CTRL + Qt::Key_Home);
    collection->addAction("scrollToStart", scrollToStart);

    auto scrollToEnd = new QAction(i18n("Scroll To End"));
    scrollToEnd->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollToEnd, &QAction::triggered, this, [=]() {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    });
    collection->setDefaultShortcut(scrollToEnd, Qt::CTRL + Qt::Key_End);
    collection->addAction("scrollToEnd", scrollToEnd);

    auto scrollUp = new QAction(i18n("Scroll Up"));
    scrollUp->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollUp, &QAction::triggered, this, [=]() {
        for (int i = 0; i < 3; ++i) {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
    });
    collection->setDefaultShortcut(scrollUp, Qt::Key_Up);
    collection->addAction("scrollUp", scrollUp);

    auto scrollDown = new QAction(i18n("Scroll Down"));
    scrollDown->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollDown, &QAction::triggered, this, [=]() {
        for (int i = 0; i < 3; ++i) {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
    });
    collection->setDefaultShortcut(scrollDown, Qt::Key_Down);
    collection->addAction("scrollDown", scrollDown);

    auto scrollUpOneScreen = new QAction(i18n("Scroll Up One Screen"));
    scrollUpOneScreen->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollUpOneScreen, &QAction::triggered, this, [=]() {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    });
    collection->setDefaultShortcuts(scrollUpOneScreen, {Qt::Key_PageUp, Qt::SHIFT | Qt::Key_Space});
    collection->addAction("scrollUpOneScreen", scrollUpOneScreen);

    auto scrollDownOneScreen = new QAction(i18n("Scroll Down One Screen"));
    scrollDownOneScreen->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollDownOneScreen, &QAction::triggered, this, [=]() {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    });
    collection->setDefaultShortcuts(scrollDownOneScreen, {Qt::Key_PageDown, Qt::Key_Space});
    collection->addAction("scrollDownOneScreen", scrollDownOneScreen);

    auto nextPage = new QAction(i18n("Next Page"));
    nextPage->setShortcutContext(Qt::WidgetShortcut);
    connect(nextPage, &QAction::triggered, this, [=]() {
        if (m_firstVisible < m_pages.count() - 1) {
            goToPage(m_firstVisible + 1);
        }
    });
    collection->setDefaultShortcut(nextPage, Qt::Key_Right);
    collection->addAction("nextPage", nextPage);

    auto prevPage = new QAction(i18n("Previous Page"));
    prevPage->setShortcutContext(Qt::WidgetShortcut);
    connect(prevPage, &QAction::triggered, this, [=]() {
        if (m_firstVisible > 0) {
            goToPage(m_firstVisible - 1);
        }
    });
    collection->setDefaultShortcut(prevPage, Qt::Key_Left);
    collection->addAction("prevPage", prevPage);
}

void View::reset()
{
    delete m_archive;
    m_archive = nullptr;
    qDeleteAll(m_pages);
    m_pages.clear();
    m_start.clear();
    m_end.clear();
    m_requestedPages.clear();
    m_files.clear();
    verticalScrollBar()->setValue(0);
}

void View::loadImages()
{
    createPages();
    calculatePageSizes();
    setPagesVisibility();

    Q_EMIT imagesLoaded();
}

void View::createPages()
{
    QScopedPointer<QIODevice> dev;
    QImageReader imageReader;
    imageReader.setAutoTransform(true);
    int i {0};
    for (auto &_file : m_files) {
        if (m_loadFromMemory) {
            const KArchiveFile *entry = m_archive->directory()->file(_file);
            if (!entry) {
                continue;
            }
            dev.reset(entry->createDevice());
        } else {
            std::unique_ptr<QFile> file(new QFile(_file));
            if (!file->open(QIODevice::ReadOnly)) {
                continue;
            }
            dev.reset(file.release());
        }

        if (dev.isNull()) {
            continue;
        }

        imageReader.setDevice(dev.data());
        if (!imageReader.canRead()) {
            continue;
        }

        QSize pageSize = imageReader.size();
        if (imageReader.transformation() & QImageIOHandler::TransformationRotate90) {
            pageSize.transpose();
        }
        if (!pageSize.isValid()) {
            const QImage i = imageReader.read();
            if (!i.isNull()) {
                pageSize = i.size();
            }
        }
        if (pageSize.isValid()) {
            Page *p = new Page(imageReader.size());
            p->setNumber(i);
            p->setFilename(_file);
            p->setView(this);

            m_pages.append(p);
            m_scene->addItem(p);
        }
        ++i;
    }
    m_start.resize(m_pages.size());
    m_end.resize(m_pages.size());
}

void View::calculatePageSizes()
{
    int pageYCoordinate = 0;
    for (int i = 0; i < m_pages.count(); i++) {
        Page *p = m_pages.at(i);
        p->calculateScaledSize();

        const int x = (viewport()->width() - p->scaledSize().width()) / 2;
        p->setPos(x, pageYCoordinate);

        int height = p->scaledSize().height();

        m_start[i] = pageYCoordinate;
        m_end[i] = pageYCoordinate + height;
        pageYCoordinate += height + MangaReaderSettings::pageSpacing();
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect());
}

void View::setPagesVisibility()
{
    const int vy1 = verticalScrollBar()->value();
//    const int vy2 = vy1 + viewport()->height();

    m_firstVisible = -1;
    m_firstVisibleOffset = 0.0F;

    for (int i = 0; i < m_pages.count(); i++) {
        // page is visible on the screen but its image not loaded
        auto page = m_pages.at(i);
        int pageNumber = page->number();

        if (isInView(m_start[pageNumber], m_end[pageNumber])) {
            if (page->isImageDeleted()) {
                addRequest(pageNumber);
            }
            if (m_firstVisible < 0) {
                m_firstVisible = pageNumber;
                // hidden portion (%) of page
                m_firstVisibleOffset = static_cast<float>(vy1 - m_start[pageNumber]) / static_cast<float>(page->scaledSize().height());
            }
        } else {
            // page is not visible but its image is loaded
            bool isPrevPageInView = false;
            if (i > 0) {
                isPrevPageInView = isInView(m_start.at(i - 1), m_end.at(i - 1));
            }
            bool isNextPageInView = false;
            if (m_start.at(i) != m_start.last()) {
                isNextPageInView = isInView(m_start.at(i + 1), m_end.at(i + 1));
            }

            if (!page->isImageDeleted()) {
                // delete page image unless it is before or after a page that is in view
                if (!(isPrevPageInView || isNextPageInView)) {
                    page->deleteImage();
                    delRequest(pageNumber);
                }
            } else { // page is not visible and its image not loaded
                // if previous page is visible load current page image too
                if (isPrevPageInView) {
                    addRequest(pageNumber);
                } else {
                    delRequest(pageNumber);
                }
            }
        }
    }
}

void View::addRequest(int number)
{
    if (hasRequest(number)) {
        return;
    }
    m_requestedPages.append(number);
    QString filename = m_pages.at(number)->filename();
    if (m_loadFromMemory) {
        Q_EMIT requestMemoryImage(number, m_archive->directory()->file(filename)->data());
    } else {
        Q_EMIT requestDriveImage(number, filename);
    }
}

auto View::hasRequest(int number) const -> bool
{
    return m_requestedPages.indexOf(number) >= 0;
}

void View::delRequest(int number)
{
    int idx = m_requestedPages.indexOf(number);
    if (idx >= 0) {
        m_requestedPages.removeAt(idx);
    }
}

void View::onImageReady(const QImage &image, int number)
{
    // when loading another manga it can happen that the number returned by the thread
    // is bigger than the total number of images the current manga has
    // resulting in an index out of range crash
    if (number > m_pages.size() - 1) {
        return;
    }
    m_pages.at(number)->setImage(image);
    //    calculatePageSizes();
    if (m_startPage > 0) {
        goToPage(m_startPage);
        m_startPage = 0;
    }
    setPagesVisibility();
}

void View::onImageResized(const QImage &image, int number)
{
    m_pages.at(number)->redraw(image);
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
}

void View::onScrollBarRangeChanged(int x, int y)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    if (m_firstVisible >= 0)
    {
        auto pageHeight = static_cast<float>(m_end[m_firstVisible] - m_start[m_firstVisible]);
        int offset = m_start[m_firstVisible] + static_cast<int>(m_firstVisibleOffset * pageHeight);
        verticalScrollBar()->setValue(offset);
    }
}

void View::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    setPagesVisibility();
}

void View::refreshPages()
{
    // clear requested pages so they are resized too
    m_requestedPages.clear();
    if (MangaReaderSettings::useCustomBackgroundColor()) {
        setBackgroundBrush(MangaReaderSettings::backgroundColor());
    } else {
        setBackgroundBrush(QPalette().base());
    }

    if (maximumWidth() != MangaReaderSettings::maxWidth()) {
        for (Page *page: qAsConst(m_pages)) {
            page->setZoom(m_globalZoom);
            if (!page->isImageDeleted()) {
                page->deleteImage();
            }
        }
    }
    calculatePageSizes();
    setPagesVisibility();
}

auto View::isInView(int imgTop, int imgBot) -> bool
{
    const int vy1 = verticalScrollBar()->value();
    const int vy2 = vy1 + viewport()->height();
    return std::min(imgBot, vy2) > std::max(imgTop, vy1);
}

void View::resizeEvent(QResizeEvent *e)
{
    if (m_pages.isEmpty()) {
        return;
    }
    if (MangaReaderSettings::useResizeTimer()) {
        m_resizeTimer->start();
    } else {
        for (Page *p : qAsConst(m_pages)) {
            p->redrawImage();
        }
        calculatePageSizes();
    }
    QGraphicsView::resizeEvent(e);
}

void View::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (event->button() != Qt::LeftButton)
        return;

    Q_EMIT doubleClicked();
}

void View::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);

    if (event->button() != Qt::MiddleButton)
        return;

    QPoint position = mapFromGlobal(event->globalPos());
    Page *page;
    if (QGraphicsItem *item = itemAt(position)) {
        page = qgraphicsitem_cast<Page *>(item);
        togglePageZoom(page);
    }
    calculatePageSizes();
}

void View::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);

    Q_EMIT mouseMoved(event);
}

void View::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
        if (event->angleDelta().y() > 0 && m_globalZoom < 2.0) {
            // zoom in, max x2
            zoomIn();
        }
        if (event->angleDelta().y() < 0 && m_globalZoom > 1.0) {
            // zoom out, not smaller that the initial size
            zoomOut();
        }
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void View::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint position = mapFromGlobal(event->globalPos());
    Page *page;
    if (QGraphicsItem *item = itemAt(position)) {
        page = qgraphicsitem_cast<Page *>(item);
        auto menu = new QMenu();
        menu->addSection(i18n("Page %1", page->number() + 1));

        QString zoomActionText = page->isZoomToggled()
                ? i18n("Zoom Out")
                : i18n("Zoom In");
        QIcon zoomActionIcon = page->isZoomToggled()
                ? QIcon::fromTheme("zoom-out")
                : QIcon::fromTheme("zoom-in");
        menu->addAction(zoomActionIcon, zoomActionText, this, [=]() {
            togglePageZoom(page);
            calculatePageSizes();
        });

        menu->addAction(QIcon::fromTheme("folder-bookmark"), i18n("Set Bookmark"), this, [=] {
            Q_EMIT addBookmark(page->number());
        });
        menu->popup(event->globalPos());
    }
}

void View::dragEnterEvent(QDragEnterEvent *e)
{
    e->setDropAction(Qt::IgnoreAction);
}

void View::dropEvent(QDropEvent *e)
{
    Q_EMIT fileDropped(e->mimeData()->urls().first().toLocalFile());
}

const QString &View::manga() const
{
    return m_manga;
}

void View::setLoadFromMemory(bool newLoadFromMemory)
{
    m_loadFromMemory = newLoadFromMemory;
}

void View::setArchive(KArchive *newArchive)
{
    m_archive = newArchive;
}

void View::goToPage(int number)
{
    if (m_pages.isEmpty()) {
        return;
    }
    verticalScrollBar()->setValue(m_start[number]);
}

auto View::imageCount() -> int
{
    return m_pages.count();
}

void View::setStartPage(int number)
{
    m_startPage = number;
}

void View::setManga(const QString &manga)
{
    m_manga = manga;
}

void View::setFiles(const QStringList &files)
{
    m_files = files;
}

void View::zoomIn()
{
    m_globalZoom += 0.1;
    refreshPages();
}

void View::zoomOut()
{
    m_globalZoom -= 0.1;
    refreshPages();
}

void View::zoomReset()
{
    m_globalZoom = 1.0;
    refreshPages();
}

void View::togglePageZoom(Page *page)
{
    if (page->isZoomToggled()) {
        auto zoom = page->zoom() < 1.3 ? 1.0 : page->zoom() - 0.3;
        page->setZoom(zoom);
    } else {
        page->setZoom(page->zoom() + 0.3);
    }
    page->setIsZoomToggled(!page->isZoomToggled());
    page->redrawImage();
}

#include "moc_view.cpp"
