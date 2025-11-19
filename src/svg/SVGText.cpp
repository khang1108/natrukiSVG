#include "SVGText.h"

SVGText::SVGText(const SVGPointF& pos, const std::string& text) : m_position(pos), m_text(text) {}

SVGRectF SVGText::localBox() const
{
    const SVGStyle& style = getStyle();
    SVGNumber fontSize = style.fontSize > 0.0 ? style.fontSize : 16.0;
    SVGNumber approxWidth = fontSize * 0.6 * static_cast<SVGNumber>(m_text.size());
    SVGNumber approxHeight = fontSize;
    return {m_position.x, m_position.y - approxHeight, approxWidth, approxHeight};
}

SVGRectF SVGText::worldBox() const { return SVGElement::worldBox(); }