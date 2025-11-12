#include "SVGElement.h"
#include <algorithm> // Dùng cho std::min, std::max

// --- Các hàm Setters / Getters ---
// Đây là các hàm implement đơn giản
// (Nếu Role A đồng ý, chúng có thể được làm 'inline' trong file .h)

void SVGElement::setStyle(const SVGStyle& style) {
	m_style = style;
}

void SVGElement::setTransform(const SVGTransform& transform) {
	m_transform = transform;
}

SVGStyle& SVGElement::getStyle() {
	return m_style;
}

const SVGStyle& SVGElement::getStyle() const {
	return m_style;
}

SVGTransform& SVGElement::getTransform() {
	return m_transform;
}

const SVGTransform& SVGElement::getTransform() const {
	return m_transform;
}

// --- Hàm Bounding Box (World) ---
// Hàm này được kế thừa bởi TẤT CẢ các con

SVGRectF SVGElement::worldBox() const {
	// 1. Lấy BBox cục bộ (do lớp con định nghĩa)
	SVGRectF box = this->localBox();

	// 2. Lấy 4 góc của BBox cục bộ
	SVGPointF p1 = { box.x, box.y };
	SVGPointF p2 = { box.x + box.width, box.y };
	SVGPointF p3 = { box.x + box.width, box.y + box.height };
	SVGPointF p4 = { box.x, box.y + box.height };

	// 3. Áp dụng ma trận biến đổi "thế giới" (m_transform)
	//    lên 4 góc này.
	SVGPointF t_p1 = m_transform.map(p1);
	SVGPointF t_p2 = m_transform.map(p2);
	SVGPointF t_p3 = m_transform.map(p3);
	SVGPointF t_p4 = m_transform.map(p4);

	// 4. Tìm (min/max) của 4 góc ĐÃ BIẾN ĐỔI.
	//    Đây là BBox mới bao trọn hình đã bị xoay/phóng to.
	SVGNumber minX = std::min({ t_p1.x, t_p2.x, t_p3.x, t_p4.x });
	SVGNumber minY = std::min({ t_p1.y, t_p2.y, t_p3.y, t_p4.y });
	SVGNumber maxX = std::max({ t_p1.x, t_p2.x, t_p3.x, t_p4.x });
	SVGNumber maxY = std::max({ t_p1.y, t_p2.y, t_p3.y, t_p4.y });

	// 5. Trả về BBox "thế giới" mới
	return { minX, minY, maxX - minX, maxY - minY };
}


