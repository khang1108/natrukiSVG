#include "SVGRect.h"

/**
 * @brief Constructor for SVG rectangle element.
 *
 * Algorithm:
 * - Stores the rectangle's position (x, y), size (width, height), and corner radii
 * - The rectangle is defined by its top-left corner (x, y) and dimensions (width, height)
 * - rx and ry are optional corner radii for rounded rectangles (0 = sharp corners)
 *
 * @param rect Rectangle definition: {x, y, width, height}
 * @param rx X-radius for rounded corners (0 = no rounding)
 * @param ry Y-radius for rounded corners (0 = no rounding)
 */
SVGRect::SVGRect(const SVGRectF& rect, SVGNumber rx, SVGNumber ry)
  : m_rect(rect), m_rx(rx), m_ry(ry)
{
}

/**
 * @brief Returns the local bounding box of the rectangle (before transforms).
 *
 * Algorithm:
 * - For a rectangle, the local bounding box is simply the rectangle itself
 * - No calculation needed - just return the stored rectangle
 * - This is the bounding box in the element's local coordinate space
 *
 * @return SVGRectF representing the rectangle's bounds in local coordinates
 */
SVGRectF SVGRect::localBox() const { return m_rect; }

/**
 * @brief Returns the world bounding box (after applying transforms).
 *
 * Algorithm:
 * - Delegates to SVGElement::worldBox() which:
 *   1. Gets localBox() (the rectangle itself)
 *   2. Transforms all four corners using the element's transform matrix
 *   3. Calculates the axis-aligned bounding box of the transformed corners
 *
 * This accounts for rotation, scaling, translation, and other transforms.
 *
 * @return SVGRectF representing the rectangle's bounds in world coordinates
 */
SVGRectF SVGRect::worldBox() const { return SVGElement::worldBox(); }