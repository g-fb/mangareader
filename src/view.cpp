#include "_debug.h"
#include "view.h"
#include "page.h"
#include "worker.h"

#include <QScrollBar>

View::View(QWidget *parent)
    : QGraphicsView(parent)
{
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

void View::reset()
{
    m_scene->clear();
    m_pages.clear();
    m_requestedPages.clear();
    m_start.clear();
    m_end.clear();
    verticalScrollBar()->setValue(0);
}

void View::loadImages()
{
    createPages();
    calculatePageSizes();
    setPagesVisibility();
}

void View::createPages()
{
    m_start.resize(m_images.count());
    m_end.resize(m_images.count());
    int w = viewport()->width() - 10;
    int h = viewport()->height() - 10;
    // create page for each image
    if (m_images.count() > 0) {
        int y = 0;
        for (int i = 0; i < m_images.count(); i++) {
            Page *p = new Page(w, h, i);
            p->setMaxWidth(1200);
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
            if (n > 0 && !p->scaledSize().width() > 0) {
                p->setEstimatedSize(QSize(avgw, avgh));
                p->redrawImage();
            }
            p->setPos(0, y);

            const int x = (viewport()->width() - p->scaledSize().width()) / 2;
            p->setPos(x, p->y());

            int height = p->scaledSize().height() > 0 ? p->scaledSize().height() : viewport()->height() - 20;
            m_start[i] = y;
            m_end[i] = y + height;
            y += height + 50;
        }
    }
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
}

void View::setPagesVisibility()
{
    const int vy1 = verticalScrollBar()->value();
    const int vy2 = vy1 + viewport()->height();

    m_firstVisible = -1;
    m_firstVisibleOffset = 0.0f;

    for (Page *page : m_pages) {
        // page is visible on the screen but its image not loaded
        int pageNumber = page->number();
        if (isInView(m_start[pageNumber], m_end[pageNumber])) {
            if (page->isImageDeleted()) {
                addRequest(pageNumber);
            }
            if (m_firstVisible < 0) {
                m_firstVisible = pageNumber;
                // hidden portion (%) of page
                m_firstVisibleOffset = static_cast<double>(vy1 - m_start[pageNumber]) / page->scaledSize().height();
            }
        } else {
            // page is not visible but its image is loaded
            bool isPrevPageInView = isInView(m_start[pageNumber - 1], m_end[pageNumber - 1]);
            bool isNextPageInView = isInView(m_start[pageNumber + 1], m_end[pageNumber + 1]);
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
    setPagesVisibility();
}

void View::onImageResized(QImage image, int number)
{
    m_pages.at(number)->redraw(image);
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
}

void View::onScrollBarRangeChanged(int x, int y)
{
    if (m_firstVisible >= 0)
    {
        int pageHeight = (m_end[m_firstVisible] - m_start[m_firstVisible]);
        int offset = m_start[m_firstVisible] + static_cast<int>(m_firstVisibleOffset * pageHeight);
        verticalScrollBar()->setValue(offset);
    }
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
