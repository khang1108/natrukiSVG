#include "SVGLine.h"

#include <algorithm>

SVGLine::SVGLine(const SVGPointF& p1, const SVGPointF& p2) : m_p1(p1), m_p2(p2) {}

SVGRectF SVGLine::localBox() const
{

  SVGNumber minX = std::min(m_p1.x, m_p2.x);
  SVGNumber minY = std::min(m_p1.y, m_p2.y);
  SVGNumber maxX = std::max(m_p1.x, m_p2.x);
  SVGNumber maxY = std::max(m_p1.y, m_p2.y);

  return {minX, minY, maxX - minX, maxY - minY};
}

SVGRectF SVGLine::worldBox() const { return SVGElement::worldBox(); }