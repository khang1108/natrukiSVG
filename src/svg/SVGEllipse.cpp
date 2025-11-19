#include "SVGEllipse.h"

SVGEllipse::SVGEllipse(const SVGPointF& center, SVGNumber rx, SVGNumber ry)
  : m_center(center), m_rx(rx), m_ry(ry)
{
}

SVGRectF SVGEllipse::localBox() const
{

  return {m_center.x - m_rx, m_center.y - m_ry, m_rx * 2.0, m_ry * 2.0};
}

SVGRectF SVGEllipse::worldBox() const { return SVGElement::worldBox(); }
