#include "SVGEllipse.h"

/**
 * @brief Constructor for SVG ellipse element.
 *
 * Algorithm:
 * - Stores the ellipse's center point and radii
 * - An ellipse is defined by its center (cx, cy) and two radii (rx, ry)
 * - rx is the horizontal radius (half-width)
 * - ry is the vertical radius (half-height)
 * - If rx == ry, the ellipse is a circle
 *
 * @param center Center point of the ellipse: {cx, cy}
 * @param rx Horizontal radius (half-width) of the ellipse
 * @param ry Vertical radius (half-height) of the ellipse
 */
SVGEllipse::SVGEllipse(const SVGPointF& center, SVGNumber rx, SVGNumber ry)
  : m_center(center), m_rx(rx), m_ry(ry)
{
}

/**
 * @brief Returns the local bounding box of the ellipse (before transforms).
 *
 * Algorithm:
 * - An ellipse's bounding box is a rectangle that exactly contains the ellipse
 * - The rectangle is centered on the ellipse's center
 * - Top-left corner: (center.x - rx, center.y - ry)
 * - Size: (2*rx) × (2*ry)
 *
 * Formula:
 * - minX = center.x - rx
 * - minY = center.y - ry
 * - width = 2 * rx
 * - height = 2 * ry
 *
 * @return SVGRectF representing the axis-aligned bounding box of the ellipse
 */
SVGRectF SVGEllipse::localBox() const
{

    return {m_center.x - m_rx, m_center.y - m_ry, m_rx * 2.0, m_ry * 2.0};
}

/**
 * @brief Returns the world bounding box (after applying transforms).
 *
 * Algorithm:
 * - Delegates to SVGElement::worldBox() which:
 *   1. Gets localBox() (the rectangular bounding box)
 *   2. Transforms all four corners using the element's transform matrix
 *   3. Calculates the axis-aligned bounding box of the transformed corners
 *
 * Note: After rotation or non-uniform scaling, the bounding box may be larger
 * than the transformed ellipse, but it will always contain it.
 *
 * @return SVGRectF representing the ellipse's bounds in world coordinates
 */
SVGRectF SVGEllipse::worldBox() const { return SVGElement::worldBox(); }
