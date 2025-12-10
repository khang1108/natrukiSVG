#include "ui/QtRenderer.h"

#include "svg/SVGCircle.h"
#include "svg/SVGEllipse.h"
#include "svg/SVGGroup.h"
#include "svg/SVGLine.h"
#include "svg/SVGPolygon.h"
#include "svg/SVGPolyline.h"
#include "svg/SVGRect.h"
#include "svg/SVGPath.h"
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
#include <QFontMetrics>

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

        return QTransform(m11, m21, 0.0, m12, m22, 0.0, dx, dy, 1.0);
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

    m_painter->save();

    // 1. Transform
    QTransform localTransform = toQTransform(text.getTransform());
    QTransform currentWorld = m_painter->worldTransform();
    QTransform combined = localTransform * currentWorld;
    m_painter->setWorldTransform(combined);

    // 2. Setup Font
    QFont font;
    font.setFamily(
        QString::fromStdString(style.fontFamily.empty() ? "Times New Roman" : style.fontFamily));

    // Set size
    if (style.fontSize > 0) {
        font.setPixelSize(static_cast<int>(style.fontSize));
    }
    else {
        font.setPixelSize(16);
    }

    // Set Italic (Nghiêng)
    if (style.isItalic) {
        font.setItalic(true);
    }

    // Set Bold (Đậm)
    if (style.isBold) {
        font.setBold(true);
    }

    // 3. Xử lý căn lề (Text Anchor)
    QString qText = QString::fromStdString(text.getText());
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(qText); // Đo chiều dài chữ

    double dx = 0.0;
    if (style.textAnchor == "middle") {
        dx = -textWidth / 2.0; // Dịch sang trái 50%
    }
    else if (style.textAnchor == "end") {
        dx = -textWidth; // Dịch sang trái 100%
    }

    // 4. Vẽ Text dưới dạng Path (Để có cả Fill và Stroke)
    SVGPointF pos = text.getPosition();
    QPainterPath path;
    // Cộng thêm dx vào tọa độ x
    path.addText(pos.x + dx, pos.y, font, qText);

    // 5. Tô màu (Fill)
    if (hasVisibleFill(style)) {
        QColor fillColor = toQColor(style.fillColor, normalizedOpacity(style.fillOpacity));
        m_painter->fillPath(path, QBrush(fillColor));
    }

    // 6. Vẽ viền (Stroke)
    if (hasVisibleStroke(style)) {
        QColor strokeColor = toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity));
        QPen pen(strokeColor);
        pen.setWidthF(style.strokeWidth > 0 ? style.strokeWidth : 1.0);
        m_painter->strokePath(path, pen);
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
    QTransform localTransform = toQTransform(line.getTransform());
    QTransform currentWorld = m_painter->worldTransform();

    m_painter->setWorldTransform(localTransform * currentWorld);

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

void QtRenderer::visit(SVGPath& svgPath)
{
    if (!prepareForDrawing(svgPath.getStyle())) {
        return;
    }

    const auto& commands = svgPath.getCommands();
    if (commands.empty())
        return;

    QPainterPath path;

    QPointF currentPos(0, 0);

    QPointF lastControlPoint(0, 0);
    char lastCmd = '\0';

    for (const auto& cmd : commands) {
        bool isRelative = islower(cmd.type);
        char type = tolower(cmd.type);
        const auto& args = cmd.args;

        switch (type) {
        case 'm': { // MoveTo (x, y)
            double x = args[0];
            double y = args[1];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }
            path.moveTo(x, y);
            currentPos = QPointF(x, y);
            lastControlPoint = currentPos; // Reset control point
            break;
        }
        case 'l': { // LineTo (x, y)
            double x = args[0];
            double y = args[1];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }
            path.lineTo(x, y);
            currentPos = QPointF(x, y);
            break;
        }
        case 'h': { // Horizontal LineTo (x)
            double x = args[0];
            if (isRelative)
                x += currentPos.x();
            path.lineTo(x, currentPos.y());
            currentPos.setX(x);
            break;
        }
        case 'v': { // Vertical LineTo (y)
            double y = args[0];
            if (isRelative)
                y += currentPos.y();
            path.lineTo(currentPos.x(), y);
            currentPos.setY(y);
            break;
        }
        case 'c': { // Cubic Bezier (x1, y1, x2, y2, x, y)
            double x1 = args[0], y1 = args[1];
            double x2 = args[2], y2 = args[3];
            double x = args[4], y = args[5];

            if (isRelative) {
                x1 += currentPos.x();
                y1 += currentPos.y();
                x2 += currentPos.x();
                y2 += currentPos.y();
                x += currentPos.x();
                y += currentPos.y();
            }
            path.cubicTo(x1, y1, x2, y2, x, y);
            lastControlPoint = QPointF(x2, y2); // Lưu điểm điều khiển thứ 2
            currentPos = QPointF(x, y);
            break;
        }
        case 's': { // Smooth Cubic (x2, y2, x, y)
            // Điểm điều khiển 1 là điểm phản chiếu của lastControlPoint qua currentPos
            double x1, y1;
            if (lastCmd == 'c' || lastCmd == 's') {
                x1 = 2 * currentPos.x() - lastControlPoint.x();
                y1 = 2 * currentPos.y() - lastControlPoint.y();
            }
            else {
                x1 = currentPos.x();
                y1 = currentPos.y();
            }

            double x2 = args[0], y2 = args[1];
            double x = args[2], y = args[3];

            if (isRelative) {
                x2 += currentPos.x();
                y2 += currentPos.y();
                x += currentPos.x();
                y += currentPos.y();
            }

            path.cubicTo(x1, y1, x2, y2, x, y);
            lastControlPoint = QPointF(x2, y2);
            currentPos = QPointF(x, y);
            break;
        }
        case 'z': { // ClosePath
            path.closeSubpath();
            // Sau khi close, currentPos thường quay về điểm MoveTo gần nhất
            break;
        }
            // TODO: Thêm case 'q', 't', 'a' nếu cần sau này.
        }

        // Cập nhật lastCmd để dùng cho logic Smooth Curve
        if (type != 'm')
            lastCmd = type;
    }

    // Gọi hàm vẽ chung 
    drawPath(path, svgPath.getStyle(), svgPath.getTransform());
}

void QtRenderer::visitGroupBegin(SVGGroup& group) 
{
    if (!m_painter) {
        return;
    }
    m_painter->save();

    SVGTransform groupTransform = group.getTransform();

    QTransform localTransform = toQTransform(groupTransform);
    QTransform currentWorld = m_painter->worldTransform();

    QTransform combined = localTransform * currentWorld;

    m_painter->setWorldTransform(combined);
}

void QtRenderer::visitGroupEnd(SVGGroup& group) 
{
    Q_UNUSED(group);
    if (!m_painter) {
        return;
    }

    m_painter->restore();
}

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
    QTransform localTransform = toQTransform(transform);
    QTransform currentWorld = m_painter->worldTransform();

    // Đảo ngược thứ tự: Local * World
    QTransform combined = localTransform * currentWorld;
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
