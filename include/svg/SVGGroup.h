#ifndef SVG_GROUP_H
#define SVG_GROUP_H

#include "SVGElement.h" // Kế thừa từ lớp cha
#include "Types.h"      // Để dùng SVGRectF (cho Bounding Box)
#include <vector>
#include <memory>

/** 
 * @brief Đại diện cho thẻ <g> (group).
 * Dùng để nhóm các phần tử con lại với nhau, áp dụng chung
 * 'style' và 'transform' cho cả nhóm.
 */
class SVGGroup : public SVGElement {
private:
    /**
     * @brief Danh sách con trỏ thông minh (unique_ptr) đến các
     * phần tử con (<rect>, <circle>, <g> khác, v.v...)
     */
    std::vector<std::unique_ptr<SVGElement>> m_children;

public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGGroup.cpp' (nếu cần).
     */
    SVGGroup();

    /**
     * @brief Thêm một phần tử con vào nhóm.
     * Role B (Parser) sẽ gọi hàm này khi duyệt cây DOM.
     */
    void addChild(std::unique_ptr<SVGElement> child);

    /**
     * @brief Lấy danh sách (chỉ đọc) các con.
     * Role C (Visitor) sẽ dùng hàm này để duyệt và vẽ các con.
     */
    const std::vector<std::unique_ptr<SVGElement>>& getChildren() const {
        return m_children;
    }

   /**
     * @brief Hàm "chấp nhận" của Visitor Pattern cho Group.
     *
     * * - Vai trò của Role B (Implement):
     * Phải implement logic trong 'SVGGroup.cpp'.
     * Logic này giờ ĐƠN GIẢN hơn rất nhiều:
     * 1. Gọi 'visitor.visitGroupBegin(*this);'
     * 2. Lặp (loop) qua 'm_children' và gọi 'child->accept(visitor)'
     * cho TỪNG đứa con.
     * 3. Gọi 'visitor.visitGroupEnd(*this);'
     *
     * (Toàn bộ logic 'save/restore' transform của Qt6
     * giờ đã được chuyển cho Role C
     * implement bên trong 'visitGroupBegin/End').
     */
    void accept(NodeVisitor& visitor) override;

    // --- Các hàm Bounding Box (Yêu cầu của dự án) ---

    /**
     * @brief Tính toán Bounding Box "cục bộ" (Local) của Group.
     * Box này *chỉ* bao gồm các con của nó, *chưa* tính transform của chính Group này.
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGGroup.cpp'. Phải lặp qua tất cả
     * 'm_children', gọi hàm 'worldBox()' của chúng và
     * tìm ra box lớn nhất bao trọn tất cả.
     */
    SVGRectF localBox() const;

    /**
     * @brief Tính toán Bounding Box "thế giới" (World).
     * Box này là 'localBox()' *sau khi đã* áp dụng 'm_transform' của chính Group này.
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic trong 'SVGGroup.cpp'.
     * 1. Gọi 'localBox()'.
     * 2. Dùng 'm_transform.map(...)' để biến đổi 4 góc
     * của 'localBox' đó.
     * 3. Trả về box mới bao trọn 4 góc đã biến đổi.
     */
    SVGRectF worldBox() const;
};

#endif // SVG_GROUP_H