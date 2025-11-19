#ifndef SVG_CIRCLE_H
#define SVG_CIRCLE_H

#include "SVGElement.h"
#include "Types.h"

/**
 * @brief Đại diện cho một phần tử <circle> (Hình tròn).
 *
 */
class SVGCircle : public SVGElement
{
private:
  SVGPointF m_center; // Lưu cx, cy
  SVGNumber m_radius; // Lưu r

public:
  /**
   * @brief Hàm khởi tạo (Constructor).
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGCircle.cpp'.
   * Được gọi bởi 'SVGFactory'.
   */
  SVGCircle(const SVGPointF& center, SVGNumber radius);

  /**
   * @brief Hàm "chấp nhận" của Visitor Pattern.
   *
   * * - Vai trò của Role A (Implement - Inline):
   * Đã implement sẵn (inline) trong file .h này.
   */
  void accept(NodeVisitor& visitor) override { visitor.visit(*this); }

  // --- Các hàm Getters (Cho Role C sử dụng) ---
  // Role C (UI) sẽ gọi các hàm này bên trong
  // 'QtRenderer::visit(SVGCircle& rect)' để
  // gọi hàm 'QPainter::drawEllipse()'.

  SVGPointF getCenter() const { return m_center; }
  SVGNumber getRadius() const { return m_radius; }

  // --- Các hàm Bounding Box ---

  /**
   * @brief Tính BBox cục bộ của hình tròn.
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGCircle.cpp'.
   * (Kết quả là: x = cx - r, y = cy - r, width = 2*r, height = 2*r).
   */
  SVGRectF localBox() const;

  /**
   * @brief Tính BBox "thế giới" (World) của hình tròn.
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGCircle.cpp'.
   * Phải lấy 'localBox()' và áp dụng 'm_transform'
   * (đã kế thừa từ cha) lên nó.
   */
  SVGRectF worldBox() const;
};
#endif // SVG_CIRCLE_H