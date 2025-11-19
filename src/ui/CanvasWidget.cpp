#include "ui/CanvasWidget.h"

#include "svg/SVGDocument.h"
#include "svg/SVGElement.h"
#include "ui/QtRenderer.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr double kZoomStep = 1.2;
    constexpr double kMinScale = 0.05;
    constexpr double kMaxScale = 40.0;
} // namespace

CanvasWidget::CanvasWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);
}

CanvasWidget::~CanvasWidget() = default;

void CanvasWidget::setDocument(std::unique_ptr<SVGDocument> document)
{
    m_document = std::move(document);
    zoomReset();
}

bool CanvasWidget::hasDocument() const { return static_cast<bool>(m_document); }

QImage CanvasWidget::renderToImage(const QSize& requestedSize)
{
    QSize outputSize = requestedSize.isValid() ? requestedSize : size();
    if (outputSize.isEmpty()) {
        outputSize = QSize(1, 1);
    }

    QImage image(outputSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    if (!m_document) {
        return image;
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderDocument(painter, outputSize);
    painter.end();

    return image;
}

void CanvasWidget::zoomIn()
{
    m_scale *= kZoomStep;
    clampScale();
    update();
}

void CanvasWidget::zoomOut()
{
    m_scale /= kZoomStep;
    clampScale();
    update();
}

void CanvasWidget::zoomReset()
{
    m_scale = 1.0;
    m_panOffset = QPointF(0, 0);
    m_rotation = 0.0;
    m_isFlipped = false;
    updateSceneBounds();
    update();
}

void CanvasWidget::rotate()
{
    m_rotation = std::fmod(m_rotation + 90.0, 360.0);
    update();
}

void CanvasWidget::flip()
{
    m_isFlipped = !m_isFlipped;
    update();
}

void CanvasWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::white);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (!m_document) {
        painter.setPen(QPen(QColor(200, 200, 200)));
        painter.drawText(rect(), Qt::AlignCenter, tr("Upload an SVG file to begin"));
        return;
    }

    renderDocument(painter, size());
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(event);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_panOffset += delta;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
    QWidget::mouseReleaseEvent(event);
}

void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    if (!event->angleDelta().isNull()) {
        double factor = std::pow(1.0015, event->angleDelta().y());
        m_scale *= factor;
        clampScale();
        update();
    }
    QWidget::wheelEvent(event);
}

SVGRectF CanvasWidget::calculateDocumentBounds() const
{
    if (!m_document) {
        return {0, 0, 0, 0};
    }

    const SVGRectF& viewBox = m_document->getViewBox();
    if (viewBox.width > 0 && viewBox.height > 0) {
        return viewBox;
    }

    const auto& children = m_document->getChildren();
    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();
    bool hasBounds = false;

    for (const auto& child : children) {
        if (!child)
            continue;
        SVGRectF box = child->worldBox();
        if (box.width <= 0 && box.height <= 0)
            continue;
        hasBounds = true;
        minX = std::min(minX, box.x);
        minY = std::min(minY, box.y);
        maxX = std::max(maxX, box.x + box.width);
        maxY = std::max(maxY, box.y + box.height);
    }

    if (!hasBounds) {
        return {0, 0, 0, 0};
    }

    return {minX, minY, maxX - minX, maxY - minY};
}

QTransform CanvasWidget::buildViewTransform(const QSize& viewportSize) const
{
    QTransform transform;
    if (viewportSize.isEmpty()) {
        return transform;
    }

    SVGRectF bounds = sceneBounds();
    if (bounds.width <= 0 || bounds.height <= 0) {
        bounds = {0, 0, static_cast<SVGNumber>(viewportSize.width()),
                  static_cast<SVGNumber>(viewportSize.height())};
    }

    double fitScaleX = viewportSize.width() / bounds.width;
    double fitScaleY = viewportSize.height() / bounds.height;
    double fitScale = 0.9 * std::min(fitScaleX, fitScaleY);

    transform.translate(viewportSize.width() / 2.0, viewportSize.height() / 2.0);
    transform.translate(m_panOffset.x(), m_panOffset.y());
    if (m_isFlipped) {
        transform.scale(-1.0, 1.0);
    }
    transform.rotate(m_rotation);
    transform.scale(m_scale * fitScale, m_scale * fitScale);
    transform.translate(-(bounds.x + bounds.width / 2.0), -(bounds.y + bounds.height / 2.0));
    return transform;
}

void CanvasWidget::renderDocument(QPainter& painter, const QSize& viewportSize)
{
    if (!m_document) {
        return;
    }

    painter.save();
    painter.setWorldTransform(buildViewTransform(viewportSize));

    QtRenderer renderer(&painter);
    m_document->draw(renderer);

    painter.restore();
}

void CanvasWidget::clampScale()
{
    if (m_scale < kMinScale) {
        m_scale = kMinScale;
    }
    else if (m_scale > kMaxScale) {
        m_scale = kMaxScale;
    }
}

void CanvasWidget::updateSceneBounds()
{
    if (!m_document) {
        m_sceneBounds = {0, 0, 0, 0};
        m_hasSceneBounds = false;
        return;
    }

    m_sceneBounds = calculateDocumentBounds();
    m_hasSceneBounds = (m_sceneBounds.width > 0 && m_sceneBounds.height > 0);
}

const SVGRectF& CanvasWidget::sceneBounds() const
{
    if (!m_hasSceneBounds && m_document) {
        const_cast<CanvasWidget*>(this)->updateSceneBounds();
    }
    return m_sceneBounds;
}
