#ifndef SVG_FACTORY_IMPL_H
#define SVG_FACTORY_IMPL_H

#include "SVGFactory.h"
#include "SVGStyle.h"
#include "SVGTransform.h"
#include "Types.h"
#include "rapidxml.hpp" 

#include <map>
#include <string>
#include <vector>

/**
 * @class SVGFactoryImpl
 * @brief Lớp triển khai cụ thể cho SVGFactory.
 * Đã cập nhật để hỗ trợ đọc CSS Class.
 */
class SVGFactoryImpl : public SVGFactory
{
  public:
    SVGFactoryImpl() = default;
    virtual ~SVGFactoryImpl() = default;

    virtual std::unique_ptr<SVGElement> createElement(rapidxml::xml_node<char>* node,
                                                      SVGElement* parentElement) override;

  private:
    // --- CÁC HÀM TIỆN ÍCH (HELPERS) ---
    const char* getAttr(rapidxml::xml_node<char>* node, const char* attrName);
    std::string trim(const std::string& str);
    SVGNumber parseNumber(const char* value, SVGNumber defaultValue = 0.0);
    std::vector<SVGPointF> parsePoints(const char* pointsStr);
    SVGColor parseColor(std::string colorStr, const SVGColor& defaultValue);

    SVGStyle parseStyle(rapidxml::xml_node<char>* node, const SVGStyle& parentStyle);
    SVGTransform parseTransform(rapidxml::xml_node<char>* node,
                                const SVGTransform& parentTransform);

    // --- MỚI: Hàm xử lý nội dung CSS trong thẻ <style> ---
    void parseCssContent(const std::string& content);

  private:
    // --- MỚI: Biến thành viên lưu trữ CSS Class ---
    std::map<std::string, std::map<std::string, std::string>> m_cssClasses;
};

#endif 