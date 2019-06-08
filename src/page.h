#ifndef PAGE_H
#define PAGE_H

#include <QGraphicsItem>

class QPixmap;
class View;

class Page : public QGraphicsItem
{
public:
    Page(int width, int height, int number, QGraphicsItem *parent = nullptr);
    ~Page() = default;
    void setView(View *view);
    void setMaxWidth(int maxWidth);
    void setImage(const QImage &image);
    void redrawImage();
    void redraw(const QImage &image);
    void deleteImage();
    void setEstimatedSize(QSize estimatedSize);
    QSize estimatedSize();
    QSize scaledSize();
    QSize sourceSize();
    bool isImageDeleted() const;
    int number();

private:
    void calculateScaledSize();
    void calculateSourceSize();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

    View    *m_view;
    QSize    m_estimatedSize;
    QSize    m_scaledSize;
    QSize    m_sourceSize;
    int      m_number;
    int      m_maxWidth;
    double   m_ratio;
    QPixmap *m_pixmap = nullptr;
    QImage  *m_image = nullptr;
};

#endif // PAGE_H
