#include "SVGElement.h"

#include <algorithm>

/**
 * @brief Sets the style of this SVG element.
 *
 * Algorithm:
 * - Stores the style object directly
 * - Style includes fill, stroke, opacity, font, and display properties
 * - This style can be inherited by child elements
 *
 * @param style The style to apply to this element
 */
void SVGElement::setStyle(const SVGStyle& style) { m_style = style; }

/**
 * @brief Sets the transform of this SVG element.
 *
 * Algorithm:
 * - Stores the transform matrix directly
 * - Transform includes translation, rotation, scaling, and other affine transformations
 * - Transforms are applied in the order: parent transforms, then this transform
 *
 * @param transform The transform to apply to this element
 */
void SVGElement::setTransform(const SVGTransform& transform) { m_transform = transform; }

/**
 * @brief Gets a mutable reference to the element's style.
 *
 * @return Reference to the style object (allows modification)
 */
SVGStyle& SVGElement::getStyle() { return m_style; }

/**
 * @brief Gets a const reference to the element's style.
 *
 * @return Const reference to the style object (read-only)
 */
const SVGStyle& SVGElement::getStyle() const { return m_style; }

/**
 * @brief Gets a mutable reference to the element's transform.
 *
 * @return Reference to the transform object (allows modification)
 */
SVGTransform& SVGElement::getTransform() { return m_transform; }

/**
 * @brief Gets a const reference to the element's transform.
 *
 * @return Const reference to the transform object (read-only)
 */
const SVGTransform& SVGElement::getTransform() const { return m_transform; }

/**
 * @brief Calculates the world bounding box (after applying transforms).
 *
 * Algorithm:
 * 1. Get the local bounding box (before transforms) by calling localBox()
 * 2. Extract the four corners of the local bounding box:
 *    - p1: top-left (x, y)
 *    - p2: top-right (x + width, y)
 *    - p3: bottom-right (x + width, y + height)
 *    - p4: bottom-left (x, y + height)
 * 3. Transform each corner using the element's transform matrix
 * 4. Find the axis-aligned bounding box of the transformed corners:
 *    - minX = minimum of all transformed X coordinates
 *    - minY = minimum of all transformed Y coordinates
 *    - maxX = maximum of all transformed X coordinates
 *    - maxY = maximum of all transformed Y coordinates
 * 5. Return rectangle: (minX, minY, maxX-minX, maxY-minY)
 *
 * Why transform corners instead of the box directly?
 * - After rotation or non-uniform scaling, the transformed shape is not axis-aligned
 * - We need to find the smallest axis-aligned box that contains the transformed shape
 * - Transforming the four corners and finding their min/max gives us this box
 *
 * This method works for any affine transformation (translation, rotation, scaling, skewing).
 *
 * @return SVGRectF representing the axis-aligned bounding box in world coordinates
 */
SVGRectF SVGElement::worldBox() const
{

    SVGRectF box = this->localBox();

    // Extract the four corners of the local bounding box
    SVGPointF p1 = {box.x, box.y};
    SVGPointF p2 = {box.x + box.width, box.y};
    SVGPointF p3 = {box.x + box.width, box.y + box.height};
    SVGPointF p4 = {box.x, box.y + box.height};

    // Transform each corner using the element's transform matrix
    SVGPointF t_p1 = m_transform.map(p1);
    SVGPointF t_p2 = m_transform.map(p2);
    SVGPointF t_p3 = m_transform.map(p3);
    SVGPointF t_p4 = m_transform.map(p4);

    // Find the axis-aligned bounding box of the transformed corners
    SVGNumber minX = std::min({t_p1.x, t_p2.x, t_p3.x, t_p4.x});
    SVGNumber minY = std::min({t_p1.y, t_p2.y, t_p3.y, t_p4.y});
    SVGNumber maxX = std::max({t_p1.x, t_p2.x, t_p3.x, t_p4.x});
    SVGNumber maxY = std::max({t_p1.y, t_p2.y, t_p3.y, t_p4.y});

    return {minX, minY, maxX - minX, maxY - minY};
}
