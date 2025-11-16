// File: /include/svg/SVGLine.h

#ifndef SVG_LINE_H
#define SVG_LINE_H

#include "SVGElement.h" // Kế thừa từ lớp cha
#include "Types.h"      // Để dùng SVGPointF, SVGRectF

/**
 * @brief Đại diện cho một phần tử <line> (Đường thẳng).
 *
 */
class SVGLine : public SVGElement {
private:
    SVGPointF m_p1; // Lưu x1, y1
    SVGPointF m_p2; // Lưu x2, y2

public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGLine.cpp'.
     * Được gọi bởi 'SVGFactory'.
     */
    SVGLine(const SVGPointF& p1, const SVGPointF& p2);
    
    /**
     * @brief Hàm "chấp nhận" của Visitor Pattern.
     *
     * * - Vai trò của Role A (Implement - Inline):
     * Đã implement sẵn (inline) trong file .h này.
     */
    void accept(NodeVisitor& visitor) override {
        visitor.visit(*this);
    }
    
    // --- Các hàm Getters (Cho Role C sử dụng) ---
    // Role C (UI) sẽ gọi các hàm này
    // để 'QPainter::drawLine()'.
    SVGPointF getP1() const { return m_p1; }
    SVGPointF getP2() const { return m_p2; }

    // --- Các hàm Bounding Box ---

    /**
     * @brief Tính BBox cục bộ của đường thẳng.
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGLine.cpp'.
     * (Tương tự Polygon, tìm x_min, y_min, x_max, y_max
     * của 2 điểm 'm_p1' và 'm_p2').
     */
    SVGRectF localBox() const;

    /**
     * @brief Tính BBox "thế giới" (World) của đường thẳng.
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGLine.cpp'.
     * Phải lấy 'localBox()' và áp dụng 'm_transform'.
     */
    SVGRectF worldBox() const;
};
#endif // SVG_LINE_H