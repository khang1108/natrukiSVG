// File: /include/svg/SVGText.h

#ifndef SVG_TEXT_H
#define SVG_TEXT_H

#include "SVGElement.h"
#include "Types.h"

#include <string>

/**
 * @brief Đại diện cho một phần tử <text> (Văn bản).
 */
class SVGText : public SVGElement
{
private:
  SVGPointF m_position; // Lưu x, y
  std::string m_text;   // Lưu nội dung văn bản

public:
  /**
   * @brief Hàm khởi tạo (Constructor).
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGText.cpp'.
   * Được gọi bởi 'SVGFactory'.
   */
  SVGText(const SVGPointF& pos, const std::string& text);

  /**
   * @brief Hàm "chấp nhận" của Visitor Pattern.
   *
   * * - Vai trò của Role A (Implement - Inline):
   * Đã implement sẵn (inline) trong file .h này.
   */
  void accept(NodeVisitor& visitor) override { visitor.visit(*this); }

  // --- Các hàm Getters (Cho Role C sử dụng) ---
  // Role C (UI) sẽ gọi các hàm này bên trong
  // 'QtRenderer::visit(SVGText& rect)' để
  // lấy 'fontFamily', 'fontSize' từ 'getStyle()'
  // và gọi hàm 'QPainter::drawText()'.

  SVGPointF getPosition() const { return m_position; }
  const std::string& getText() const { return m_text; }

  // --- Các hàm Bounding Box ---

  /**
   * @brief Tính BBox cục bộ của Text.
   *
   * * - Vai trò của Role B (Implement):
   * Đây là hàm PHỨC TẠP. Role B không thể tính toán
   * BBox của text một cách chính xác nếu không biết về Font.
   * Role B có thể phải:
   * 1. Bỏ qua (return box 0x0).
   * 2. Hoặc tính toán gần đúng (cực kỳ khó).
   * 3. Hoặc yêu cầu Role A/C cung cấp một interface
   * để "hỏi" hệ thống font về kích thước (ví dụ: 'IFontMetrics').
   * Sau đó, Role B có thể dùng interface này để
   * tính toán kích thước bounding box dựa trên
   * 'nội dung văn bản' và 'kích thước font' trong 'm_style'.
   */
  SVGRectF localBox() const;

  /**
   * @brief Tính BBox "thế giới" (World) của Text.
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGText.cpp'.
   * Lấy 'localBox()' và áp dụng 'm_transform'.
   */
  SVGRectF worldBox() const;
};
#endif // SVG_TEXT_H