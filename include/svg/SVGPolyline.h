#ifndef SVG_POLYLINE_H
#define SVG_POLYLINE_H

#include "SVGElement.h" // Kế thừa từ lớp cha
#include "Types.h"      // Để dùng std::vector<SVGPointF> và SVGRectF

/**
 * @brief Đại diện cho một phần tử <polyline> (Đa tuyến).
 *
 * Một <polyline> là một chuỗi các điểm được nối với nhau,
 * nhưng điểm cuối cùng KHÔNG tự động nối về điểm đầu.
 */
class SVGPolyline : public SVGElement
{
private:
  /**
   * @brief Danh sách các điểm (vertices) của đa tuyến.
   * Lưu trữ giá trị của thuộc tính 'points'.
   *
   */
  std::vector<SVGPointF> m_points;

public:
  /**
   * @brief Hàm khởi tạo (Constructor).
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGPolyline.cpp'.
   * Hàm này được 'SVGFactory' gọi.
   * Role B cũng chịu trách nhiệm parse chuỗi 'points'
   * (ví dụ: "10,10 20,20 10,30")
   * để tạo ra 'std::vector<SVGPointF>' và truyền vào đây.
   */
  SVGPolyline(const std::vector<SVGPointF>& points);

  /**
   * @brief Hàm "chấp nhận" của Visitor Pattern.
   *
   * * - Vai trò của Role A (Implement - Inline):
   * Đã implement sẵn (inline) trong file .h này.
   * Nó gọi đúng hàm 'visit' của visitor.
   */
  void accept(NodeVisitor& visitor) override { visitor.visit(*this); }

  // --- Các hàm Getters (Cho Role C sử dụng) ---
  /**
   * @brief Lấy danh sách các điểm.
   * Role C (UI) sẽ gọi hàm này bên trong
   * 'QtRenderer::visit(SVGPolyline& polyline)' để
   * tạo một 'QPolygonF' và gọi 'QPainter::drawPolyline()'.
   * @return Một tham chiếu hằng (const ref) đến vector điểm.
   */
  const std::vector<SVGPointF>& getPoints() const { return m_points; }

  // --- Các hàm Bounding Box ---

  /**
   * @brief Tính BBox cục bộ của đa tuyến.
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGPolyline.cpp'.
   * Tương tự như 'SVGPolygon', phải lặp (loop) qua 'm_points'
   * để tìm ra 4 giá trị: x_min, y_min, x_max, y_max.
   * (Lưu ý: BBox của 'polyline' có thể phải tính cả
   * 'stroke-width' vào, nhưng ở mức MVP
   * chỉ cần BBox của các điểm là đủ).
   */
  SVGRectF localBox() const;

  /**
   * @brief Tính BBox "thế giới" (World) của đa tuyến.
   *
   * * - Vai trò của Role B (Implement):
   * Viết logic trong 'SVGPolyline.cpp'.
   * Phải lấy 'localBox()' và áp dụng 'm_transform'
   * (đã kế thừa từ cha) lên nó.
   */
  SVGRectF worldBox() const;
};
#endif // SVG_POLYLINE_H