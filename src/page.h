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
    const QImage &image() const;
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

    auto number() -> int;
    void setNumber(int newNumber);

    const QString &filename() const;
    void setFilename(const QString &newFilename);

private:
    auto boundingRect() const -> QRectF override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    View    *m_view{};
    QSize    m_scaledSize;
    QSize    m_sourceSize;
    int      m_maxWidth{-1};
    int      m_number{-1};
    double   m_zoom{1.0};
    bool     m_isZoomToggled{false};
    double   m_ratio{1.0};
    QPixmap  m_pixmap;
    QImage   m_image;
    QString  m_filename;
};

#endif // PAGE_H
