#include "ui/QtRenderer.h"

#include "svg/SVGCircle.h"
#include "svg/SVGEllipse.h"
#include "svg/SVGGroup.h"
#include "svg/SVGLine.h"
#include "svg/SVGPath.h"
#include "svg/SVGPolygon.h"
#include "svg/SVGPolyline.h"
#include "svg/SVGRect.h"
#include "svg/SVGStyle.h"
#include "svg/SVGText.h"
#include "svg/SVGTransform.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <cctype>
#include <cmath> 
#include <cstdlib>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

        return QTransform(mappedX.x - mappedOrigin.x, mappedX.y - mappedOrigin.y, 0.0,
                          mappedY.x - mappedOrigin.x, mappedY.y - mappedOrigin.y, 0.0,
                          mappedOrigin.x, mappedOrigin.y, 1.0);
    }

    double normalizedOpacity(SVGNumber value)
    {
        if (value < 0.0)
            return 1.0;
        return std::clamp(static_cast<double>(value), 0.0, 1.0);
    }

    QColor toQColor(const SVGColor& color, double opacityFactor)
    {
        if (color.isNone)
            return QColor(Qt::transparent);
        double alpha = normalizedOpacity(static_cast<double>(color.a) / 255.0) * opacityFactor;
        QColor qColor(color.r, color.g, color.b);
        qColor.setAlphaF(std::clamp(alpha, 0.0, 1.0));
        return qColor;
    }

    bool hasVisibleFill(const SVGStyle& style)
    {
        return !style.fillColor.isNone && normalizedOpacity(style.fillOpacity) > 0.0;
    }

    bool hasVisibleStroke(const SVGStyle& style)
    {
        return !style.strokeColor.isNone && style.strokeWidth > 0.0 &&
               normalizedOpacity(style.strokeOpacity) > 0.0;
    }

    Qt::FillRule toQtFillRule(SVGFillRule rule)
    {
        return (rule == SVGFillRule::EvenOdd) ? Qt::OddEvenFill : Qt::WindingFill;
    }

    // --- CHUYỂN ĐỔI SVG ARC SANG CUBIC BEZIER ---
    void drawSVGArc(QPainterPath& path, double rx, double ry, double xAxisRotation,
                    bool largeArcFlag, bool sweepFlag, double x, double y, double curX, double curY)
    {
        // 1. Xử lý bán kính = 0 -> Vẽ đường thẳng
        if (rx == 0.0 || ry == 0.0) {
            path.lineTo(x, y);
            return;
        }

        rx = std::abs(rx);
        ry = std::abs(ry);

        // 2. Tính toán các thông số trung gian
        double phi = xAxisRotation * M_PI / 180.0;
        double cosPhi = std::cos(phi);
        double sinPhi = std::sin(phi);

        double dx = (curX - x) / 2.0;
        double dy = (curY - y) / 2.0;

        // Chuyển về hệ tọa độ xoay
        double x1p = cosPhi * dx + sinPhi * dy;
        double y1p = -sinPhi * dx + cosPhi * dy;

        // Kiểm tra và scale bán kính nếu cần (nếu điểm bắt đầu và kết thúc quá xa so với bán kính)
        double rx_sq = rx * rx;
        double ry_sq = ry * ry;
        double x1p_sq = x1p * x1p;
        double y1p_sq = y1p * y1p;

        double radiiCheck = x1p_sq / rx_sq + y1p_sq / ry_sq;
        if (radiiCheck > 1.0) {
            double scale = std::sqrt(radiiCheck);
            rx *= scale;
            ry *= scale;
            rx_sq = rx * rx;
            ry_sq = ry * ry;
        }

        // 3. Tính tâm cung tròn (cx', cy')
        double sign = (largeArcFlag == sweepFlag) ? -1.0 : 1.0;
        double num = std::max(0.0, rx_sq * ry_sq - rx_sq * y1p_sq - ry_sq * x1p_sq);
        double den = rx_sq * y1p_sq + ry_sq * x1p_sq;
        double coef = sign * std::sqrt(num / den);

        double cxp = coef * ((rx * y1p) / ry);
        double cyp = coef * -((ry * x1p) / rx);

        // Chuyển tâm về hệ tọa độ gốc
        double cx = cosPhi * cxp - sinPhi * cyp + (curX + x) / 2.0;
        double cy = sinPhi * cxp + cosPhi * cyp + (curY + y) / 2.0;

        // 4. Tính góc bắt đầu và góc quét
        auto angle = [](double ux, double uy, double vx, double vy) {
            double dot = ux * vx + uy * vy;
            double len = std::sqrt(ux * ux + uy * uy) * std::sqrt(vx * vx + vy * vy);
            double val = dot / len;
            if (val > 1.0)
                val = 1.0;
            if (val < -1.0)
                val = -1.0;
            double ang = std::acos(val);
            if (ux * vy - uy * vx < 0)
                ang = -ang;
            return ang;
        };

        double ux = (x1p - cxp) / rx;
        double uy = (y1p - cyp) / ry;
        double vx = (-x1p - cxp) / rx;
        double vy = (-y1p - cyp) / ry;

        double startAngle = angle(1, 0, ux, uy);
        double dAngle = angle(ux, uy, vx, vy);

        // Xử lý hướng vẽ (Sweep Flag)
        if (!sweepFlag && dAngle > 0)
            dAngle -= 2 * M_PI;
        else if (sweepFlag && dAngle < 0)
            dAngle += 2 * M_PI;

        // 5. Chia nhỏ cung tròn thành các đoạn Bezier (mỗi đoạn <= 90 độ)
        int segments = static_cast<int>(std::ceil(std::abs(dAngle) / (M_PI / 2.0)));
        double delta = dAngle / segments;
        // Công thức tính độ dài tay đòn Bezier (kappa)
        double t =
            8.0 / 3.0 * std::sin(delta / 4.0) * std::sin(delta / 4.0) / std::sin(delta / 2.0);

        double theta = startAngle;

        for (int i = 0; i < segments; ++i) {
            double cosTheta = std::cos(theta);
            double sinTheta = std::sin(theta);
            double theta2 = theta + delta;
            double cosTheta2 = std::cos(theta2);
            double sinTheta2 = std::sin(theta2);

            // Điểm đầu đoạn cong
            double epx = cosPhi * rx * cosTheta - sinPhi * ry * sinTheta + cx;
            double epy = sinPhi * rx * cosTheta + cosPhi * ry * sinTheta + cy;

            // Vector tiếp tuyến tại điểm đầu
            double edx = -cosPhi * rx * sinTheta - sinPhi * ry * cosTheta;
            double edy = -sinPhi * rx * sinTheta + cosPhi * ry * cosTheta;

            // Điểm cuối đoạn cong
            double ep2x = cosPhi * rx * cosTheta2 - sinPhi * ry * sinTheta2 + cx;
            double ep2y = sinPhi * rx * cosTheta2 + cosPhi * ry * sinTheta2 + cy;

            // Vector tiếp tuyến tại điểm cuối
            double ed2x = -cosPhi * rx * sinTheta2 - sinPhi * ry * cosTheta2;
            double ed2y = -sinPhi * rx * sinTheta2 + cosPhi * ry * cosTheta2;

            // Control points
            double c1x = epx + t * edx;
            double c1y = epy + t * edy;
            double c2x = ep2x - t * ed2x;
            double c2y = ep2y - t * ed2y;

            path.cubicTo(c1x, c1y, c2x, c2y, ep2x, ep2y);
            theta = theta2;
        }
    }

    QPainterPath buildPainterPath(const std::string& data)
    {
        return QPainterPath();
    }

} // namespace

QtRenderer::QtRenderer(QPainter* painter) : m_painter(painter) {}

QtRenderer::~QtRenderer() = default;

void QtRenderer::visit(SVGRect& rect)
{
    if (!prepareForDrawing(rect.getStyle()))
        return;
    const SVGRectF& r = rect.getRect();
    QPainterPath path;
    if (rect.getRx() > 0.0 || rect.getRy() > 0.0)
        path.addRoundedRect(QRectF(r.x, r.y, r.width, r.height), rect.getRx(), rect.getRy());
    else
        path.addRect(QRectF(r.x, r.y, r.width, r.height));
    drawPath(path, rect.getStyle(), rect.getTransform());
}

void QtRenderer::visit(SVGCircle& circle)
{
    if (!prepareForDrawing(circle.getStyle()))
        return;
    SVGPointF center = circle.getCenter();
    QPainterPath path;
    path.addEllipse(QPointF(center.x, center.y), circle.getRadius(), circle.getRadius());
    drawPath(path, circle.getStyle(), circle.getTransform());
}

void QtRenderer::visit(SVGEllipse& ellipse)
{
    if (!prepareForDrawing(ellipse.getStyle()))
        return;
    SVGPointF center = ellipse.getCenter();
    QPainterPath path;
    path.addEllipse(QPointF(center.x, center.y), ellipse.getRx(), ellipse.getRy());
    drawPath(path, ellipse.getStyle(), ellipse.getTransform());
}

void QtRenderer::visit(SVGLine& line)
{
    if (!m_painter)
        return;
    const SVGStyle& style = line.getStyle();
    if (!prepareForDrawing(style) || !hasVisibleStroke(style))
        return;

    m_painter->save();
    QTransform combined = toQTransform(line.getTransform()) * m_painter->worldTransform();
    m_painter->setWorldTransform(combined);

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

void QtRenderer::visit(SVGPolygon& polygon)
{
    if (!prepareForDrawing(polygon.getStyle()))
        return;
    const auto& pts = polygon.getPoints();
    if (pts.empty())
        return;
    QPolygonF pathPoints;
    pathPoints.reserve(static_cast<int>(pts.size()));
    for (const auto& p : pts)
        pathPoints << QPointF(p.x, p.y);
    QPainterPath path;
    path.addPolygon(pathPoints);
    path.closeSubpath();
    drawPath(path, polygon.getStyle(), polygon.getTransform());
}

void QtRenderer::visit(SVGPolyline& polyline)
{
    if (!prepareForDrawing(polyline.getStyle()))
        return;
    const auto& pts = polyline.getPoints();
    if (pts.size() < 2)
        return;
    QPainterPath path(QPointF(pts[0].x, pts[0].y));
    for (size_t i = 1; i < pts.size(); ++i)
        path.lineTo(QPointF(pts[i].x, pts[i].y));
    drawPath(path, polyline.getStyle(), polyline.getTransform());
}

void QtRenderer::visit(SVGText& text)
{
    if (!prepareForDrawing(text.getStyle()))
        return;
    const SVGStyle& style = text.getStyle();
    m_painter->save();
    QTransform combined = toQTransform(text.getTransform()) * m_painter->worldTransform();
    m_painter->setWorldTransform(combined);

    QFont font;
    font.setFamily(
        QString::fromStdString(style.fontFamily.empty() ? "Times New Roman" : style.fontFamily));
    font.setPixelSize(style.fontSize > 0 ? static_cast<int>(style.fontSize) : 16);
    if (style.isItalic)
        font.setItalic(true);
    if (style.isBold)
        font.setBold(true);

    QString qText = QString::fromStdString(text.getText());
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(qText);
    double dx = 0.0;
    if (style.textAnchor == "middle")
        dx = -textWidth / 2.0;
    else if (style.textAnchor == "end")
        dx = -textWidth;

    SVGPointF pos = text.getPosition();
    QPainterPath path;
    path.addText(pos.x + dx, pos.y, font, qText);

    if (hasVisibleFill(style)) {
        m_painter->fillPath(
            path, QBrush(toQColor(style.fillColor, normalizedOpacity(style.fillOpacity))));
    }
    if (hasVisibleStroke(style)) {
        QPen pen(toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity)));
        pen.setWidthF(style.strokeWidth > 0 ? style.strokeWidth : 1.0);
        m_painter->strokePath(path, pen);
    }
    m_painter->restore();
}


void QtRenderer::visitGroupBegin(SVGGroup& group)
{
    if (!m_painter)
        return;
    m_painter->save();

    // Áp dụng transform của Group
    QTransform combined = toQTransform(group.getTransform()) * m_painter->worldTransform();
    m_painter->setWorldTransform(combined);
}

void QtRenderer::visitGroupEnd(SVGGroup& group)
{
    Q_UNUSED(group);
    if (m_painter) {
        m_painter->restore();
    }
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
    QPointF subPathStart(0, 0);
    QPointF lastControlPoint(0, 0);
    char lastCmd = '\0';

    for (const auto& cmd : commands) {
        bool isRelative = islower(cmd.type);
        char type = tolower(cmd.type);
        const auto& args = cmd.args;

        switch (type) {
        case 'm': { // MoveTo
            double x = args[0];
            double y = args[1];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }
            path.moveTo(x, y);
            currentPos = QPointF(x, y);
            subPathStart = currentPos;
            lastControlPoint = currentPos; // Reset control point
            break;
        }
        case 'l': { // LineTo
            double x = args[0];
            double y = args[1];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }
            path.lineTo(x, y);
            currentPos = QPointF(x, y);
            lastControlPoint = currentPos;
            break;
        }
        case 'h': { // Horizontal
            double x = args[0];
            if (isRelative)
                x += currentPos.x();
            path.lineTo(x, currentPos.y());
            currentPos.setX(x);
            lastControlPoint = currentPos;
            break;
        }
        case 'v': { // Vertical
            double y = args[0];
            if (isRelative)
                y += currentPos.y();
            path.lineTo(currentPos.x(), y);
            currentPos.setY(y);
            lastControlPoint = currentPos;
            break;
        }
        case 'c': { // Cubic
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
            lastControlPoint = QPointF(x2, y2);
            currentPos = QPointF(x, y);
            break;
        }
        case 's': { // Smooth Cubic
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
        case 'q': { // Quadratic
            double x1 = args[0], y1 = args[1];
            double x = args[2], y = args[3];
            if (isRelative) {
                x1 += currentPos.x();
                y1 += currentPos.y();
                x += currentPos.x();
                y += currentPos.y();
            }
            path.quadTo(x1, y1, x, y);
            lastControlPoint = QPointF(x1, y1);
            currentPos = QPointF(x, y);
            break;
        }
        case 't': { // Smooth Quadratic
            double x1, y1;
            if (lastCmd == 'q' || lastCmd == 't') {
                x1 = 2 * currentPos.x() - lastControlPoint.x();
                y1 = 2 * currentPos.y() - lastControlPoint.y();
            }
            else {
                x1 = currentPos.x();
                y1 = currentPos.y();
            }
            double x = args[0], y = args[1];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }
            path.quadTo(x1, y1, x, y);
            lastControlPoint = QPointF(x1, y1);
            currentPos = QPointF(x, y);
            break;
        }
        case 'a': { // Arc
            double rx = args[0];
            double ry = args[1];
            double xAxisRot = args[2];
            bool largeArc = (args[3] != 0.0);
            bool sweep = (args[4] != 0.0);
            double x = args[5];
            double y = args[6];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }
            // Gọi hàm xử lý Arc mới
            drawSVGArc(path, rx, ry, xAxisRot, largeArc, sweep, x, y, currentPos.x(),
                       currentPos.y());

            currentPos = QPointF(x, y);
            lastControlPoint = currentPos;
            break;
        }
        case 'z': { // ClosePath
            path.closeSubpath();
            currentPos = subPathStart;
            lastControlPoint = currentPos;
            break;
        }
        }
        lastCmd = type;
    }
    drawPath(path, svgPath.getStyle(), svgPath.getTransform());
}

bool QtRenderer::prepareForDrawing(const SVGStyle& style) const
{
    if (!m_painter)
        return false;
    if (!style.isDisplayed)
        return false;
    return hasVisibleFill(style) || hasVisibleStroke(style);
}

void QtRenderer::drawPath(const QPainterPath& path, const SVGStyle& style,
                          const SVGTransform& transform)
{
    if (!m_painter)
        return;
    m_painter->save();
    QTransform combined = toQTransform(transform) * m_painter->worldTransform();
    m_painter->setWorldTransform(combined);

    QBrush brush(Qt::NoBrush);
    if (hasVisibleFill(style)) {
        brush = QBrush(toQColor(style.fillColor, normalizedOpacity(style.fillOpacity)));
    }

    QPen pen(Qt::NoPen);
    if (hasVisibleStroke(style)) {
        pen = QPen(toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity)),
                   style.strokeWidth > 0.0 ? style.strokeWidth : 1.0);
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