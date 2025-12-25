#ifndef SVG_GRADIENT_H
#define SVG_GRADIENT_H

#include "SVGElement.h"
#include "Types.h"

#include <string>
#include <vector>

enum class SVGGradientUnits
{
    UserSpaceOnUse,
    ObjectBoundingBox
};

enum class SVGSpreadMethod
{
    Pad,
    Reflect,
    Repeat
};

struct SVGStop
{
    SVGNumber offset;
    SVGColor stopColor;
    SVGNumber stopOpacity;
};

class SVGGradient : public SVGElement
{
  public:
    SVGGradient()
      : gradientUnits(SVGGradientUnits::ObjectBoundingBox), spreadMethod(SVGSpreadMethod::Pad)
    {
    }
    ~SVGGradient() override = default;

    // Gradient attributes
    std::vector<SVGStop> stops;
    SVGGradientUnits gradientUnits;
    SVGSpreadMethod spreadMethod;
    std::string href; // Reference to another gradient (xlink:href or href)

    // Implement pure virtual methods from SVGElement
    // Gradients do not draw themselves directly when visited
    void accept(NodeVisitor& visitor) override {}

    // Gradients do not have a bounding box
    SVGRectF localBox() const override { return {0, 0, 0, 0}; }
};

class SVGLinearGradient : public SVGGradient
{
  public:
    SVGLinearGradient() : x1(0.0), y1(0.0), x2(1.0), y2(0.0) {} // Default 0% to 100%
    ~SVGLinearGradient() override = default;

    // Linear Gradient attributes (can be numbers or percentages, storing as numbers for now)
    // Note: Parsing logic will need to handle % vs numbers.
    // Usually standardizing to 0..1 bounding box or absolute values.
    SVGNumber x1, y1, x2, y2;
};

class SVGRadialGradient : public SVGGradient
{
  public:
    SVGRadialGradient() : cx(0.5), cy(0.5), r(0.5), fx(0.5), fy(0.5) {}
    ~SVGRadialGradient() override = default;

    SVGNumber cx, cy, r, fx, fy;
};

#endif // SVG_GRADIENT_H
