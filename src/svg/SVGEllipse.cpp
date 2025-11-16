#include "SVGEllipse.h"

// --- Hàm khởi tạo (Constructor) ---
SVGEllipse::SVGEllipse(const SVGPointF& center, SVGNumber rx, SVGNumber ry)
	: m_center(center), m_rx(rx), m_ry(ry)
{
	// Thân hàm để trống
}

// --- Hàm Bounding Box ---

SVGRectF SVGEllipse::localBox() const {
	// Bounding Box "cục bộ" (local) của một hình elip
	// x = cx - rx
	// y = cy - ry
	// width = 2 * rx
	// height = 2 * ry

	return {
		m_center.x - m_rx,
		m_center.y - m_ry,
		m_rx * 2.0,
		m_ry * 2.0
	};
}

SVGRectF SVGEllipse::worldBox() const {
	// Gọi hàm worldBox() của lớp cha (SVGElement)
	return SVGElement::worldBox();
}
