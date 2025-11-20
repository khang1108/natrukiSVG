#include "SVGCircle.h"

/**
 * @brief Constructor for SVG circle element.
 *
 * Algorithm:
 * - Stores the circle's center point and radius
 * - A circle is defined by its center (cx, cy) and radius (r)
 * - All points on the circle are equidistant from the center
 *
 * @param center Center point of the circle: {cx, cy}
 * @param radius Radius of the circle (distance from center to edge)
 */
SVGCircle::SVGCircle(const SVGPointF& center, SVGNumber radius) : m_center(center), m_radius(radius)
{
}

/**
 * @brief Returns the local bounding box of the circle (before transforms).
 *
 * Algorithm:
 * - A circle's bounding box is a square that exactly contains the circle
 * - The square is centered on the circle's center
 * - Top-left corner: (center.x - radius, center.y - radius)
 * - Size: diameter × diameter (radius * 2)
 *
 * Formula:
 * - minX = center.x - radius
 * - minY = center.y - radius
 * - width = height = diameter = 2 * radius
 *
 * @return SVGRectF representing the axis-aligned bounding box of the circle
 */
SVGRectF SVGCircle::localBox() const
{
    SVGNumber diameter = m_radius * 2.0;

    return {m_center.x - m_radius, m_center.y - m_radius, diameter, diameter};
}

/**
 * @brief Returns the world bounding box (after applying transforms).
 *
 * Algorithm:
 * - Delegates to SVGElement::worldBox() which:
 *   1. Gets localBox() (the square bounding box)
 *   2. Transforms all four corners using the element's transform matrix
 *   3. Calculates the axis-aligned bounding box of the transformed corners
 *
 * Note: After non-uniform scaling or rotation, the bounding box may be larger
 * than the transformed circle, but it will always contain it.
 *
 * @return SVGRectF representing the circle's bounds in world coordinates
 */
SVGRectF SVGCircle::worldBox() const { return SVGElement::worldBox(); }
