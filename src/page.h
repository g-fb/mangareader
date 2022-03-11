/*
 * SPDX-FileCopyrightText: 2019 George Florea Bănuș <georgefb899@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PAGE_H
#define PAGE_H

#include <QGraphicsItem>

class QPixmap;
class View;

class Page : public QGraphicsItem
{
public:
    Page(QSize sourceSize, QGraphicsItem *parent = nullptr);
    ~Page();
    void setView(View *view);
    void setMaxWidth(int maxWidth);
    void setImage(const QImage &image);
    void redrawImage();
    void calculateScaledSize();
    void redraw(const QImage &image);
    void deleteImage();
    void setScaledSize(QSize size);
    auto scaledSize() -> QSize;
    auto sourceSize() -> QSize;
    auto isImageDeleted() const -> bool;
    auto zoom() const -> double;
    void setZoom(double zoom);

    bool isZoomToggled() const;
    void setIsZoomToggled(bool isZoomToggled);

    const QString &key() const;
    void setKey(const QString &newKey);

    auto number() -> int;
    void setNumber(int newNumber);

private:
    auto boundingRect() const -> QRectF override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

    View    *m_view{};
    QSize    m_scaledSize;
    QSize    m_sourceSize;
    int      m_maxWidth;
    int      m_number;
    QString  m_key;
    double   m_zoom = 1.0;
    bool     m_isZoomToggled{false};
    double   m_ratio;
    QPixmap  m_pixmap;
    QImage   m_image;
};

#endif // PAGE_H
