#ifndef SVG_ELLIPSE_H
#define SVG_ELLIPSE_H

#include "SVGElement.h" // Kế thừa từ lớp cha
#include "Types.h"      // Để dùng SVGPointF, SVGNumber, SVGRectF

/**
 * @brief Đại diện cho một phần tử <ellipse> (Hình elip).
 *
 */
class SVGEllipse : public SVGElement {
private:
    SVGPointF m_center; // Lưu cx, cy
    SVGNumber m_rx;     // Bán kính x
    SVGNumber m_ry;     // Bán kính y

public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGEllipse.cpp'.
     * Được gọi bởi 'SVGFactory'.
     */
    SVGEllipse(const SVGPointF& center, SVGNumber rx, SVGNumber ry);
    
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
    // để 'QPainter::drawEllipse()'.
    SVGPointF getCenter() const { return m_center; }
    SVGNumber getRx() const { return m_rx; }
    SVGNumber getRy() const { return m_ry; }

    // --- Các hàm Bounding Box ---

    /**
     * @brief Tính BBox cục bộ của hình elip.
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGEllipse.cpp'.
     * (Kết quả là: x = cx - rx, y = cy - ry, width = 2*rx, height = 2*ry).
     */
    SVGRectF localBox() const;

    /**
     * @brief Tính BBox "thế giới" (World) của hình elip.
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGEllipse.cpp'.
     * Phải lấy 'localBox()' và áp dụng 'm_transform'.
     */
    SVGRectF worldBox() const;
};
#endif // SVG_ELLIPSE_H