#include "SVGCircle.h"

SVGCircle::SVGCircle(const SVGPointF& center, SVGNumber radius) : m_center(center), m_radius(radius)
{
}
SVGRectF SVGCircle::localBox() const
{
  SVGNumber diameter = m_radius * 2.0;

  return {m_center.x - m_radius, m_center.y - m_radius, diameter, diameter};
}
SVGRectF SVGCircle::worldBox() const { return SVGElement::worldBox(); }
