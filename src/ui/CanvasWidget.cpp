#include "ui/CanvasWidget.h"

#include "svg/SVGDocument.h"
#include "svg/SVGElement.h"
#include "ui/QtRenderer.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStyleOption>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr double kZoomStep = 1.2;
    constexpr double kMinScale = 0.001; // Allow much smaller scale for very large SVGs
    constexpr double kMaxScale = 100.0; // Allow larger scale for very small details
} // namespace

/**
 * @brief Constructor for CanvasWidget - initializes the widget for SVG rendering.
 *
 * Algorithm:
 * - Sets widget attributes for proper rendering:
 *   - WA_TranslucentBackground: Allows transparent backgrounds
 *   - WA_OpaquePaintEvent: Optimizes painting (we fill background ourselves)
 *   - WA_NoSystemBackground: Prevents system from drawing background
 * - Enables mouse tracking for cursor-anchored zoom
 */
CanvasWidget::CanvasWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);
}

CanvasWidget::~CanvasWidget() = default;

/**
 * @brief Sets the SVG document to be displayed.
 *
 * Algorithm:
 * - Takes ownership of the document
 * - Invalidates cached scene bounds (forces recalculation)
 * - Resets zoom/pan/rotate/flip to initial state
 * - Triggers a repaint to show the new document
 *
 * @param document Unique pointer to the SVG document (ownership is transferred)
 */
void CanvasWidget::setDocument(std::unique_ptr<SVGDocument> document)
{
    m_document = std::move(document);
    // Invalidate cached bounds to force recalculation
    m_hasSceneBounds = false;
    m_hasPaintedWithValidSize = false;
    m_fitScale = 1.0;             // Reset fit scale
    m_lastViewportSize = QSize(); // Reset viewport size tracking
    zoomReset();                  // Reset view to initial state
}

/**
 * @brief Checks if a document is currently loaded.
 *
 * @return true if a document is loaded, false otherwise
 */
bool CanvasWidget::hasDocument() const { return static_cast<bool>(m_document); }

/**
 * @brief Renders the SVG document to a QImage.
 *
 * Algorithm:
 * 1. Determine output size (use requested size or widget size)
 * 2. Create QImage with ARGB32_Premultiplied format (supports transparency)
 * 3. Fill image with transparent background
 * 4. Create QPainter and render document to image
 * 5. Return the rendered image
 *
 * This is used for exporting the SVG to PNG or other image formats.
 *
 * @param requestedSize Desired output size (if invalid, uses widget size)
 * @return QImage containing the rendered SVG
 */
QImage CanvasWidget::renderToImage(const QSize& requestedSize)
{
    QSize outputSize = requestedSize.isValid() ? requestedSize : size();
    if (outputSize.isEmpty()) {
        outputSize = QSize(1, 1); // Minimum size
    }

    QImage image(outputSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    if (!m_document) {
        return image; // Empty image if no document
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderDocument(painter, outputSize);
    painter.end();

    return image;
}

/**
 * @brief Zooms in by a fixed factor (centered on viewport center).
 *
 * Algorithm:
 * - Calculates viewport center as anchor point
 * - Applies zoom factor (kZoomStep = 1.2, so 20% zoom in)
 * - Zoom is anchored to viewport center (content scales around center)
 */
void CanvasWidget::zoomIn()
{
    QPointF anchor(width() / 2.0, height() / 2.0);
    applyZoom(kZoomStep, anchor);
}

/**
 * @brief Zooms out by a fixed factor (centered on viewport center).
 *
 * Algorithm:
 * - Calculates viewport center as anchor point
 * - Applies inverse zoom factor (1/kZoomStep, so 20% zoom out)
 * - Zoom is anchored to viewport center
 */
void CanvasWidget::zoomOut()
{
    QPointF anchor(width() / 2.0, height() / 2.0);
    applyZoom(1.0 / kZoomStep, anchor);
}

/**
 * @brief Resets zoom, pan, rotation, and flip to initial state.
 *
 * Algorithm:
 * - Sets scale to 1.0 (no zoom)
 * - Sets pan offset to (0, 0) (no pan)
 * - Sets rotation to 0.0 (no rotation)
 * - Sets flip to false (no flip)
 * - Recalculates scene bounds
 * - Triggers repaint
 */
void CanvasWidget::zoomReset()
{
    m_scale = 1.0;
    m_panOffset = QPointF(0, 0);
    m_rotation = 0.0;
    m_isFlipped = false;
    // Invalidate fitScale so it gets recalculated with current viewport size
    m_fitScale = 0.0;
    m_lastViewportSize = QSize();
    updateSceneBounds();
    update();
}

void CanvasWidget::setScale(double scale)
{
    m_scale = scale;
    clampScale();
    update();
}

void CanvasWidget::rotate()
{
    m_rotation = std::fmod(m_rotation + 90.0, 360.0);
    update();
}

/**
 * @brief Toggles horizontal flip (mirror) of the view.
 *
 * Algorithm:
 * - Toggles m_isFlipped flag
 * - Triggers repaint
 * - Flip is applied in buildViewTransform() by scaling X by -1
 */
void CanvasWidget::flip()
{
    m_isFlipped = !m_isFlipped;
    update();
}

/**
 * @brief Handles paint events - renders the SVG document to the widget.
 *
 * Algorithm:
 * 1. Create QPainter for this widget
 * 2. Enable antialiasing for smooth rendering
 * 3. Fill background with white (CompositionMode_Source for opaque fill)
 * 4. Switch to SourceOver mode for drawing SVG content
 * 5. If no document: show placeholder text
 * 6. If document exists:
 *    - Ensure scene bounds are recalculated on first paint with valid size
 *      (fixes issues where elements render incorrectly on first paint)
 *    - Render the document using the current view transform
 *
 * Background:
 * - Uses white background (Qt::white) for better visibility
 * - CompositionMode_Source ensures background is fully opaque
 *
 * @param event The paint event (unused, but required by Qt)
 */
void CanvasWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // Fill background with white (opaque)
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::white);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (!m_document) {
        // Show placeholder text when no document is loaded
        painter.setPen(QPen(QColor(200, 200, 200)));
        painter.drawText(rect(), Qt::AlignCenter, tr("Upload an SVG file to begin"));
        return;
    }

    // Ensure scene bounds are recalculated on first paint with valid size
    // This fixes the issue where elements with transforms render incorrectly on first paint
    // We need to recalculate even if bounds were already calculated, because they might
    // have been calculated before the widget had a valid size
    if (size().isValid() && !m_hasPaintedWithValidSize) {
        m_hasSceneBounds = false; // Force recalculation
        updateSceneBounds();
        m_hasPaintedWithValidSize = true;
    }

    renderDocument(painter, size());
}

/**
 * @brief Handles mouse press events - starts panning.
 *
 * Algorithm:
 * - If left mouse button is pressed:
 *   1. Set panning flag to true
 *   2. Store current mouse position as last position
 *   3. Change cursor to closed hand (indicates dragging)
 * - This initiates panning mode for dragging the view
 *
 * @param event The mouse press event
 */
void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor); // Show dragging cursor
    }
    QWidget::mousePressEvent(event);
}

/**
 * @brief Handles mouse move events - performs panning.
 *
 * Algorithm:
 * - If panning is active:
 *   1. Calculate mouse movement delta (current position - last position)
 *   2. Add delta to pan offset (moves the view)
 *   3. Update last mouse position
 *   4. Trigger repaint to show new position
 * - This allows dragging the SVG content around the viewport
 *
 * @param event The mouse move event
 */
void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        // Pan speed should be consistent regardless of zoom level
        // The delta is already in screen pixels, which is correct for pan offset
        m_panOffset += delta; // Move view by mouse delta
        update();             // Repaint
    }
    QWidget::mouseMoveEvent(event);
}

/**
 * @brief Handles mouse release events - stops panning.
 *
 * Algorithm:
 * - If left mouse button is released:
 *   1. Set panning flag to false
 *   2. Change cursor back to arrow
 * - This ends panning mode
 *
 * @param event The mouse release event
 */
void CanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor); // Restore normal cursor
    }
    QWidget::mouseReleaseEvent(event);
}

/**
 * @brief Handles mouse wheel events - performs cursor-anchored zoom.
 *
 * Algorithm:
 * 1. Calculate zoom factor from wheel delta:
 *    - factor = 1.0015^delta (exponential scaling)
 *    - Positive delta (scroll up) = zoom in
 *    - Negative delta (scroll down) = zoom out
 * 2. Apply zoom anchored to cursor position
 *    - The point under the cursor stays under the cursor after zoom
 *    - This provides intuitive zoom behavior
 *
 * Zoom factor calculation:
 * - Uses exponential function for smooth zoom
 * - 1.0015 provides fine-grained control
 * - Larger deltas result in larger zoom changes
 *
 * @param event The wheel event containing scroll delta and cursor position
 */
void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    if (!event->angleDelta().isNull()) {
        // Calculate zoom factor from wheel delta (exponential scaling)
        double factor = std::pow(1.0015, event->angleDelta().y());
        applyZoom(factor, event->position()); // Zoom anchored to cursor
    }
    QWidget::wheelEvent(event);
}

/**
 * @brief Calculates the bounding box of the entire SVG document.
 *
 * Algorithm:
 * 1. First, check if the SVG has a viewBox attribute
 *    - If viewBox is valid (width > 0 and height > 0), use it as the bounds
 *    - viewBox defines the coordinate system and visible area of the SVG
 * 2. If no viewBox, calculate bounds from actual content:
 *    - Iterate through all top-level children of the document
 *    - For each child, get its worldBox() (bounding box after all transforms)
 *    - Track the minimum and maximum X and Y coordinates across all children
 *    - Return a rectangle that encompasses all elements
 *
 * The bounds are used to:
 * - Fit the SVG content to the viewport (initial zoom)
 * - Calculate the view transformation matrix
 * - Determine the visible area for panning and zooming
 *
 * @return SVGRectF representing the bounding box of the document
 */
SVGRectF CanvasWidget::calculateDocumentBounds() const
{
    if (!m_document) {
        return {0, 0, 0, 0};
    }

    // Try to use viewBox if available and reasonable
    // But also calculate bounds from content to see which is better
    const SVGRectF& viewBox = m_document->getViewBox();

    // Calculate bounds from actual content (after transforms)
    const auto& children = m_document->getChildren();
    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();
    bool hasBounds = false;

    // Iterate through all children and find the union of their bounding boxes
    for (const auto& child : children) {
        if (!child)
            continue;
        SVGRectF box = child->worldBox(); // Get bounds after transforms
        // Accept bounds even if width or height is 0 (points/lines)
        // Only skip if both are negative or invalid
        if (box.width < 0 || box.height < 0 || !std::isfinite(box.x) || !std::isfinite(box.y))
            continue; // Skip invalid bounds
        hasBounds = true;
        // Update min/max coordinates
        minX = std::min(minX, box.x);
        minY = std::min(minY, box.y);
        maxX = std::max(maxX, box.x + std::max(box.width, 0.0));
        maxY = std::max(maxY, box.y + std::max(box.height, 0.0));
    }

    if (!hasBounds) {
        // No content bounds found, try using viewBox as fallback
        if (viewBox.width > 0 && viewBox.height > 0) {
            return viewBox;
        }
        return {0, 0, 0, 0}; // No valid bounds found
    }

    SVGRectF contentBounds = {minX, minY, maxX - minX, maxY - minY};

    // Ensure bounds have positive dimensions
    if (contentBounds.width <= 0) {
        contentBounds.width = 1.0;
    }
    if (contentBounds.height <= 0) {
        contentBounds.height = 1.0;
    }

    // Use content bounds (they include transforms, which is what we want for rendering)
    // This ensures we can see all the transformed content, even if it's outside the viewBox
    return contentBounds;
}

/**
 * @brief Builds the view transformation matrix that maps SVG coordinates to screen coordinates.
 *
 * Algorithm (transformation order matters - applied right to left):
 * 1. Translate to scene center: Move origin to center of SVG content
 *    - Translate by -(bounds.x + bounds.width/2, bounds.y + bounds.height/2)
 * 2. Scale: Apply zoom level and fit-to-viewport scaling
 *    - Calculate fitScale to fit content in viewport (90% to leave margin)
 *    - Apply user zoom (m_scale) multiplied by fitScale
 * 3. Rotate: Apply user rotation (m_rotation degrees)
 * 4. Flip: If flipped, mirror horizontally (scale X by -1)
 * 5. Pan: Apply user pan offset (m_panOffset)
 * 6. Translate to viewport center: Move origin to center of screen
 *    - Translate by (viewportWidth/2, viewportHeight/2)
 *
 * The transformation chain:
 * T_viewport_center * T_pan * T_flip * T_rotate * T_scale * T_scene_center
 *
 * This ensures:
 * - Content is centered and fits in the viewport initially
 * - Zoom is anchored correctly (scales around center)
 * - Pan moves the content relative to the viewport
 * - Rotation and flip are applied around the center
 *
 * @param viewportSize Size of the viewport (widget size)
 * @return QTransform representing the complete view transformation
 */
QTransform CanvasWidget::buildViewTransform(const QSize& viewportSize) const
{
    QTransform transform;
    if (viewportSize.isEmpty()) {
        return transform; // Empty viewport, return identity
    }

    // Get scene bounds (SVG content bounds)
    SVGRectF bounds = sceneBounds();
    if (bounds.width <= 0 || bounds.height <= 0) {
        // No valid bounds, use viewport size as fallback
        bounds = {0, 0, static_cast<SVGNumber>(viewportSize.width()),
                  static_cast<SVGNumber>(viewportSize.height())};
    }

    // Recalculate fitScale if viewport size changed or if it hasn't been calculated yet
    double fitScale = m_fitScale;
    if (viewportSize != m_lastViewportSize || m_fitScale <= 0.0 || !std::isfinite(m_fitScale)) {
        // Calculate scale to fit content in viewport (98% to leave small margin)
        // Use a larger margin to ensure full content is visible
        if (bounds.width > 0 && bounds.height > 0 && std::isfinite(bounds.width) &&
            std::isfinite(bounds.height)) {
            double fitScaleX = viewportSize.width() / bounds.width;
            double fitScaleY = viewportSize.height() / bounds.height;
            fitScale =
                0.98 * std::min(fitScaleX, fitScaleY); // Use smaller scale to fit both dimensions
            // Ensure fitScale is reasonable (not too small or too large)
            if (fitScale < 1e-10 || fitScale > 1e10 || !std::isfinite(fitScale)) {
                fitScale = 1.0; // Fallback for extreme values
            }
        }
        else {
            fitScale = 1.0; // Fallback if bounds are invalid
        }
        // Update stored values (need const_cast because this is const method, but we cache values)
        const_cast<CanvasWidget*>(this)->m_fitScale = fitScale;
        const_cast<CanvasWidget*>(this)->m_lastViewportSize = viewportSize;
    }

    transform.translate(viewportSize.width() / 2.0, viewportSize.height() / 2.0);
    transform.translate(m_panOffset.x(), m_panOffset.y());

    transform.rotate(m_rotation);

    if (m_isFlipped) {
        transform.scale(-1.0, 1.0);
    }
    transform.scale(m_scale * fitScale, m_scale * fitScale);
    transform.translate(-(bounds.x + bounds.width / 2.0), -(bounds.y + bounds.height / 2.0));
    return transform;

    // Đã fix phần rotate và flip
}

/**
 * @brief Renders the SVG document using the current view transform.
 *
 * Algorithm:
 * 1. Save painter state
 * 2. Apply view transformation (zoom, pan, rotate, flip)
 * 3. Create QtRenderer and render document (visits all elements)
 * 4. Restore painter state
 *
 * The view transform maps SVG coordinates to screen coordinates,
 * applying all user interactions (zoom, pan, rotate, flip).
 *
 * @param painter The QPainter to use for rendering
 * @param viewportSize Size of the viewport (widget size)
 */
void CanvasWidget::renderDocument(QPainter& painter, const QSize& viewportSize)
{
    if (!m_document) {
        return; // No document to render
    }

    painter.save(); // Save current state
    // Apply view transformation (zoom, pan, rotate, flip)
    painter.setWorldTransform(buildViewTransform(viewportSize));

    // Render document using Visitor pattern
    // Render document using Visitor pattern
    QtRenderer renderer(&painter, *m_document);
    m_document->draw(renderer);

    painter.restore(); // Restore state
}

/**
 * @brief Applies zoom anchored to a specific viewport point (cursor-anchored zoom).
 *
 * Algorithm (cursor-anchored zoom):
 * 1. Validate zoom factor (must be finite and positive)
 * 2. Get current view transform
 * 3. Invert transform to find world coordinate under viewport anchor (cursor position)
 * 4. Apply zoom factor to scale
 * 5. Clamp scale to valid range [kMinScale, kMaxScale]
 * 6. Calculate new view transform with updated scale
 * 7. Project world anchor back to viewport using new transform
 * 8. Adjust pan offset so the world point stays under the viewport anchor
 *
 * Why cursor-anchored zoom?
 * - The point under the cursor stays under the cursor after zoom
 * - Provides intuitive zoom behavior (like Google Maps)
 * - User can zoom into a specific area by placing cursor there
 *
 * Formula:
 * - worldAnchor = inverse(currentTransform) * viewportAnchor
 * - newScale = oldScale * factor
 * - projected = newTransform * worldAnchor
 * - panOffset += viewportAnchor - projected
 *
 * @param factor Zoom factor (>1.0 = zoom in, <1.0 = zoom out)
 * @param viewportAnchor Viewport point to anchor zoom to (e.g., cursor position)
 */
void CanvasWidget::applyZoom(double factor, const QPointF& viewportAnchor)
{
    if (!m_document) {
        return; // No document to zoom
    }

    if (!std::isfinite(factor) || factor <= 0.0) {
        return; // Invalid zoom factor
    }

    QSize viewportSize = size();
    QTransform currentTransform;
    QTransform inverseTransform;
    QPointF worldAnchor;
    bool hasAnchor = false;

    // Find world coordinate under viewport anchor (cursor position)
    if (!viewportSize.isEmpty()) {
        currentTransform = buildViewTransform(viewportSize);
        bool invertible = false;
        inverseTransform = currentTransform.inverted(&invertible);
        hasAnchor = invertible;
        if (hasAnchor) {
            // Convert viewport point to world coordinate
            worldAnchor = inverseTransform.map(viewportAnchor);
        }
    }

    // Apply zoom factor
    m_scale *= factor;
    clampScale(); // Clamp to valid range

    // Adjust pan so world point stays under viewport anchor
    if (hasAnchor) {
        QTransform newTransform = buildViewTransform(viewportSize);
        QPointF projected = newTransform.map(worldAnchor); // Project back to viewport
        QPointF delta = viewportAnchor - projected;        // Calculate adjustment
        m_panOffset += delta;                              // Adjust pan
    }

    update(); // Repaint
}

/**
 * @brief Clamps the zoom scale to valid range [kMinScale, kMaxScale].
 *
 * Algorithm:
 * - If scale < kMinScale: set to kMinScale (prevent zooming out too far)
 * - If scale > kMaxScale: set to kMaxScale (prevent zooming in too far)
 * - Otherwise: keep scale unchanged
 *
 * Limits:
 * - kMinScale = 0.05 (5% - minimum zoom out)
 * - kMaxScale = 40.0 (4000% - maximum zoom in)
 *
 * This prevents:
 * - Zooming out so far that content becomes invisible
 * - Zooming in so far that floating-point precision issues occur
 */
void CanvasWidget::clampScale()
{
    // Clamp scale to valid range, but allow very small scales for very large SVGs
    if (m_scale < kMinScale) {
        m_scale = kMinScale; // Clamp to minimum
    }
    else if (m_scale > kMaxScale) {
        m_scale = kMaxScale; // Clamp to maximum
    }
    // Also ensure scale is finite
    if (!std::isfinite(m_scale)) {
        m_scale = 1.0; // Reset to default if invalid
    }
}

/**
 * @brief Updates the cached scene bounds (bounding box of the SVG document).
 *
 * Algorithm:
 * 1. If no document: set bounds to empty and mark as invalid
 * 2. If document exists: calculate bounds using calculateDocumentBounds()
 * 3. Mark bounds as valid if width and height are both > 0
 *
 * Caching:
 * - Bounds are cached to avoid recalculating on every paint
 * - Cache is invalidated when document changes (in setDocument)
 * - Cache is recalculated when needed (first paint, resize, etc.)
 *
 * The bounds are used for:
 * - Fitting content to viewport (initial zoom)
 * - Calculating view transformation
 * - Determining visible area
 */
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

void CanvasWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    // Force recalculation of scene bounds when widget is first shown
    // to ensure correct initial rendering with proper widget size
    if (m_document && size().isValid()) {
        // Invalidate and recalculate bounds to ensure they're correct
        m_hasSceneBounds = false;
        m_hasPaintedWithValidSize = false; // Reset flag to force recalculation on next paint
        updateSceneBounds();
        update();
    }
}

void CanvasWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // Update when widget is resized to ensure correct transform
    if (m_document && event->size().isValid()) {
        // Invalidate fitScale so it gets recalculated with new viewport size
        m_fitScale = 0.0;
        m_lastViewportSize = QSize();
        update();
    }
}