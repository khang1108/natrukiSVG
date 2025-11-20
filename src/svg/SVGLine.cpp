#include "SVGLine.h"

#include <algorithm>

/**
 * @brief Constructor for SVG line element.
 *
 * Algorithm:
 * - Stores the two endpoints of the line
 * - A line is defined by two points: (x1, y1) and (x2, y2)
 * - The line extends from p1 to p2
 *
 * @param p1 First endpoint of the line: {x1, y1}
 * @param p2 Second endpoint of the line: {x2, y2}
 */
SVGLine::SVGLine(const SVGPointF& p1, const SVGPointF& p2) : m_p1(p1), m_p2(p2) {}

/**
 * @brief Returns the local bounding box of the line (before transforms).
 *
 * Algorithm:
 * - A line's bounding box is the smallest axis-aligned rectangle containing both endpoints
 * - Find the minimum and maximum X and Y coordinates of the two points
 * - The bounding box is the rectangle from (minX, minY) to (maxX, maxY)
 *
 * Formula:
 * - minX = min(p1.x, p2.x)
 * - minY = min(p1.y, p2.y)
 * - maxX = max(p1.x, p2.x)
 * - maxY = max(p1.y, p2.y)
 * - width = maxX - minX
 * - height = maxY - minY
 *
 * Note: For horizontal or vertical lines, one dimension will be 0.
 *
 * @return SVGRectF representing the axis-aligned bounding box of the line
 */
SVGRectF SVGLine::localBox() const
{

    SVGNumber minX = std::min(m_p1.x, m_p2.x);
    SVGNumber minY = std::min(m_p1.y, m_p2.y);
    SVGNumber maxX = std::max(m_p1.x, m_p2.x);
    SVGNumber maxY = std::max(m_p1.y, m_p2.y);

    return {minX, minY, maxX - minX, maxY - minY};
}

/**
 * @brief Returns the world bounding box (after applying transforms).
 *
 * Algorithm:
 * - Delegates to SVGElement::worldBox() which:
 *   1. Gets localBox() (the rectangle containing both endpoints)
 *   2. Transforms all four corners using the element's transform matrix
 *   3. Calculates the axis-aligned bounding box of the transformed corners
 *
 * This accounts for rotation, scaling, translation, and other transforms.
 *
 * @return SVGRectF representing the line's bounds in world coordinates
 */
SVGRectF SVGLine::worldBox() const { return SVGElement::worldBox(); }