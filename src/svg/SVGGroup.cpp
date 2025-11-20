#include "SVGGroup.h"

#include <algorithm>
#include <limits>

/**
 * @brief Constructor for SVG group element.
 *
 * Algorithm:
 * - Initializes an empty group with no children
 * - Groups are containers that can hold multiple child elements
 * - Groups can have their own style and transform, which are inherited/applied to children
 */
SVGGroup::SVGGroup() {}

/**
 * @brief Adds a child element to this group.
 *
 * Algorithm:
 * - Takes ownership of the child element using std::move
 * - Adds the child to the group's children vector
 * - The child becomes part of the group's hierarchy
 *
 * @param child Unique pointer to the child element to add (ownership is transferred)
 */
void SVGGroup::addChild(std::unique_ptr<SVGElement> child)
{

    m_children.push_back(std::move(child));
}

/**
 * @brief Implements the Visitor pattern accept method for groups.
 *
 * Algorithm (Visitor Pattern):
 * 1. Call visitor.visitGroupBegin() to notify visitor that we're entering a group
 *    - This allows the visitor to save state, apply group transforms, etc.
 * 2. Iterate through all children in the group
 * 3. For each child, call child->accept(visitor)
 *    - This recursively visits all descendants
 *    - Each child handles its own visit logic
 * 4. Call visitor.visitGroupEnd() to notify visitor that we're leaving the group
 *    - This allows the visitor to restore state, etc.
 *
 * This implements the Visitor pattern, allowing operations (like rendering) to be
 * separated from the data structure (the SVG tree).
 *
 * @param visitor The visitor object that will process this group and its children
 */
void SVGGroup::accept(NodeVisitor& visitor)
{

    visitor.visitGroupBegin(*this);

    for (const auto& child : m_children) {
        if (child) {
            child->accept(visitor);
        }
    }

    visitor.visitGroupEnd(*this);
}

/**
 * @brief Calculates the local bounding box of the group (before group's own transform).
 *
 * Algorithm:
 * 1. If group has no children, return empty bounding box
 * 2. Initialize min/max values to extreme values (infinity)
 * 3. Iterate through all children in the group
 * 4. For each child:
 *    - Get the child's worldBox() (bounding box after child's own transforms)
 *    - Note: We use worldBox() because children may have their own transforms
 *    - Update minX, minY, maxX, maxY to include the child's bounds
 * 5. Return rectangle from (minX, minY) with size (maxX-minX, maxY-minY)
 *
 * Formula:
 * - minX = minimum of all child worldBox.x values
 * - minY = minimum of all child worldBox.y values
 * - maxX = maximum of all (child worldBox.x + child worldBox.width) values
 * - maxY = maximum of all (child worldBox.y + child worldBox.height) values
 * - width = maxX - minX
 * - height = maxY - minY
 *
 * Important: This calculates the union of all children's bounding boxes.
 * The group's own transform is NOT applied here (that's done in worldBox()).
 *
 * @return SVGRectF representing the bounding box containing all children
 */
SVGRectF SVGGroup::localBox() const
{

    if (m_children.empty()) {
        return {0, 0, 0, 0};
    }

    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();

    for (const auto& child : m_children) {
        if (!child)
            continue;

        // Get child's world box (after child's own transforms)
        SVGRectF childWorldBox = child->worldBox();

        if (childWorldBox.width >= 0 && childWorldBox.height >= 0) {

            minX = std::min(minX, childWorldBox.x);
            minY = std::min(minY, childWorldBox.y);
            maxX = std::max(maxX, childWorldBox.x + childWorldBox.width);
            maxY = std::max(maxY, childWorldBox.y + childWorldBox.height);
        }
    }

    if (minX == std::numeric_limits<SVGNumber>::infinity()) {
        return {0, 0, 0, 0}; // No valid children found
    }

    return {minX, minY, maxX - minX, maxY - minY};
}

/**
 * @brief Returns the world bounding box (after applying group's transform).
 *
 * Algorithm:
 * - Delegates to SVGElement::worldBox() which:
 *   1. Gets localBox() (the union of all children's bounds)
 *   2. Transforms all four corners using the group's transform matrix
 *   3. Calculates the axis-aligned bounding box of the transformed corners
 *
 * This accounts for the group's rotation, scaling, translation, and other transforms.
 *
 * @return SVGRectF representing the group's bounds in world coordinates
 */
SVGRectF SVGGroup::worldBox() const { return SVGElement::worldBox(); }