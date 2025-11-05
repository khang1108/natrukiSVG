#ifndef SVG_FACTORY_H
#define SVG_FACTORY_H

#include <memory>   
#include <string>

// --- Khai báo trước (Forward Declaration) ---
// Cho RapidXML
namespace rapidxml {
    template<class Ch> class xml_node; //
}

// Cho lớp cha
class SVGElement;

/**
 * @brief Giao diện (Interface) cho Factory Pattern.
 *
 * Đây là "cầu nối" chính giữa Role B (Parser) và Role A (Definitions).
 */
class SVGFactory {
public:
    SVGFactory() = default;
    virtual ~SVGFactory() = default;

    /**
     * @brief Phương thức Factory chính (lớp trừu tượng).
     * Nhiệm vụ của nó là nhận một node XML và "nhả" ra
     * một đối tượng SVGElement tương ứng.
     *
     * @param node Con trỏ đến node XML của RapidXML (ví dụ: node <rect>).
     *
     * @return Con trỏ thông minh (unique_ptr) đến đối tượng mới
     * (ví dụ: SVGRect, SVGCircle...).
     *
     * * - Vai trò của Role B (Implement):
     * Đây là một NHIỆM VỤ CỐT LÕI của Role B. B phải:
     * 1. Tạo một lớp mới (ví dụ: 'MySVGFactoryImpl') kế thừa 'SVGFactory'.
     * 2. Implement (viết code .cpp) cho hàm 'createElement' này.
     * 3. Bên trong hàm, B phải 'switch/case' hoặc 'if/else'
     * dựa trên 'node->name()' (lấy từ RapidXML).
     * 4. Ví dụ: 'if (strcmp(node->name(), "rect") == 0)'
     * 5. Sau đó, B phải đọc các thuộc tính (attribute) từ 'node'
     * (ví dụ: 'node->first_attribute("x")').
     * 6. Cuối cùng, gọi 'std::make_unique<SVGRect>(...)' với các
     * giá trị đã parse và trả về.
     */
    virtual std::unique_ptr<SVGElement> createElement(
        rapidxml::xml_node<char>* node) = 0;
};

#endif // SVG_FACTORY_H