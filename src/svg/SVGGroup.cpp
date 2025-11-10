#include "SVGGroup.h"
#include <limits>    // Dùng cho std::numeric_limits
#include <algorithm> // Dùng cho std::min/max

// --- Hàm khởi tạo (Constructor) ---
SVGGroup::SVGGroup() {
	// Thân hàm để trống
}

// --- Hàm Quản lý Con ---
void SVGGroup::addChild(std::unique_ptr<SVGElement> child) {
	// Di chuyển (move) con trỏ thông minh vào vector
	m_children.push_back(std::move(child));
}

// --- Hàm Visitor Pattern ---
void SVGGroup::accept(NodeVisitor& visitor) {
	// Logic "duyệt" (traversal) của Visitor Pattern:

	// 1. Thông báo cho Visitor biết chúng ta BẮT ĐẦU vào một Group
	visitor.visitGroupBegin(*this);

	// 2. Đệ quy: Yêu cầu TẤT CẢ các con tự "chấp nhận" visitor
	for (const auto& child : m_children) {
		if (child) {
			child->accept(visitor);
		}
	}

	// 3. Thông báo cho Visitor biết chúng ta KẾT THÚC Group này
	visitor.visitGroupEnd(*this);
}

// --- Hàm Bounding Box ---

SVGRectF SVGGroup::localBox() const {
	// Bounding Box "cục bộ" của một Group là
	// hình chữ nhật LỚN NHẤT bao bọc BBox "thế giới" (worldBox)
	// của TẤT CẢ các con của nó.

	if (m_children.empty()) {
		return { 0, 0, 0, 0 }; // Box rỗng nếu không có con
	}

	SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
	SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
	SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
	SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();

	for (const auto& child : m_children) {
		if (!child) continue;

		// **QUAN TRỌNG**: Gọi 'worldBox()', không phải 'localBox()'.
		// Vì BBox của con phải được tính toán
		// *sau khi* đã áp dụng transform CỦA CON.
		SVGRectF childWorldBox = child->worldBox();

		// Kiểm tra xem BBox của con có hợp lệ không
		if (childWorldBox.width >= 0 && childWorldBox.height >= 0) {
			// Hợp nhất BBox của con vào BBox chung của Group
			minX = std::min(minX, childWorldBox.x);
			minY = std::min(minY, childWorldBox.y);
			maxX = std::max(maxX, childWorldBox.x + childWorldBox.width);
			maxY = std::max(maxY, childWorldBox.y + childWorldBox.height);
		}
	}

	// Nếu không có con nào có BBox hợp lệ (ví dụ: toàn text)
	if (minX == std::numeric_limits<SVGNumber>::infinity()) {
		return { 0, 0, 0, 0 };
	}

	// Trả về BBox cục bộ của Group
	return {
		minX,
		minY,
		maxX - minX,
		maxY - minY
	};
}

SVGRectF SVGGroup::worldBox() const {
	// Gọi hàm worldBox() của lớp cha (SVGElement)
	return SVGElement::worldBox();
}