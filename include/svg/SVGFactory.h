#ifndef SVG_FACTORY_H
#define SVG_FACTORY_H

#include <memory>
#include <string>

// --- Khai báo trước (Forward Declaration) ---
namespace rapidxml
{
  template<class Ch> class xml_node;
}
class SVGElement;
// (Không cần SVGStyle ở đây nữa)

/**
 * @brief Giao diện (Interface) cho Factory Pattern.
 */
class SVGFactory
{
public:
  SVGFactory() = default;
  virtual ~SVGFactory() = default;

  /**
   * @brief Phương thức Factory chính (PHIÊN BẢN ĐÃ SỬA)
   *
   * @param node Con trỏ đến node XML của RapidXML.
   * @param parentElement (THAM SỐ DUY NHẤT) Con trỏ đến node CHA.
   * * LÝ DO: parentElement chứa CẢ parentStyle VÀ parentTransform.
   * * Factory sẽ tự lấy 2 giá trị đó từ đây.
   * * Nếu parentElement là NULL (cấp cao nhất), Factory sẽ dùng giá trị mặc định.
   */
  virtual std::unique_ptr<SVGElement>
  createElement(rapidxml::xml_node<char>* node,
                SVGElement* parentElement // <--- SỬA Ở ĐÂY (chỉ 2 tham số)
                ) = 0;

  /**
   * @brief Tạo ra một instance (thể hiện) cụ thể của Factory.
   */
  static std::unique_ptr<SVGFactory> createDefaultFactory();
};

#endif // SVG_FACTORY_H