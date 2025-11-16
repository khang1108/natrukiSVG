#ifndef SVG_POLYGON_H
#define SVG_POLYGON_H

#include "SVGElement.h" // Kế thừa từ lớp cha
#include "Types.h"      // Để dùng std::vector<SVGPointF> và SVGRectF

/**
 * @brief Đại diện cho một phần tử <polygon> (Đa giác).
 *
 * Một <polygon> là một chuỗi các điểm được nối với nhau,
 * và điểm cuối cùng tự động nối về điểm đầu tiên (tạo thành hình khép kín).
 */
class SVGPolygon : public SVGElement {
private:
    /**
     * @brief Danh sách các điểm (vertices) của đa giác.
     * Lưu trữ giá trị của thuộc tính 'points'.
     *
     */
    std::vector<SVGPointF> m_points;

public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGPolygon.cpp'.
     * Hàm này được 'SVGFactory' gọi.
     * Role B cũng phải chịu trách nhiệm parse chuỗi 'points'
     * (ví dụ: "10,10 20,20 10,30")
     * để tạo ra 'std::vector<SVGPointF>' và truyền vào đây.
     */
    SVGPolygon(const std::vector<SVGPointF>& points);
    
    /**
     * @brief Hàm "chấp nhận" của Visitor Pattern.
     *
     * * - Vai trò của Role A (Implement - Inline):
     * Đã implement sẵn (inline) trong file .h này.
     * Nó gọi đúng hàm 'visit' của visitor.
     */
    void accept(NodeVisitor& visitor) override {
        visitor.visit(*this);
    }
    
    // --- Các hàm Getters (Cho Role C sử dụng) ---
    /**
     * @brief Lấy danh sách các điểm.
     * Role C (UI) sẽ gọi hàm này bên trong
     * 'QtRenderer::visit(SVGPolygon& polygon)' để
     * tạo một 'QPolygonF' và gọi 'QPainter::drawPolygon()'.
     * @return Một tham chiếu hằng (const ref) đến vector điểm.
     */
    const std::vector<SVGPointF>& getPoints() const { return m_points; }

    // --- Các hàm Bounding Box ---

    /**
     * @brief Tính BBox cục bộ của đa giác.
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGPolygon.cpp'.
     * Phải lặp (loop) qua toàn bộ 'm_points' để tìm ra
     * 4 giá trị: x_min, y_min, x_max, y_max.
     * BBox sẽ là {x_min, y_min, x_max - x_min, y_max - y_min}.
     */
    SVGRectF localBox() const;

    /**
     * @brief Tính BBox "thế giới" (World) của đa giác.
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGPolygon.cpp'.
     * Phải lấy 'localBox()' và áp dụng 'm_transform'
     * (đã kế thừa từ cha) lên nó.
     * (Lưu ý: BBox của một đa giác đã xoay (rotated) có thể
     * lớn hơn BBox cục bộ được xoay).
     */
    SVGRectF worldBox() const;
};
#endif // SVG_POLYGON_H