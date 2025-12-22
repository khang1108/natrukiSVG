#ifndef SVG_GRADIENT
#define SVG_GRADIENT

#include "SVGElement.h"
#include "Types.h"
#include "SVGTransform.h"
#include <vector>

/*
* @brief: GradientUnit enum class
* @note: GradientUnit chỉ định đơn vị cho gradient
* @see: https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/gradientUnits
*/
enum class GradientUnit
{
    UserSpaceOnUse,
    ObjectBoundingBox
};

/*
* @brief: SpreadMethod enum class
* @note: SpreadMethod chỉ định cách xử lý khi giá trị offset vượt quá [0, 1]
* @see: https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/spreadMethod
*/
enum class SpreadMethod
{
    Pad,
    Reflect,
    Repeat
};

struct SVGStop
{
    float offset;
    SVGColor stopColor;
    SVGNumber stopOpacity;
};

class Gradient : public SVGElement
{
private:
    GradientUnit gradientUnits;
    SpreadMethod spreadMethod;
    SVGTransform transform;
    std::vector<SVGStop> stops;

public:
    Gradient();
    ~Gradient() = default;

    // --- Visitor Pattern ---
    void accept(NodeVisitor& visitor) override;
    SVGRectF localBox() const override;

    // --- Stop ---
    void addStop(const SVGStop& stop);

    // --- GradientUnit ---
    GradientUnit getGradientUnits() const;
    void setGradientUnits(GradientUnit gradientUnits);

    // --- SpreadMethod ---
    SpreadMethod getSpreadMethod() const;
    void setSpreadMethod(SpreadMethod spreadMethod);

    // --- Transform ---
    SVGTransform getTransform() const;
    void setTransform(SVGTransform transform);

    // --- Stops ---
    std::vector<SVGStop> getStops() const;
    void setStops(std::vector<SVGStop> stops);
};

class LinearGradient : public Gradient
{
public:
    LinearGradient();
    ~LinearGradient() = default;
};

class RadialGradient : public Gradient
{
public:
    RadialGradient();
    ~RadialGradient() = default;
};
#endif