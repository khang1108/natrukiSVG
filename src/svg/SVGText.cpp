#include "SVGText.h"

SVGText::SVGText(const SVGPointF& pos, const std::string& text) : m_position(pos), m_text(text) {}

SVGRectF SVGText::localBox() const { return {m_position.x, m_position.y, 0, 0}; }

SVGRectF SVGText::worldBox() const { return SVGElement::worldBox(); }