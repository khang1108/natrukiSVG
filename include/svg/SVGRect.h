#ifndef SVG_RECT_H
#define SVG_RECT_H

#include "SVGElement.h" // Kế thừa từ lớp cha
#include "Types.h"      // Để dùng SVGRectF

/**
 * @brief Đại diện cho một phần tử <rect> (Hình chữ nhật).
 *
 */
class SVGRect : public SVGElement
{
private:
  SVGRectF m_rect; // Lưu x, y, width, height
  SVGNumber m_rx;  // Bán kính bo góc x
  SVGNumber m_ry;  // Bán kính bo góc y

public:
  /**
   * @brief Hàm khởi tạo (Constructor).
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGRect.cpp'. Hàm này được
   * 'SVGFactory' gọi sau khi parse thuộc tính.
   */
  SVGRect(const SVGRectF& rect, SVGNumber rx, SVGNumber ry);

  /**
   * @brief Hàm "chấp nhận" của Visitor Pattern.
   *
   * * - Vai trò của Role A (Implement - Inline):
   * Đã implement sẵn (inline) trong file .h này.
   * Nó gọi đúng hàm 'visit' của visitor.
   */
  void accept(NodeVisitor& visitor) override { visitor.visit(*this); }

  // --- Các hàm Getters (Cho Role C sử dụng) ---
  // Role C (UI) sẽ gọi các hàm này bên trong
  // 'QtRenderer::visit(SVGRect& rect)' để lấy dữ liệu
  // và gọi hàm 'QPainter::drawRoundedRect()'.

  const SVGRectF& getRect() const { return m_rect; }
  SVGNumber getRx() const { return m_rx; }
  SVGNumber getRy() const { return m_ry; }

  // --- Các hàm Bounding Box ---

  /**
   * @brief Tính BBox cục bộ. Với <rect>, nó chính là 'm_rect'.
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGRect.cpp' (chỉ cần trả về 'm_rect').
   */
  SVGRectF localBox() const;

  /**
   * @brief Tính BBox "thế giới" (World).
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGRect.cpp'.
   * Phải lấy 'localBox()' và áp dụng 'm_transform'
   * (đã kế thừa từ cha) lên nó.
   */
  SVGRectF worldBox() const;
};
#endif // SVG_RECT_H