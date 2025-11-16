#include "SVGPolygon.h"
#include <algorithm> // Dùng cho std::min và std::max
#include <limits> // Dùng cho std::numeric_limits

// --- Hàm khởi tạo (Constructor) ---
SVGPolygon::SVGPolygon(const std::vector<SVGPointF>& points)
	: m_points(points)
{
	// Thân hàm để trống
}

// --- Hàm Bounding Box ---

SVGRectF SVGPolygon::localBox() const {
	// Bounding Box "cục bộ" (local) của một đa giác
	// là hình chữ nhật nhỏ nhất chứa TẤT CẢ các điểm của nó.

	if (m_points.empty()) {
		return { 0, 0, 0, 0 }; // Trả về box rỗng nếu không có điểm
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

	return {
		minX,
		minY,
		maxX - minX,
		maxY - minY
	};
}

SVGRectF SVGPolygon::worldBox() const {
	// Gọi hàm worldBox() của lớp cha (SVGElement)
	return SVGElement::worldBox();
}