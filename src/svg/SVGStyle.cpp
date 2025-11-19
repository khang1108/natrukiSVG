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

void SVGStyle::inheritFrom(const SVGStyle& parent)
{

    if (!IsColorSet(this->fillColor)) {
        this->fillColor = parent.fillColor;
    }

    if (!IsColorSet(this->strokeColor)) {
        this->strokeColor = parent.strokeColor;
    }

    if (this->strokeWidth == UNSET_NUMBER) {
        this->strokeWidth = parent.strokeWidth;
    }

    if (this->fillOpacity == UNSET_NUMBER) {
        this->fillOpacity = parent.fillOpacity;
    }

    if (this->strokeOpacity == UNSET_NUMBER) {
        this->strokeOpacity = parent.strokeOpacity;
    }

    if (this->fillRule != SVGFillRule::EvenOdd) {
        if (parent.fillRule == SVGFillRule::EvenOdd) {
            this->fillRule = SVGFillRule::EvenOdd;
        }
    }

    if (this->fontFamily.empty()) {
        this->fontFamily = parent.fontFamily;
    }

    if (this->fontSize == UNSET_NUMBER) {
        this->fontSize = parent.fontSize;
    }

    if (!parent.isDisplayed) {
        this->isDisplayed = parent.isDisplayed;
    }
}

void SVGStyle::applyDefaults()
{
    if (!IsColorSet(this->fillColor) && !this->fillColor.isNone) {
        this->fillColor = {0, 0, 0, 255, false};
    }

    if (!IsColorSet(this->strokeColor) && !this->strokeColor.isNone) {
        this->strokeColor = {0, 0, 0, 0, true};
    }

    if (this->strokeWidth == UNSET_NUMBER) {
        this->strokeWidth = 1.0;
    }

    if (this->fillOpacity == UNSET_NUMBER) {
        this->fillOpacity = 1.0;
    }

    if (this->strokeOpacity == UNSET_NUMBER) {
        this->strokeOpacity = 1.0;
    }

    if (this->fontSize == UNSET_NUMBER) {
        this->fontSize = 16.0;
    }
}