#include "SVGStyle.h"

#include <limits>

namespace
{

    bool IsColorSet(const SVGColor& color)
    {
        if (color.isNone) {
            return true;
        }

        return color.a > 0;
    }

    const SVGNumber UNSET_NUMBER = -1.0;
} // namespace

SVGStyle::SVGStyle()
  : fillColor{0, 0, 0, 255}, strokeColor{0, 0, 0, 0, true}, strokeWidth(1.0), fillOpacity(1.0),
    strokeOpacity(1.0), fillRule(SVGFillRule::NonZero), fontSize(16.0)
{

    fillColor = {0, 0, 0, 0, false};

    strokeColor = {0, 0, 0, 0, false};

    strokeWidth = UNSET_NUMBER;

    fillOpacity = UNSET_NUMBER;

    strokeOpacity = UNSET_NUMBER;

    fillRule = SVGFillRule::NonZero;

    fontSize = UNSET_NUMBER;
}

/**
 * @brief Inherits style properties from a parent style (CSS-style inheritance).
 *
 * Algorithm (CSS Cascading Style Sheets inheritance):
 * 1. For each style property, check if the current style has a value set
 * 2. If not set (unset/inherited), copy the value from the parent style
 * 3. If already set, keep the current value (child overrides parent)
 *
 * This implements the CSS inheritance model where child elements inherit properties
 * from their parent elements unless explicitly overridden.
 *
 * Special cases:
 * - Colors: Use IsColorSet() to check if color is set (including "none")
 * - Numbers: Use UNSET_NUMBER (-1.0) to indicate unset values
 * - fillRule: Only inherit EvenOdd if parent has it and child doesn't explicitly have it
 * - isDisplayed: If parent is not displayed, child is also not displayed
 *
 * @param parent The parent style to inherit from
 */
void SVGStyle::inheritFrom(const SVGStyle& parent)
{

    // Inherit fill color if not set
    if (!IsColorSet(this->fillColor)) {
        this->fillColor = parent.fillColor;
    }

    // Inherit stroke color if not set
    if (!IsColorSet(this->strokeColor)) {
        this->strokeColor = parent.strokeColor;
    }

    // Inherit stroke width if not set
    if (this->strokeWidth == UNSET_NUMBER) {
        this->strokeWidth = parent.strokeWidth;
    }

    // Inherit fill opacity if not set
    if (this->fillOpacity == UNSET_NUMBER) {
        this->fillOpacity = parent.fillOpacity;
    }

    // Inherit stroke opacity if not set
    if (this->strokeOpacity == UNSET_NUMBER) {
        this->strokeOpacity = parent.strokeOpacity;
    }

    // Inherit fill rule (only EvenOdd from parent if child doesn't have it)
    if (this->fillRule != SVGFillRule::EvenOdd) {
        if (parent.fillRule == SVGFillRule::EvenOdd) {
            this->fillRule = SVGFillRule::EvenOdd;
        }
    }

    // Inherit font family if not set
    if (this->fontFamily.empty()) {
        this->fontFamily = parent.fontFamily;
    }

    // Inherit font size if not set
    if (this->fontSize == UNSET_NUMBER) {
        this->fontSize = parent.fontSize;
    }

    // If parent is not displayed, child is also not displayed
    if (!parent.isDisplayed) {
        this->isDisplayed = parent.isDisplayed;
    }
}

/**
 * @brief Applies default values to unset style properties (SVG specification defaults).
 *
 * Algorithm:
 * 1. Check each style property to see if it's unset (not inherited, not explicitly set)
 * 2. If unset, apply the SVG specification default value
 * 3. If set (either explicitly or inherited), keep the current value
 *
 * SVG Default Values (per SVG 1.1 specification):
 * - fill: black (rgb(0,0,0)) - shapes are filled with black by default
 * - stroke: none - no stroke by default
 * - stroke-width: 1.0 - if stroke is set, default width is 1
 * - fill-opacity: 1.0 (fully opaque)
 * - stroke-opacity: 1.0 (fully opaque)
 * - font-size: 16.0 pixels
 *
 * Important: This should be called AFTER inheritFrom() to ensure inherited values
 * are not overridden by defaults.
 */
void SVGStyle::applyDefaults()
{
    // Only apply defaults if the color hasn't been set (including inherited)
    // IsColorSet returns true if color.isNone OR color.a > 0
    // So if IsColorSet returns false, it means the color is truly unset
    if (!IsColorSet(this->fillColor)) {
        // If fillColor is not set and not explicitly set to "none", use black (SVG default)
        if (!this->fillColor.isNone) {
            this->fillColor = {0, 0, 0, 255, false}; // Black, fully opaque
        }
    }

    if (!IsColorSet(this->strokeColor)) {
        // If strokeColor is not set and not explicitly set to "none", use none (SVG default)
        if (!this->strokeColor.isNone) {
            this->strokeColor = {0, 0, 0, 0, true}; // None (transparent)
        }
    }

    // Default stroke width is 1.0 (if stroke is set)
    if (this->strokeWidth == UNSET_NUMBER) {
        this->strokeWidth = 1.0;
    }

    // Default fill opacity is 1.0 (fully opaque)
    if (this->fillOpacity == UNSET_NUMBER) {
        this->fillOpacity = 1.0;
    }

    // Default stroke opacity is 1.0 (fully opaque)
    if (this->strokeOpacity == UNSET_NUMBER) {
        this->strokeOpacity = 1.0;
    }

    // Default font size is 16.0 pixels
    if (this->fontSize == UNSET_NUMBER) {
        this->fontSize = 16.0;
    }
}