#include "SVGRect.h"

// --- Hàm khởi tạo (Constructor) ---
SVGRect::SVGRect(const SVGRectF& rect, SVGNumber rx, SVGNumber ry)
	: m_rect(rect), m_rx(rx), m_ry(ry)
{
	// Thân hàm này có thể để trống.
	// Việc gán giá trị đã được thực hiện trong
	// "initializer list" (danh sách khởi tạo) ở trên.
}

// --- Hàm Bounding Box ---

SVGRectF SVGRect::localBox() const {
	// Bounding Box "cục bộ" (local) của một hình chữ nhật
	// chính là bản thân hình chữ nhật đó.
	return m_rect;
}

SVGRectF SVGRect::worldBox() const {
	// Gọi hàm worldBox() của lớp cha (SVGElement)
	return SVGElement::worldBox();
}