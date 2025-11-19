#include "SVGPolygon.h"

#include <algorithm>
#include <limits>

SVGPolygon::SVGPolygon(const std::vector<SVGPointF>& points) : m_points(points) {}

SVGRectF SVGPolygon::localBox() const
{

  if (m_points.empty()) {
    return {0, 0, 0, 0};
  }

  SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
  SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
  SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
  SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();

  for (const auto& p : m_points) {
    minX = std::min(minX, p.x);
    minY = std::min(minY, p.y);
    maxX = std::max(maxX, p.x);
    maxY = std::max(maxY, p.y);
  }

  return {minX, minY, maxX - minX, maxY - minY};
}

SVGRectF SVGPolygon::worldBox() const { return SVGElement::worldBox(); }