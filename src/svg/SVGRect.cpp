#include "SVGRect.h"

SVGRect::SVGRect(const SVGRectF& rect, SVGNumber rx, SVGNumber ry)
  : m_rect(rect), m_rx(rx), m_ry(ry)
{
}

SVGRectF SVGRect::localBox() const { return m_rect; }

SVGRectF SVGRect::worldBox() const { return SVGElement::worldBox(); }