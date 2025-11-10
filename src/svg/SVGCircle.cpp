#include "SVGCircle.h"

// --- Hàm khởi tạo (Constructor) ---
SVGCircle::SVGCircle(const SVGPointF& center, SVGNumber radius)
	: m_center(center), m_radius(radius)
{
	// Thân hàm này có thể để trống.
}

// --- Hàm Bounding Box ---

SVGRectF SVGCircle::localBox() const {
	// Bounding Box "cục bộ" (local) của một hình tròn
	// được tính từ tâm (cx, cy) và bán kính (r).
	//
	// x = cx - r
	// y = cy - r
	// width = 2 * r
	// height = 2 * r

	SVGNumber diameter = m_radius * 2.0;

	return {
		m_center.x - m_radius,
		m_center.y - m_radius,
		diameter,
		diameter
	};
}

SVGRectF SVGCircle::worldBox() const {
	// Gọi hàm worldBox() của lớp cha (SVGElement)
	return SVGElement::worldBox();
}
