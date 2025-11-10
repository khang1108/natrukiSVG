#include "SVGText.h"

// --- Hàm khởi tạo (Constructor) ---
SVGText::SVGText(const SVGPointF& pos, const std::string& text)
	: m_position(pos), m_text(text)
{
	// Thân hàm để trống
}

// --- Hàm Bounding Box ---

SVGRectF SVGText::localBox() const {
	// Role B không thể
	// tính toán BBox chính xác của text nếu không có
	// hệ thống 'IFontMetrics' (do Role C cung cấp).

	// Giải pháp 1 (An toàn): Trả về một BBox rỗng tại điểm (x, y).
	return { m_position.x, m_position.y, 0, 0 };

	// Giải pháp 2 (Ước lượng - không chính xác):
	// Giả sử 1 ký tự rộng ~ 0.5 * fontSize, cao 1.0 * fontSize
	// SVGNumber fontSize = m_style.fontSize; // Cần implement SVGStyle trước
	// SVGNumber approxWidth = m_text.length() * fontSize * 0.5;
	// SVGNumber approxHeight = fontSize;
	// return {m_position.x, m_position.y - approxHeight, approxWidth, approxHeight};
}

SVGRectF SVGText::worldBox() const {
	// Gọi hàm worldBox() của lớp cha (SVGElement)
	return SVGElement::worldBox();
}