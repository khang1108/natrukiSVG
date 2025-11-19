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
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <cctype>
#include <cstdlib>
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

    bool isCommand(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0; }

    class SvgPathBuilder
    {
      public:
        explicit SvgPathBuilder(const std::string& data) : m_data(data) {}

        QPainterPath build()
        {
            while (m_index < m_data.size()) {
                skipSeparators();
                if (m_index >= m_data.size()) {
                    break;
                }

                char c = m_data[m_index];
                if (isCommand(c)) {
                    m_command = c;
                    ++m_index;
                }
                else if (m_command == 0) {
                    ++m_index;
                    continue;
                }

                bool relative = std::islower(static_cast<unsigned char>(m_command)) != 0;
                char cmd = static_cast<char>(std::toupper(static_cast<unsigned char>(m_command)));

                switch (cmd) {
                case 'M':
                    handleMove(relative);
                    break;
                case 'L':
                    handleLine(relative);
                    break;
                case 'H':
                    handleHorizontal(relative);
                    break;
                case 'V':
                    handleVertical(relative);
                    break;
                case 'C':
                    handleCubic(relative);
                    break;
                case 'S':
                    handleSmoothCubic(relative);
                    break;
                case 'Q':
                    handleQuadratic(relative);
                    break;
                case 'T':
                    handleSmoothQuadratic(relative);
                    break;
                case 'A':
                    handleArc(relative);
                    break;
                case 'Z':
                    m_path.closeSubpath();
                    m_current = m_subpathStart;
                    m_prevCommand = 'Z';
                    break;
                default:
                    skipUnknown();
                    break;
                }
            }

            return m_path;
        }

      private:
        const std::string& m_data;
        size_t m_index = 0;
        char m_command = 0;
        char m_prevCommand = 0;
        QPointF m_current{0.0, 0.0};
        QPointF m_subpathStart{0.0, 0.0};
        QPointF m_lastCubicControl{0.0, 0.0};
        QPointF m_lastQuadControl{0.0, 0.0};
        QPainterPath m_path;

        void skipSeparators()
        {
            while (m_index < m_data.size()) {
                char c = m_data[m_index];
                if (c == ',' || std::isspace(static_cast<unsigned char>(c))) {
                    ++m_index;
                }
                else {
                    break;
                }
            }
        }

        bool readNumber(double& value)
        {
            skipSeparators();
            if (m_index >= m_data.size()) {
                return false;
            }
            const char* start = m_data.c_str() + m_index;
            char* end = nullptr;
            value = std::strtod(start, &end);
            if (start == end) {
                return false;
            }
            m_index = static_cast<size_t>(end - m_data.c_str());
            return true;
        }

        bool readPoint(QPointF& out, bool relative)
        {
            double x = 0.0;
            double y = 0.0;
            if (!readNumber(x) || !readNumber(y)) {
                return false;
            }
            if (relative) {
                out.setX(m_current.x() + x);
                out.setY(m_current.y() + y);
            }
            else {
                out.setX(x);
                out.setY(y);
            }
            return true;
        }

        void handleMove(bool relative)
        {
            bool firstPoint = true;
            while (true) {
                QPointF point;
                if (!readPoint(point, relative)) {
                    break;
                }
                if (firstPoint) {
                    m_path.moveTo(point);
                    m_subpathStart = point;
                    m_current = point;
                    firstPoint = false;
                    m_prevCommand = 'M';
                }
                else {
                    m_path.lineTo(point);
                    m_current = point;
                    m_prevCommand = 'L';
                }
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void handleLine(bool relative)
        {
            while (true) {
                QPointF point;
                if (!readPoint(point, relative)) {
                    break;
                }
                m_path.lineTo(point);
                m_current = point;
                m_prevCommand = 'L';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void handleHorizontal(bool relative)
        {
            while (true) {
                double value = 0.0;
                if (!readNumber(value)) {
                    break;
                }
                QPointF point = m_current;
                point.setX(relative ? m_current.x() + value : value);
                m_path.lineTo(point);
                m_current = point;
                m_prevCommand = 'L';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void handleVertical(bool relative)
        {
            while (true) {
                double value = 0.0;
                if (!readNumber(value)) {
                    break;
                }
                QPointF point = m_current;
                point.setY(relative ? m_current.y() + value : value);
                m_path.lineTo(point);
                m_current = point;
                m_prevCommand = 'L';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void handleCubic(bool relative)
        {
            while (true) {
                QPointF c1;
                QPointF c2;
                QPointF end;
                if (!readPoint(c1, relative) || !readPoint(c2, relative) ||
                    !readPoint(end, relative)) {
                    break;
                }
                m_path.cubicTo(c1, c2, end);
                m_lastCubicControl = c2;
                m_current = end;
                m_prevCommand = 'C';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void handleSmoothCubic(bool relative)
        {
            while (true) {
                QPointF c2;
                QPointF end;
                if (!readPoint(c2, relative) || !readPoint(end, relative)) {
                    break;
                }
                QPointF c1 = m_current;
                if (m_prevCommand == 'C' || m_prevCommand == 'S') {
                    c1 = QPointF(2 * m_current.x() - m_lastCubicControl.x(),
                                 2 * m_current.y() - m_lastCubicControl.y());
                }
                m_path.cubicTo(c1, c2, end);
                m_lastCubicControl = c2;
                m_current = end;
                m_prevCommand = 'S';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void handleQuadratic(bool relative)
        {
            while (true) {
                QPointF control;
                QPointF end;
                if (!readPoint(control, relative) || !readPoint(end, relative)) {
                    break;
                }
                m_path.quadTo(control, end);
                m_lastQuadControl = control;
                m_current = end;
                m_prevCommand = 'Q';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void handleSmoothQuadratic(bool relative)
        {
            while (true) {
                QPointF end;
                if (!readPoint(end, relative)) {
                    break;
                }
                QPointF control = m_current;
                if (m_prevCommand == 'Q' || m_prevCommand == 'T') {
                    control = QPointF(2 * m_current.x() - m_lastQuadControl.x(),
                                      2 * m_current.y() - m_lastQuadControl.y());
                }
                m_path.quadTo(control, end);
                m_lastQuadControl = control;
                m_current = end;
                m_prevCommand = 'T';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void handleArc(bool relative)
        {
            constexpr int paramsPerArc = 7;
            while (true) {
                double values[paramsPerArc] = {};
                bool ok = true;
                for (int i = 0; i < paramsPerArc; ++i) {
                    ok &= readNumber(values[i]);
                }
                if (!ok) {
                    break;
                }
                QPointF end(values[5], values[6]);
                if (relative) {
                    end.setX(m_current.x() + end.x());
                    end.setY(m_current.y() + end.y());
                }
                m_path.lineTo(end);
                m_current = end;
                m_prevCommand = 'A';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void skipUnknown()
        {
            while (m_index < m_data.size() && !isCommand(m_data[m_index])) {
                ++m_index;
            }
        }
    };

    QPainterPath buildPainterPath(const std::string& data)
    {
        SvgPathBuilder builder(data);
        return builder.build();
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
    QFont font;
    if (!style.fontFamily.empty()) {
        font.setFamily(QString::fromStdString(style.fontFamily));
    }
    else {
        font.setFamily(QStringLiteral("Times New Roman"));
    }
    font.setPointSizeF(style.fontSize > 0 ? style.fontSize : 16.0);
    SVGPointF pos = text.getPosition();
    QPainterPath path;
    path.addText(QPointF(pos.x, pos.y), font, QString::fromStdString(text.getText()));
    drawPath(path, style, text.getTransform());
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
void QtRenderer::visit(SVGPath& path)
{
    if (!prepareForDrawing(path.getStyle())) {
        return;
    }

    const auto& d = path.getPath();
    if (d.empty()) {
        return;
    }

    QPainterPath qPath = buildPainterPath(d);
    if (qPath.isEmpty()) {
        return;
    }
    drawPath(qPath, path.getStyle(), path.getTransform());
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
