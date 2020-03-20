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
#include <KToolBar>

#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>

View::View(QWidget *parent)
    : QGraphicsView(parent)
{
    MainWindow* p = qobject_cast<MainWindow *>(parent);
    auto scrollUp = new QAction(i18n("Scroll Up"));
    connect(scrollUp, &QAction::triggered, this, [=]() {
        for (int i = 0; i < 3; ++i) {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
    });
    p->actionCollection()->addAction("scrollUp", scrollUp);
    p->actionCollection()->setDefaultShortcut(scrollUp, Qt::Key_Up);

    auto scrollDown = new QAction(i18n("Scroll Down"));
    connect(scrollDown, &QAction::triggered, this, [=]() {
        for (int i = 0; i < 3; ++i) {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
    });
    p->actionCollection()->addAction("scrollDown", scrollDown);
    p->actionCollection()->setDefaultShortcut(scrollDown, Qt::Key_Down);

    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);

    setBackgroundBrush(QColor(MangaReaderSettings::backgroundColor()));
    setCacheMode(QGraphicsView::CacheBackground);

    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    connect(this, &View::requestPage,
            Worker::instance(), &Worker::processImageRequest);

    connect(Worker::instance(), &Worker::imageReady,
            this, &View::onImageReady);

    connect(Worker::instance(), &Worker::imageResized,
            this, &View::onImageResized);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged,
            this, &View::onScrollBarRangeChanged);
}

int View::imageCount()
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

void View::createPages()
{
    for (Page *page : m_pages) {
        delete page;
    }
    m_pages.clear();
    m_start.clear();
    m_end.clear();
    m_start.resize(m_images.count());
    m_end.resize(m_images.count());
    int w = viewport()->width() - 10;
    int h = viewport()->height() - 10;
    // create page for each image
    if (m_images.count() > 0) {
        for (int i = 0; i < m_images.count(); i++) {
            Page *p = new Page(w, h, i);
            p->setView(this);
            m_pages.append(p);
            m_scene->addItem(p);
        }
    }
}

void View::calculatePageSizes()
{
    if (m_pages.count() > 0) {
        int avgw = 0;
        int avgh = 0;
        int n = 0;
        for (int i = 0; i < m_pages.count(); i++) {
            // find loaded pages
            Page *p = m_pages.at(i);
            p->setMaxWidth(MangaReaderSettings::maxWidth());
            if (p->scaledSize().width() > 0) {
                const QSize s(p->scaledSize());
                avgw += s.width();
                avgh += s.height();
                ++n;
            }
        }
        if (n > 0) {
            avgw /= n;
            avgh /= n;
        }
        int y = 0;

        for (int i = 0; i < m_pages.count(); i++) {
            Page *p = m_pages.at(i);
            if (n > 0 && !(p->scaledSize().width() > 0)) {
                p->setEstimatedSize(QSize(avgw, avgh));
                p->redrawImage();
            }
            p->setPos(0, y);

            const int x = (viewport()->width() - p->scaledSize().width()) / 2;
            p->setPos(x, p->y());

            int height = p->scaledSize().height() > 0 ? p->scaledSize().height() : viewport()->height() - 20;
            m_start[i] = y;
            m_end[i] = y + height;
            y += height + MangaReaderSettings::pageSpacing();
        }
    }
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
}

void View::setPagesVisibility()
{
    const int vy1 = verticalScrollBar()->value();
//    const int vy2 = vy1 + viewport()->height();

    m_firstVisible = -1;
    m_firstVisibleOffset = 0.0f;

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
                m_firstVisibleOffset = static_cast<float>(vy1 - m_start[pageNumber]) / page->scaledSize().height();
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

bool View::hasRequest(int number) const
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

void View::onImageReady(QImage image, int number)
{
    m_pages.at(number)->setImage(image);
    calculatePageSizes();
    if (m_startPage > 0) {
        goToPage(m_startPage);
        m_startPage = 0;
    }
    setPagesVisibility();
}

void View::onImageResized(QImage image, int number)
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
        int pageHeight = (m_end[m_firstVisible] - m_start[m_firstVisible]);
        int offset = m_start[m_firstVisible] + static_cast<int>(m_firstVisibleOffset * pageHeight);
        verticalScrollBar()->setValue(offset);
    }
}

void View::onSettingsChanged()
{
    // clear requested pages so they are resized too
    m_requestedPages.clear();
    setBackgroundBrush(QColor(MangaReaderSettings::backgroundColor()));

    if (maximumWidth() != MangaReaderSettings::maxWidth()) {
        for (Page *page: m_pages) {
            if (!page->isImageDeleted()) {
                page->deleteImage();
            }
        }
    }
    calculatePageSizes();
    setPagesVisibility();
}

bool View::isInView(int imgTop, int imgBot)
{
    const int vy1 = verticalScrollBar()->value();
    const int vy2 = vy1 + viewport()->height();
    return std::min(imgBot, vy2) > std::max(imgTop, vy1);
}

void View::resizeEvent(QResizeEvent *e)
{
    for (Page *p : m_pages) {
        p->redrawImage();
    }
    calculatePageSizes();
    QGraphicsView::resizeEvent(e);
}

void View::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit doubleClicked();
}

void View::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved(event);
}

void View::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint position = mapFromGlobal(event->globalPos());
    Page *page;
    if (QGraphicsItem *item = itemAt(position)) {
        page = qgraphicsitem_cast<Page *>(item);
        QMenu *menu = new QMenu();
        menu->addAction(QIcon::fromTheme("folder-bookmark"), i18n("Set Bookmark"), this, [ = ] {
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

void View::setManga(QString manga)
{
    m_manga = manga;
}

void View::setImages(QStringList images)
{
    m_images = images;
}
