#include "SVGPolygon.h"

#include <algorithm>
#include <limits>

/**
 * @brief Constructor for SVG polygon element.
 *
 * Algorithm:
 * - Stores a list of points that define the polygon
 * - A polygon is a closed shape formed by connecting points in order
 * - The last point is automatically connected to the first point
 * - Points are stored in the order they appear in the SVG
 *
 * @param points Vector of points defining the polygon vertices
 */
SVGPolygon::SVGPolygon(const std::vector<SVGPointF>& points) : m_points(points) {}

/**
 * @brief Returns the local bounding box of the polygon (before transforms).
 *
 * Algorithm:
 * 1. If polygon has no points, return empty bounding box
 * 2. Initialize min/max values to extreme values (infinity)
 * 3. Iterate through all points in the polygon
 * 4. For each point, update minX, minY, maxX, maxY
 * 5. Return rectangle from (minX, minY) with size (maxX-minX, maxY-minY)
 *
 * Formula:
 * - minX = minimum of all point.x values
 * - minY = minimum of all point.y values
 * - maxX = maximum of all point.x values
 * - maxY = maximum of all point.y values
 * - width = maxX - minX
 * - height = maxY - minY
 *
 * This finds the smallest axis-aligned rectangle that contains all vertices.
 * Note: The bounding box may be larger than the actual polygon if it's rotated.
 *
 * @return SVGRectF representing the axis-aligned bounding box of the polygon
 */
SVGRectF SVGPolygon::localBox() const
{

    if (m_points.empty()) {
        return {0, 0, 0, 0};
    }

    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();

    for (const auto& p : m_points) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }

    return {minX, minY, maxX - minX, maxY - minY};
}

/**
 * @brief Returns the world bounding box (after applying transforms).
 *
 * Algorithm:
 * - Delegates to SVGElement::worldBox() which:
 *   1. Gets localBox() (the rectangle containing all vertices)
 *   2. Transforms all four corners using the element's transform matrix
 *   3. Calculates the axis-aligned bounding box of the transformed corners
 *
 * This accounts for rotation, scaling, translation, and other transforms.
 *
 * @return SVGRectF representing the polygon's bounds in world coordinates
 */
SVGRectF SVGPolygon::worldBox() const { return SVGElement::worldBox(); }