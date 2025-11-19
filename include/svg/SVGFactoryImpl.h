#ifndef SVG_FACTORY_IMPL_H
#define SVG_FACTORY_IMPL_H

#include "SVGFactory.h" // Kế thừa từ "hợp đồng"

// Bao gồm các thư viện cần thiết cho các hàm helper
#include "SVGStyle.h"
#include "SVGTransform.h"
#include "Types.h"
#include "rapidxml.hpp"

#include <map>
#include <string>
#include <vector>

/**
 * @class SVGFactoryImpl
 * @brief Lớp triển khai (Implementation) cụ thể cho SVGFactory.
 * Đây là nơi Role B viết logic parse cốt lõi.
 */
class SVGFactoryImpl : public SVGFactory
{

public:
  SVGFactoryImpl() = default;
  virtual ~SVGFactoryImpl() = default;

  /**
   * @brief Implement hàm 'createElement' từ giao diện.
   * (Logic if/else sẽ nằm trong file .cpp)
   */
  virtual std::unique_ptr<SVGElement> createElement(rapidxml::xml_node<char>* node,
                                                    SVGElement* parentElement) override;

private:
  // --- CÁC HÀM TIỆN ÍCH (HELPERS) ĐỂ PARSE ---
  // (Đây là các hàm private, bị che giấu khỏi phần còn lại của dự án)

  const char* getAttr(rapidxml::xml_node<char>* node, const char* attrName);

  std::string trim(const std::string& str);

  SVGNumber parseNumber(const char* value, SVGNumber defaultValue = 0.0);

  std::vector<SVGPointF> parsePoints(const char* pointsStr);

  SVGColor parseColor(std::string colorStr, const SVGColor& defaultValue);

  SVGStyle parseStyle(rapidxml::xml_node<char>* node, const SVGStyle& parentStyle);

  SVGTransform parseTransform(rapidxml::xml_node<char>* node, const SVGTransform& parentTransform);
};

#endif // SVG_FACTORY_IMPL_H