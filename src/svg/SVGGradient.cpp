#include "SVGGradient.h"

Gradient::Gradient() 
    : gradientUnits(GradientUnit::ObjectBoundingBox), spreadMethod(SpreadMethod::Pad)
{
}

void Gradient::accept(NodeVisitor& visitor)
{
    // Currently NodeVisitor does not support Gradient.
    // Ideally: visitor.visit(*this);
}

SVGRectF Gradient::localBox() const
{
    return {0, 0, 0, 0}; // Gradients do not have a bounding box
}

void Gradient::addStop(const SVGStop& stop)
{
    stops.push_back(stop);
}

GradientUnit Gradient::getGradientUnits() const
{
    return gradientUnits;
}

void Gradient::setGradientUnits(GradientUnit gradientUnits)
{
    this->gradientUnits = gradientUnits;
}

SpreadMethod Gradient::getSpreadMethod() const
{
    return spreadMethod;
}

void Gradient::setSpreadMethod(SpreadMethod spreadMethod)
{
    this->spreadMethod = spreadMethod;
}

SVGTransform Gradient::getTransform() const
{
    return transform;
}

void Gradient::setTransform(SVGTransform transform)
{
    this->transform = transform;
}

std::vector<SVGStop> Gradient::getStops() const
{
    return stops;
}

void Gradient::setStops(std::vector<SVGStop> stops)
{
    this->stops = stops;
}

// --- LinearGradient ---
LinearGradient::LinearGradient() {}

// --- RadialGradient ---
RadialGradient::RadialGradient() {}
