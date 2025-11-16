#include "ui/QtRenderer.h"
#include "svg/SVGCircle.h"
#include "svg/SVGEllipse.h"
#include "svg/SVGLine.h"
#include "svg/SVGPolygon.h"
#include "svg/SVGPolyline.h"
#include "svg/SVGRect.h"
#include "svg/SVGText.h"
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QDebug>
#include <cmath>

// Constructor
QtRenderer::QtRenderer(QPainter *painter)
    : m_painter(painter) {}
// Destructor
QtRenderer::~QtRenderer() {}
// Visit SVGRect
void QtRenderer::visit(SVGRect &rect)
{
    // Lấy dữ liệu từ SVGRect
    SVGRectF r = rect.getRect();
    SVGNumber rx = rect.getRx();
    SVGNumber ry = rect.getRy();

    // Thiết lập bút và cọ vẽ (ví dụ, màu sắc, độ dày)
    QPen pen(Qt::black);       // Mặc định màu đen
    QBrush brush(Qt::NoBrush); // Mặc định không tô
    m_painter->setPen(pen);
    m_painter->setBrush(brush);

    // Vẽ hình chữ nhật bo góc
    m_painter->drawRoundedRect(QRectF(r.x, r.y, r.width, r.height), rx, ry);
}
// Visit SVGCircle
void QtRenderer::visit(SVGCircle &circle)
{
    SVGPointF center = circle.getCenter();
    SVGNumber radius = circle.getRadius();
    QPen pen(Qt::black);
    QBrush brush(Qt::NoBrush);
    m_painter->setPen(pen);
    m_painter->setBrush(brush);
    m_painter->drawEllipse(QPointF(center.x, center.y), radius, radius);
}
// Visit SVGPolygon
void QtRenderer::visit(SVGPolygon &polygon)
{
    const std::vector<SVGPointF> &points = polygon.getPoints();
    QPolygonF qPolygon;
    for (const auto &pt : points)
    {
        qPolygon << QPointF(pt.x, pt.y);
    }
    QPen pen(Qt::black);
    QBrush brush(Qt::NoBrush);
    m_painter->setPen(pen);
    m_painter->setBrush(brush);
    m_painter->drawPolygon(qPolygon);
}
// Visit SVGPolyline
void QtRenderer::visit(SVGPolyline &polyline)
{
    const std::vector<SVGPointF> &points = polyline.getPoints();
    QPolygonF qPolygon;
    for (const auto &pt : points)
    {
        qPolygon << QPointF(pt.x, pt.y);
    }
    QPen pen(Qt::black);
    m_painter->setPen(pen);
    m_painter->drawPolyline(qPolygon);
}
// Visit SVGText
void QtRenderer::visit(SVGText &text)
{
    SVGPointF position = text.getPosition();
    std::string content = text.getText();
    QFont font("Arial", 12); // Mặc định font Arial, cỡ 12
    QPen pen(Qt::black);
    m_painter->setFont(font);
    m_painter->setPen(pen);
    m_painter->drawText(QPointF(position.x, position.y), QString::fromStdString(content));
}
// Visit SVGEllipse
void QtRenderer::visit(SVGEllipse &ellipse)
{
    SVGPointF center = ellipse.getCenter();
    SVGNumber rx = ellipse.getRx();
    SVGNumber ry = ellipse.getRy();
    QPen pen(Qt::black);
    QBrush brush(Qt::NoBrush);
    m_painter->setPen(pen);
    m_painter->setBrush(brush);
    m_painter->drawEllipse(QPointF(center.x, center.y), rx, ry);
}
// Visit SVGLine
void QtRenderer::visit(SVGLine &line)
{
    SVGPointF start = line.getP1();
    SVGPointF end = line.getP2();
    QPen pen(Qt::black);
    m_painter->setPen(pen);
    m_painter->drawLine(QPointF(start.x, start.y), QPointF(end.x, end.y));
}

// Visit SVGGroup (begin)
void QtRenderer::visitGroupBegin(SVGGroup &group)
{
}
