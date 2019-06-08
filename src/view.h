#ifndef VIEW_H
#define VIEW_H

#include <QGraphicsView>
#include <QObject>

class Page;
class QGraphicsScene;

class View : public QGraphicsView
{
    Q_OBJECT

public:
    View(QWidget *parent = nullptr);
    ~View() = default;
    void reset();
    void setManga(QString manga);
    void setImages(QStringList images);
    void loadImages();

signals:
    void requestPage(int number);

public slots:
    void setImage(QImage image, int number);
    void imageResized(QImage image, int number);

private:
    void resizeEvent(QResizeEvent *e) override;
    void createPages();
    void calculatePageSizes();
    void setPagesVisibility();
    void scrollContentsBy(int dx, int dy) override;
    void addRequest(int number);
    void delRequest(int number);
    bool hasRequest(int number) const;
    bool isInView(int imgTop, int imgBot);

    QGraphicsScene  *m_scene;
    QString          m_manga;
    QStringList      m_images;
    QVector<Page*>   m_pages;
    QVector<int>     m_start;
    QVector<int>     m_end;
    QVector<int>     m_requestedPages;
};

#endif // VIEW_H
