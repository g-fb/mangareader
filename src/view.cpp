/*
 * Copyright 2019 Florea Banus George <georgefb899@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "_debug.h"
#include "mainwindow.h"
#include "page.h"
#include "view.h"
#include "worker.h"
#include "settings.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KXMLGUIFactory>

#include <QApplication>
#include <QImageReader>
#include <QMenu>
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

    connect(this, &View::requestPage,
            Worker::instance(), &Worker::processImageRequest);

    connect(Worker::instance(), &Worker::imageReady,
            this, &View::onImageReady);

    connect(Worker::instance(), &Worker::imageResized,
            this, &View::onImageResized);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged,
            this, &View::onScrollBarRangeChanged);
}

void View::setupActions()
{
    KActionCollection *collection = actionCollection();
    collection->setComponentDisplayName("View");
    collection->addAssociatedWidget(this);

    auto scrollUp = new QAction(i18n("Scroll Up"));
    collection->setDefaultShortcut(scrollUp, Qt::Key_Up);
    scrollUp->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollUp, &QAction::triggered, this, [=]() {
        for (int i = 0; i < 3; ++i) {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
    });

    auto scrollDown = new QAction(i18n("Scroll Down"));
    collection->setDefaultShortcut(scrollDown, Qt::Key_Down);
    scrollDown->setShortcutContext(Qt::WidgetShortcut);
    connect(scrollDown, &QAction::triggered, this, [=]() {
        for (int i = 0; i < 3; ++i) {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
    });

    auto nextPage = new QAction(i18n("Next Page"));
    collection->setDefaultShortcut(nextPage, Qt::Key_Right);
    nextPage->setShortcutContext(Qt::WidgetShortcut);
    connect(nextPage, &QAction::triggered, this, [=]() {
        if (m_firstVisible < m_pages.count() - 1) {
            goToPage(m_firstVisible + 1);
        }
    });

    auto prevPage = new QAction(i18n("Previous Page"));
    collection->setDefaultShortcut(prevPage, Qt::Key_Left);
    prevPage->setShortcutContext(Qt::WidgetShortcut);
    connect(prevPage, &QAction::triggered, this, [=]() {
        if (m_firstVisible > 0) {
            goToPage(m_firstVisible - 1);
        }
    });

    collection->addAction("scrollUp", scrollUp);
    collection->addAction("scrollDown", scrollDown);
    collection->addAction("nextPage", nextPage);
    collection->addAction("prevPage", prevPage);
}

auto View::imageCount() -> int
{
    return m_images.count();
}

void View::reset()
{
    m_requestedPages.clear();
    verticalScrollBar()->setValue(0);
}

void View::loadImages()
{
    createPages();
    calculatePageSizes();
    setPagesVisibility();

    emit imagesLoaded();
}

void View::goToPage(int number)
{
    verticalScrollBar()->setValue(m_start[number]);
}

void View::setStartPage(int number)
{
    m_startPage = number;
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

void View::createPages()
{
    qDeleteAll(m_pages);
    m_pages.clear();
    m_start.clear();
    m_end.clear();
    m_start.resize(m_images.count());
    m_end.resize(m_images.count());

    // create page for each image
    for (int i = 0; i < m_images.count(); i++) {

        QImageReader imageReader(m_images[i]);
        Page *p = new Page(imageReader.size(), i);
        p->setView(this);

        m_pages.append(p);
        m_scene->addItem(p);
    }
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
    emit requestPage(number);
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
    m_pages.at(number)->setImage(image);
    calculatePageSizes();
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

    emit doubleClicked();
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

    emit mouseMoved(event);
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
            emit addBookmark(page->number());
        });
        menu->popup(event->globalPos());
    }
}

void View::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    setPagesVisibility();
}

void View::setManga(const QString &manga)
{
    m_manga = manga;
}

void View::setImages(const QStringList &images)
{
    m_images = images;
}
