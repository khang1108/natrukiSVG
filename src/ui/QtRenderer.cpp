#include "ui/QtRenderer.h"

#include "svg/SVGCircle.h"
#include "svg/SVGEllipse.h"
#include "svg/SVGGroup.h"
#include "svg/SVGLine.h"
#include "svg/SVGPolygon.h"
#include "svg/SVGPolyline.h"
#include "svg/SVGRect.h"
#include "svg/SVGStyle.h"
#include "svg/SVGText.h"
#include "svg/SVGTransform.h"

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <vector>

namespace
{

    QTransform toQTransform(const SVGTransform& transform)
    {
        SVGPointF origin{0.0, 0.0};
        SVGPointF xAxis{1.0, 0.0};
        SVGPointF yAxis{0.0, 1.0};

        SVGPointF mappedOrigin = transform.map(origin);
        SVGPointF mappedX = transform.map(xAxis);
        SVGPointF mappedY = transform.map(yAxis);

        qreal m11 = mappedX.x - mappedOrigin.x;
        qreal m21 = mappedX.y - mappedOrigin.y;
        qreal m12 = mappedY.x - mappedOrigin.x;
        qreal m22 = mappedY.y - mappedOrigin.y;
        qreal dx = mappedOrigin.x;
        qreal dy = mappedOrigin.y;

        return QTransform(m11, m12, 0.0, m21, m22, 0.0, dx, dy, 1.0);
    }

    double normalizedOpacity(SVGNumber value)
    {
        if (value < 0.0) {
            return 1.0;
        }
        return std::clamp(static_cast<double>(value), 0.0, 1.0);
    }

    QColor toQColor(const SVGColor& color, double opacityFactor)
    {
        if (color.isNone) {
            return QColor(Qt::transparent);
        }

        double alpha = normalizedOpacity(static_cast<double>(color.a) / 255.0) * opacityFactor;
        QColor qColor(color.r, color.g, color.b);
        qColor.setAlphaF(std::clamp(alpha, 0.0, 1.0));
        return qColor;
    }

    bool hasVisibleFill(const SVGStyle& style)
    {
        if (style.fillColor.isNone) {
            return false;
        }
        return normalizedOpacity(style.fillOpacity) > 0.0;
    }

    bool hasVisibleStroke(const SVGStyle& style)
    {
        if (style.strokeColor.isNone) {
            return false;
        }
        if (style.strokeWidth <= 0.0) {
            return false;
        }
        return normalizedOpacity(style.strokeOpacity) > 0.0;
    }

    Qt::FillRule toQtFillRule(SVGFillRule rule)
    {
        return (rule == SVGFillRule::EvenOdd) ? Qt::OddEvenFill : Qt::WindingFill;
    }

} // namespace

QtRenderer::QtRenderer(QPainter* painter) : m_painter(painter) {}

QtRenderer::~QtRenderer() = default;

void QtRenderer::visit(SVGRect& rect)
{
    if (!prepareForDrawing(rect.getStyle())) {
        return;
    }

    const SVGRectF& r = rect.getRect();
    QPainterPath path;
    if (rect.getRx() > 0.0 || rect.getRy() > 0.0) {
        path.addRoundedRect(QRectF(r.x, r.y, r.width, r.height), rect.getRx(), rect.getRy());
    }
    else {
        path.addRect(QRectF(r.x, r.y, r.width, r.height));
    }
    drawPath(path, rect.getStyle(), rect.getTransform());
}

void QtRenderer::visit(SVGCircle& circle)
{
    if (!prepareForDrawing(circle.getStyle())) {
        return;
    }

    SVGPointF center = circle.getCenter();
    QPainterPath path;
    path.addEllipse(QPointF(center.x, center.y), circle.getRadius(), circle.getRadius());
    drawPath(path, circle.getStyle(), circle.getTransform());
}

void QtRenderer::visit(SVGPolygon& polygon)
{
    if (!prepareForDrawing(polygon.getStyle())) {
        return;
    }

    const auto& pts = polygon.getPoints();
    if (pts.empty()) {
        return;
    }

    QPolygonF pathPoints;
    pathPoints.reserve(static_cast<int>(pts.size()));
    for (const auto& p : pts) {
        pathPoints << QPointF(p.x, p.y);
    }

    QPainterPath path;
    path.addPolygon(pathPoints);
    path.closeSubpath();
    drawPath(path, polygon.getStyle(), polygon.getTransform());
}

void QtRenderer::visit(SVGPolyline& polyline)
{
    if (!prepareForDrawing(polyline.getStyle())) {
        return;
    }

    const auto& pts = polyline.getPoints();
    if (pts.size() < 2) {
        return;
    }

    QPainterPath path(QPointF(pts[0].x, pts[0].y));
    for (size_t i = 1; i < pts.size(); ++i) {
        path.lineTo(QPointF(pts[i].x, pts[i].y));
    }
    drawPath(path, polyline.getStyle(), polyline.getTransform());
}

void QtRenderer::visit(SVGText& text)
{
    if (!prepareForDrawing(text.getStyle())) {
        return;
    }

    const SVGStyle& style = text.getStyle();
    if (!m_painter) {
        return;
    }

    m_painter->save();
    QTransform transform = m_painter->worldTransform();
    transform *= toQTransform(text.getTransform());
    m_painter->setWorldTransform(transform);

    QFont font;
    if (!style.fontFamily.empty()) {
        font.setFamily(QString::fromStdString(style.fontFamily));
    }
    else {
        font.setFamily(QStringLiteral("Times New Roman"));
    }
    font.setPointSizeF(style.fontSize > 0 ? style.fontSize : 16.0);
    m_painter->setFont(font);

    if (hasVisibleFill(style)) {
        QColor fillColor = toQColor(style.fillColor, normalizedOpacity(style.fillOpacity));
        m_painter->setPen(Qt::NoPen);
        m_painter->setBrush(Qt::NoBrush);
        SVGPointF pos = text.getPosition();
        m_painter->setPen(fillColor);
        m_painter->drawText(QPointF(pos.x, pos.y), QString::fromStdString(text.getText()));
    }

    m_painter->restore();
}

void QtRenderer::visit(SVGEllipse& ellipse)
{
    if (!prepareForDrawing(ellipse.getStyle())) {
        return;
    }

    SVGPointF center = ellipse.getCenter();
    QPainterPath path;
    path.addEllipse(QPointF(center.x, center.y), ellipse.getRx(), ellipse.getRy());
    drawPath(path, ellipse.getStyle(), ellipse.getTransform());
}

void QtRenderer::visit(SVGLine& line)
{
    if (!m_painter) {
        return;
    }
    const SVGStyle& style = line.getStyle();
    if (!prepareForDrawing(style)) {
        return;
    }
    if (!hasVisibleStroke(style)) {
        return;
    }

    m_painter->save();
    QTransform transform = m_painter->worldTransform();
    transform *= toQTransform(line.getTransform());
    m_painter->setWorldTransform(transform);

    QPen pen(toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity)));
    pen.setWidthF(style.strokeWidth > 0.0 ? style.strokeWidth : 1.0);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCapStyle(Qt::FlatCap);
    m_painter->setPen(pen);
    m_painter->setBrush(Qt::NoBrush);

    SVGPointF p1 = line.getP1();
    SVGPointF p2 = line.getP2();
    m_painter->drawLine(QPointF(p1.x, p1.y), QPointF(p2.x, p2.y));
    m_painter->restore();
}

void QtRenderer::visitGroupBegin(SVGGroup& group) { Q_UNUSED(group); }

void QtRenderer::visitGroupEnd(SVGGroup& group) { Q_UNUSED(group); }

bool QtRenderer::prepareForDrawing(const SVGStyle& style) const
{
    if (!m_painter) {
        return false;
    }
    if (!style.isDisplayed) {
        return false;
    }
    return hasVisibleFill(style) || hasVisibleStroke(style);
}

void QtRenderer::drawPath(const QPainterPath& path, const SVGStyle& style,
                          const SVGTransform& transform)
{
    if (!m_painter) {
        return;
    }

    m_painter->save();
    QTransform combined = m_painter->worldTransform();
    combined *= toQTransform(transform);
    m_painter->setWorldTransform(combined);

    QBrush brush(Qt::NoBrush);
    if (hasVisibleFill(style)) {
        QColor fillColor = toQColor(style.fillColor, normalizedOpacity(style.fillOpacity));
        brush = QBrush(fillColor);
    }

    QPen pen(Qt::NoPen);
    if (hasVisibleStroke(style)) {
        QColor strokeColor = toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity));
        pen = QPen(strokeColor, style.strokeWidth > 0.0 ? style.strokeWidth : 1.0);
        pen.setJoinStyle(Qt::MiterJoin);
        pen.setCapStyle(Qt::FlatCap);
    }

    m_painter->setBrush(brush);
    m_painter->setPen(pen);

    QPainterPath pathCopy(path);
    pathCopy.setFillRule(toQtFillRule(style.fillRule));
    m_painter->drawPath(pathCopy);
    m_painter->restore();
}
