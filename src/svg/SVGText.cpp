#include "SVGText.h"

#include "SVGStyle.h"

/**
 * @brief Constructor for SVG text element.
 *
 * Algorithm:
 * - Stores the text content and its position
 * - Text is positioned at (x, y) where (x, y) is typically the baseline start
 * - The text string is stored as-is for rendering
 *
 * @param pos Position of the text: {x, y} (baseline start position)
 * @param text The text content to display
 */
SVGText::SVGText(const SVGPointF& pos, const std::string& text) : m_position(pos), m_text(text) {}

/**
 * @brief Returns the local bounding box of the text (before transforms).
 *
 * Algorithm:
 * 1. Get font size from style (default to 16.0 if not set)
 * 2. Approximate text width: fontSize * 0.6 * character_count
 *    - Factor 0.6 is an approximation for average character width
 *    - Actual width depends on font, but this provides a reasonable estimate
 * 3. Approximate text height: fontSize (height of one line)
 * 4. Position: (x, y - height) where y is the baseline
 *    - Text baseline is at y, but bounding box starts above it
 *
 * Formula:
 * - width = fontSize * 0.6 * text.length()
 * - height = fontSize
 * - x = position.x
 * - y = position.y - height (text extends upward from baseline)
 *
 * Note: This is an approximation. Accurate text bounds would require:
 * - Font metrics (actual character widths, ascenders, descenders)
 * - Font family and style information
 * - Text rendering engine measurements
 *
 * @return SVGRectF representing the approximate bounding box of the text
 */
SVGRectF SVGText::localBox() const
{
    const SVGStyle& style = getStyle();
    SVGNumber fontSize = style.fontSize > 0.0 ? style.fontSize : 16.0;
    SVGNumber approxWidth = fontSize * 0.6 * static_cast<SVGNumber>(m_text.size());
    SVGNumber approxHeight = fontSize;
    return {m_position.x, m_position.y - approxHeight, approxWidth, approxHeight};
}

/**
 * @brief Returns the world bounding box (after applying transforms).
 *
 * Algorithm:
 * - Delegates to SVGElement::worldBox() which:
 *   1. Gets localBox() (the approximate text bounding box)
 *   2. Transforms all four corners using the element's transform matrix
 *   3. Calculates the axis-aligned bounding box of the transformed corners
 *
 * This accounts for rotation, scaling, translation, and other transforms.
 *
 * @return SVGRectF representing the text's bounds in world coordinates
 */
SVGRectF SVGText::worldBox() const { return SVGElement::worldBox(); }