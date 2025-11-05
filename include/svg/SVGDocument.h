// File: /include/svg/SVGDocument.h

#ifndef SVG_DOCUMENT_H
#define SVG_DOCUMENT_H

#include "Types.h"  // Để lấy kiểu dữ liệu SVGRectF (cho viewBox)
#include <vector>     
#include <memory>     // Để dùng std::unique_ptr (quản lý bộ nhớ tự động)

class SVGElement; 
class NodeVisitor; 

/**
 * @class SVGDocument
 * @brief Đại diện cho toàn bộ tài liệu SVG (tương ứng với thẻ <svg>).
 * Đây là nút gốc (root) của cây cấu trúc SVG (DOM tree).
 * Nó chứa các thuộc tính toàn cục (như viewBox) và danh sách
 * các phần tử con cấp cao nhất của nó.
 */
class SVGDocument {

private:
    /**
     * @brief Lưu trữ 4 giá trị của thuộc tính 'viewBox' (min-x, min-y, width, height).
     * 'viewBox' định nghĩa một hệ tọa độ ảo cho file SVG.
     * * - Vai trò của Role B (Parser): Sẽ đọc chuỗi "0 0 100 100"
     * từ file XML và dùng hàm setViewBox() để lưu vào biến này.
     * * - Vai trò của Role C (UI): Sẽ đọc giá trị này (qua hàm getViewBox())
     * để tính toán ma trận biến đổi (scale/translate) tổng thể,
     * đảm bảo hình SVG được phóng to/thu nhỏ vừa vặn với cửa sổ Qt6.
     *
     * Nếu file SVG không có viewBox, Role C sẽ phải tự xử lý
     * (ví dụ: dùng width/height hoặc render 1:1).
     */
    SVGRectF m_viewBox;
    /**
     * @brief Danh sách các phần tử con cấp cao nhất trong tài liệu SVG.
     * Mỗi phần tử con là một con trỏ thông minh (std::unique_ptr) trỏ đến một đối tượng kế thừa từ SVGElement
     * (ví dụ: SVGRect, SVGCircle, SVGGroup, v.v.).
     * 
     * Role B (Parser) sẽ thêm các phần tử con vào đây sau khi phân tích (parse) file SVG.
     * 
     * Role C (UI) sẽ duyệt qua danh sách này để vẽ từng phần tử lên màn hình.
     * */
    std::vector<std::unique_ptr<SVGElement>> m_children;

public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     * Sẽ khởi tạo m_viewBox về giá trị 0 và m_children là một vector rỗng.
     * (Logic này sẽ do Role B implement trong file SVGDocument.cpp)
     *
     */
    SVGDocument();

    /**
     * @brief Hàm (setter) để Role B (Parser) thiết lập giá trị viewBox
     * sau khi phân tích (parse) thẻ <svg>.
     * @param viewBox Một struct SVGRectF chứa 4 giá trị của viewBox.
     */
    void setViewBox(const SVGRectF& viewBox);

    /**
     * @brief Thêm một phần tử con vào tài liệu.
     * Role B (Parser) sẽ gọi hàm này sau khi dùng SVGFactory để tạo ra một element mới.
     * * @param child Con trỏ thông minh (unique_ptr) đến phần tử con.
     * Quyền sở hữu sẽ được "move" vào 'm_children'.
     */
    void addChild(std::unique_ptr<SVGElement> child);

    /**
     * @brief Bắt đầu quá trình "duyệt" (và vẽ) cây SVG.
     * Đây là điểm khởi đầu của mẫu thiết kế Visitor Pattern.
     * * - Role C (UI) sẽ tạo ra một 'visitor' (ví dụ: QtRenderer) và gọi hàm này: myDocument.draw(myQtRenderer);
     * * - Logic của hàm này (do Role B implement) sẽ đơn giản là
     * lặp qua 'm_children' và gọi 'child->accept(visitor)' cho mỗi con.
     *
     * * @param visitor Một tham chiếu đến 'visitor' sẽ thực hiện hành động
     * (ví dụ: vẽ) trên từng nút.
     */
    void draw(NodeVisitor& visitor);

    // --- CÁC HÀM GETTERS (Cho Role C sử dụng) ---

    /**
     * @brief Lấy thông tin viewBox.
     * Role C (UI) sẽ dùng hàm này để biết cách
     * thiết lập QPainter (scale, translate) sao cho vừa với cửa sổ.
     * @return Một tham chiếu hằng (const ref) đến 'm_viewBox'.
     */
    const SVGRectF& getViewBox() const { return m_viewBox; }

    /**
     * @brief Lấy danh sách các con.
     * (Chủ yếu được dùng nội bộ bởi hàm 'draw',
     * nhưng để public cũng hữu ích cho việc debug).
     * @return Tham chiếu hằng đến vector chứa các con.
     */
    const std::vector<std::unique_ptr<SVGElement>>& getChildren() const {
        return m_children;
    }
};

#endif // SVG_DOCUMENT_H