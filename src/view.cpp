/*
 * SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "view.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QTimer>

#include <KActionCollection>
#include <KLocalizedString>
#include <KXMLGUIFactory>

#include "imagerequest.h"
#include "mainwindow.h"
#include "page.h"
#include "settings.h"

View::View(MainWindow *parent)
    : QGraphicsView{ parent }
{
    auto *vBar = verticalScrollBar();
    m_scrollAnimation = new QPropertyAnimation(vBar, "value", this);
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_scrollAnimation->setDuration(150);

    KXMLGUIClient::setComponentName(QStringLiteral("mangareader"), i18n("View"));
    setXMLFile(QStringLiteral("viewui.rc"));

    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setInterval(100);
    m_resizeTimer->setSingleShot(true);
    connect(m_resizeTimer, &QTimer::timeout, this, [this]() {
        for (Page *p : std::as_const(m_pages)) {
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
    setAlignment(Qt::AlignTop | Qt::AlignLeft);

    connect(MangaReaderSettings::self(), &MangaReaderSettings::Show2PagesPerRowChanged, this, [this]() {
        calculatePageSizes();
    });

    connect(verticalScrollBar(), &QScrollBar::rangeChanged,
            this, &View::onScrollBarRangeChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        QPoint topCenter = QPoint(m_scene->width()/2, 1);
        Page *p = qgraphicsitem_cast<Page *>(itemAt(topCenter));
        if (p) {
            Q_EMIT currentImageChanged(p->number());
        }
        setPagesVisibility();
    });
}

View::~View()
{
}

void View::setupActions()
{
    KActionCollection *collection = actionCollection();
    collection->setComponentDisplayName(u"View"_s);
    collection->addAssociatedWidget(this);

    auto scrollToStart = new QAction(i18n("Scroll To Start"));
    scrollToStart->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollToStart, &QAction::triggered, this, [this]() {
        verticalScrollBar()->setValue(verticalScrollBar()->minimum());
    });
    collection->setDefaultShortcut(scrollToStart, Qt::CTRL | Qt::Key_Home);
    collection->addAction(u"scrollToStart"_s, scrollToStart);

    auto scrollToEnd = new QAction(i18n("Scroll To End"));
    scrollToEnd->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollToEnd, &QAction::triggered, this, [this]() {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    });
    collection->setDefaultShortcut(scrollToEnd, Qt::CTRL | Qt::Key_End);
    collection->addAction(u"scrollToEnd"_s, scrollToEnd);

    auto scrollUp = new QAction(i18n("Scroll Up"));
    scrollUp->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollUp, &QAction::triggered, this, [this]() {
        for (int i = 0; i < 3; ++i) {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
    });
    collection->setDefaultShortcut(scrollUp, Qt::Key_Up);
    collection->addAction(u"scrollUp"_s, scrollUp);

    auto scrollDown = new QAction(i18n("Scroll Down"));
    scrollDown->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollDown, &QAction::triggered, this, [this]() {
        for (int i = 0; i < 3; ++i) {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
    });
    collection->setDefaultShortcut(scrollDown, Qt::Key_Down);
    collection->addAction(u"scrollDown"_s, scrollDown);

    auto scrollUpOneScreen = new QAction(i18n("Scroll Up One Screen"));
    scrollUpOneScreen->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollUpOneScreen, &QAction::triggered, this, [this]() {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    });
    collection->setDefaultShortcuts(scrollUpOneScreen, {Qt::Key_PageUp, Qt::SHIFT | Qt::Key_Space});
    collection->addAction(u"scrollUpOneScreen"_s, scrollUpOneScreen);

    auto scrollDownOneScreen = new QAction(i18n("Scroll Down One Screen"));
    scrollDownOneScreen->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollDownOneScreen, &QAction::triggered, this, [this]() {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    });
    collection->setDefaultShortcuts(scrollDownOneScreen, {Qt::Key_PageDown, Qt::Key_Space});
    collection->addAction(u"scrollDownOneScreen"_s, scrollDownOneScreen);

    auto nextPage = new QAction(i18n("Next Page"));
    nextPage->setShortcutContext(Qt::WidgetShortcut);
    connect(nextPage, &QAction::triggered, this, [this]() {
        int step = MangaReaderSettings::show2PagesPerRow() ? 2 : 1;
        if (m_firstVisible < m_pages.count() - step) {
            goToPage(m_firstVisible + step);
        }
    });
    collection->setDefaultShortcut(nextPage, Qt::Key_Right);
    collection->addAction(u"nextPage"_s, nextPage);

    auto prevPage = new QAction(i18n("Previous Page"));
    prevPage->setShortcutContext(Qt::WidgetShortcut);
    connect(prevPage, &QAction::triggered, this, [this]() {
        int step = MangaReaderSettings::show2PagesPerRow() ? 2 : 1;
        if (m_firstVisible >= step) {
            goToPage(m_firstVisible - step);
        }
    });
    collection->setDefaultShortcut(prevPage, Qt::Key_Left);
    collection->addAction(u"prevPage"_s, prevPage);
}

void View::reset()
{
    qDeleteAll(m_pages);
    m_pages.clear();
    m_start.clear();
    m_end.clear();
    m_requestedPages.clear();
    m_files.clear();
    verticalScrollBar()->setValue(0);
}

void View::openManga(const QString &path)
{
    m_manga = std::make_unique<Manga>(path);

    connect(m_manga.get(), &Manga::imagesReady, this, [this]() {
        reset();
        setFiles(m_manga->images());
        createPages();
        Q_EMIT imagesLoaded(m_startPage);
        calculatePageSizes();
        setPagesVisibility();
    });

    connect(m_manga.get(), &Manga::imageReady,
            this, &View::onImageReady, Qt::QueuedConnection);

    m_manga->init();
}

void View::loadImages()
{
    createPages();
    Q_EMIT imagesLoaded(m_startPage);
    calculatePageSizes();
}

void View::createPages()
{
    QFileInfo fi;
    QScopedPointer<QIODevice> dev;
    QImageReader imageReader;
    imageReader.setAutoTransform(true);
    int i {0};
    for (auto &_file : m_files) {
        QFileInfo fi(_file.path);
        Page *p = new Page(_file.size);
        p->setNumber(i);
        p->setFilename(_file.path);
        p->setView(this);

        m_pages.append(p);
        m_scene->addItem(p);
        ++i;
    }
    m_start.resize(m_pages.size());
    m_end.resize(m_pages.size());
}

void View::calculatePageSizes()
{
    int pageYCoordinate = 0;
    const int hSpacing = MangaReaderSettings::hPageSpacing();
    const int vSpacing = MangaReaderSettings::vPageSpacing();
    int viewportWidth = viewport()->width();

    for (int i = 0; i < m_pages.count(); i++) {
        const auto &p1 = m_pages.at(i);
        p1->calculateScaledSize();

        if (MangaReaderSettings::show2PagesPerRow() && (i + 1 < m_pages.count())) {
            const auto &p2 = m_pages.at(i + 1);
            p2->calculateScaledSize();

            int totalWidth = p1->scaledSize().width() + p2->scaledSize().width() + hSpacing;
            int startX = (viewportWidth - totalWidth) / 2;

            p1->setPos(startX, pageYCoordinate);
            p1->setRect({p1->x(), p1->y(),
                         static_cast<qreal>(p1->scaledSize().width()),
                         static_cast<qreal>(p1->scaledSize().height())});

            p2->setPos(startX + p1->scaledSize().width() + hSpacing, pageYCoordinate);
            p2->setRect({p2->x(), p2->y(),
                         static_cast<qreal>(p2->scaledSize().width()),
                         static_cast<qreal>(p2->scaledSize().height())});

            int maxHeight = std::max(p1->scaledSize().height(), p2->scaledSize().height());

            // Map both pages to the same Y-range for visibility logic
            m_start[i] = pageYCoordinate;
            m_start[i + 1] = pageYCoordinate;
            m_end[i] = pageYCoordinate + maxHeight;
            m_end[i + 1] = pageYCoordinate + maxHeight;

            pageYCoordinate += maxHeight + vSpacing;
            // skip next page as we processed it here
            i++;
        } else {
            // single page
            const int x = (viewportWidth - p1->scaledSize().width()) / 2;
            p1->setPos(x, pageYCoordinate);
            p1->setRect({p1->x(), p1->y(),
                         static_cast<qreal>(p1->scaledSize().width()),
                         static_cast<qreal>(p1->scaledSize().height())});

            int height = p1->scaledSize().height();
            m_start[i] = pageYCoordinate;
            m_end[i] = pageYCoordinate + height;
            pageYCoordinate += height + vSpacing;
        }
    }
    m_scene->setSceneRect(0, 0, viewportWidth, pageYCoordinate);
}

void View::setPagesVisibility()
{
    QList<ImageRequest *> requestedImages;
    QList<Page *> visiblePages;

    m_firstVisible = -1;
    m_firstVisibleOffset = 0.0F;

    const QRectF viewportRect(horizontalScrollBar()->value(),
                              verticalScrollBar()->value(),
                              viewport()->width(),
                              viewport()->height());
    // use a bigger rect than the viewport's rect to check if the page is in view
    // this way pages just outside the actual viewport are also loaded
    const QRectF customViewportRect(horizontalScrollBar()->value(),
                                    verticalScrollBar()->value() - viewport()->height(),
                                    viewport()->width(),
                                    viewport()->height() * 3);
    for (const auto &page : std::as_const(m_pages)) {
        QRectF intersectionRect = customViewportRect.intersected(page->rect());
        if (intersectionRect.isEmpty()) {
            page->deleteImage();
            continue;
        }

        if (viewportRect.intersects(page->rect())) {
            if (m_firstVisible < 0) {
                m_firstVisible = page->number();
                // hidden portion (%) of page
                m_firstVisibleOffset = static_cast<float>(verticalScrollBar()->value() - page->pos().y())
                                       / static_cast<float>(page->scaledSize().height());
            }
        }

        visiblePages.append(page);

        if (page->isImageDeleted()) {
            ImageRequest *ir = new ImageRequest();
            ir->pageNumber = page->number();
            ir->path = page->filename();
            ir->size = page->scaledSize();
            requestedImages.append(ir);
        }
    }

    m_manga->addRequests(requestedImages);
}

void View::addRequest(int number)
{
    if (m_requestedPages.contains(number)) {
        return;
    }
    m_requestedPages.insert(number);
    QString filename = m_pages.at(number)->filename();
    Q_EMIT requestImage(number, filename);
}

void View::delRequest(int number)
{
    m_requestedPages.remove(number);
}

void View::onImageReady(const QImage &image, int number)
{
    if (number < 0 || number >= m_pages.size()) {
        return;
    }
    m_pages.at(number)->setImage(image);
    if (m_startPage > 0) {
        goToPage(m_startPage);
        m_startPage = 0;
    }
}

void View::onImageResized(const QImage &image, int number)
{
    if (number < 0 || number >= m_pages.size()) {
        return;
    }
    m_pages.at(number)->redraw(image);
}

void View::onScrollBarRangeChanged(int x, int y)
{
    Q_UNUSED(x)
    Q_UNUSED(y)

    if (m_pages.isEmpty()) {
        return;
    }

    if (m_firstVisible >= 0)
    {
        auto page = m_pages.at(m_firstVisible);
        page->calculateScaledSize();
        auto pageHeight = page->scaledSize().height();
        int offset = page->pos().y() + m_firstVisibleOffset * pageHeight;

        verticalScrollBar()->setValue(offset);
    }
}

void View::refreshPages()
{
    if (!m_manga) {
        return;
    }

    // clear requested pages so they are resized too
    m_requestedPages.clear();
    if (MangaReaderSettings::useCustomBackgroundColor()) {
        setBackgroundBrush(MangaReaderSettings::backgroundColor());
    } else {
        setBackgroundBrush(QPalette().base());
    }

    if (maximumWidth() != MangaReaderSettings::maxWidth()) {
        for (Page *page: std::as_const(m_pages)) {
            page->setZoom(m_globalZoom);
            if (!page->isImageDeleted()) {
                page->deleteImage();
            }
        }
    }
    calculatePageSizes();
    setPagesVisibility();
}

void View::resizeEvent(QResizeEvent *e)
{
    if (m_pages.isEmpty()) {
        return;
    }
    if (MangaReaderSettings::useResizeTimer()) {
        m_resizeTimer->start();
    } else {
        for (Page *p : std::as_const(m_pages)) {
            p->redrawImage();
        }
        calculatePageSizes();
        setPagesVisibility();
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

    if (event->button() != Qt::MiddleButton) {
        return;
    }

    QPointF position = mapFromGlobal(event->globalPosition());
    Page *page;
    if (QGraphicsItem *item = itemAt(position.toPoint())) {
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
        event->accept();
    } else {
        if (!MangaReaderSettings::smoothScrolling()) {
            QGraphicsView::wheelEvent(event);
            return;
        }

        const int delta = event->angleDelta().y();
        if (delta == 0) {
            return;
        }

        auto *bar = verticalScrollBar();

        // initialize target on first scroll
        if (m_scrollAnimation->state() != QAbstractAnimation::Running) {
            m_targetScrollValue = bar->value();
        }

        // accumulate scroll distance
        const int step = delta;
        m_targetScrollValue -= step;

        // clamp to scrollbar range
        m_targetScrollValue = qBound(bar->minimum(), m_targetScrollValue, bar->maximum());

        // restart animation from current position
        m_scrollAnimation->stop();
        m_scrollAnimation->setStartValue(bar->value());
        m_scrollAnimation->setEndValue(m_targetScrollValue);
        m_scrollAnimation->start();

        event->accept();
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
                ? QIcon::fromTheme(u"zoom-out"_s)
                : QIcon::fromTheme(u"zoom-in"_s);
        menu->addAction(zoomActionIcon, zoomActionText, this, [this, page]() {
            togglePageZoom(page);
            calculatePageSizes();
        });

        menu->addAction(QIcon::fromTheme(u"folder-bookmark"_s), i18n("Set Bookmark"), this, [this, page] {
            Q_EMIT addBookmark(page->number());
        });

        menu->addAction(QIcon::fromTheme(u"selection-make-bitmap-copy"_s), i18n("Copy Image"), this, [page] {
            QApplication::clipboard()->setImage(page->image());
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

void View::setLoadFromMemory(bool newLoadFromMemory)
{
    m_loadFromMemory = newLoadFromMemory;
}

bool View::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::PaletteChange:
        if (MangaReaderSettings::useCustomBackgroundColor()) {
            setBackgroundBrush(MangaReaderSettings::backgroundColor());
        } else {
            setBackgroundBrush(QPalette().base());
        }
        break;
    default:
        break;

    }
    return QGraphicsView::event(event);
}

void View::goToPage(int number)
{
    if (m_pages.isEmpty()) {
        return;
    }
    verticalScrollBar()->setValue(m_pages.at(number)->pos().y());
}

auto View::imageCount() -> int
{
    return m_pages.count();
}

void View::setStartPage(int number)
{
    m_startPage = number;
}

void View::setFiles(const QList<Image> &images)
{
    m_files = images;
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
