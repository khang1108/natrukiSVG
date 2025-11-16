#include "SVGLine.h"
#include <algorithm> // Dùng cho std::min và std::max

// --- Hàm khởi tạo (Constructor) ---
SVGLine::SVGLine(const SVGPointF& p1, const SVGPointF& p2)
	: m_p1(p1), m_p2(p2)
{
	// Thân hàm để trống
}

// --- Hàm Bounding Box ---

SVGRectF SVGLine::localBox() const {
	// Bounding Box "cục bộ" (local) của một đường thẳng
	// là hình chữ nhật nhỏ nhất chứa 2 điểm cuối của nó.

	SVGNumber minX = std::min(m_p1.x, m_p2.x);
	SVGNumber minY = std::min(m_p1.y, m_p2.y);
	SVGNumber maxX = std::max(m_p1.x, m_p2.x);
	SVGNumber maxY = std::max(m_p1.y, m_p2.y);

	return {
		minX,
		minY,
		maxX - minX,
		maxY - minY
	};
}

SVGRectF SVGLine::worldBox() const {
	// Gọi hàm worldBox() của lớp cha (SVGElement)
	return SVGElement::worldBox();
}