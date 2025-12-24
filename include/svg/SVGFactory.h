#ifndef SVG_FACTORY_H
#define SVG_FACTORY_H

#include "SVGStyle.h"
#include "SVGTransform.h"
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
    // Creates an element from an XML node, using provided base style and transform
    virtual std::unique_ptr<SVGElement> createElement(rapidxml::xml_node<char>* node,
                                                      const SVGStyle& baseStyle,
                                                      const SVGTransform& baseTransform) = 0;
                                                      
    // Parses a CSS declaration string (e.g. "fill:red; stroke:blue") into an SVGStyle
    virtual SVGStyle createStyleFromCSS(const std::string& cssContent) = 0;
  
  // Helper to parse stops for a gradient
  virtual void parseStops(SVGElement* gradient, rapidxml::xml_node<char>* node) {}

  /**
   * @brief Tạo ra một instance (thể hiện) cụ thể của Factory.
   */
  static std::unique_ptr<SVGFactory> createDefaultFactory();
};

#endif // SVG_FACTORY_H