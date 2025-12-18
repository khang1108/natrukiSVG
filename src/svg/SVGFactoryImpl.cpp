#include "SVGFactoryImpl.h"

#include "SVGCircle.h"
#include "SVGEllipse.h"
#include "SVGGroup.h"
#include "SVGLine.h"
#include "SVGPath.h"
#include "SVGPolygon.h"
#include "SVGPolyline.h"
#include "SVGRect.h"
#include "SVGPath.h"
#include "SVGText.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include <QColor>
#include <QString>

std::unique_ptr<SVGElement> SVGFactoryImpl::createElement(rapidxml::xml_node<char>* node,
                                                          SVGElement* parentElement)
{
    const char* name = node->name();
    if (!name || *name == '\0') {
        return nullptr; // Invalid node
    }
    //-------------------------------------------------
    // 1. Reset cache CSS khi bắt đầu parse file mới 
    if (strcmp(name, "svg") == 0) {
        m_cssClasses.clear();
    }

    // 2. Xử lý thẻ <style>: Đọc nội dung CSS lưu vào m_cssClasses
    if (strcmp(name, "style") == 0) {
        parseCssContent(node->value());
        return nullptr; // Không tạo đối tượng vẽ cho thẻ style
    }

    // 3. Xử lý thẻ <defs>: Trả về Group ẩn để tiếp tục duyệt con (tìm thẻ style bên trong)
    if (strcmp(name, "defs") == 0) {
        auto group = std::make_unique<SVGGroup>();
        SVGStyle defsStyle;
        defsStyle.isDisplayed = false; // Defs không hiển thị
        group->setStyle(defsStyle);
        return group;
    }
    //-------------------------------------------------
    // Get parent's style and transform for inheritance
    const SVGStyle& parentStyle = (parentElement) ? parentElement->getStyle() : SVGStyle();
    const SVGTransform& parentTransform =
        (parentElement) ? parentElement->getTransform() : SVGTransform();

    std::unique_ptr<SVGElement> newElement = nullptr;

  if (strcmp(name, "rect") == 0) {
    SVGRectF rect = {parseNumber(getAttr(node, "x"), 0.0), parseNumber(getAttr(node, "y"), 0.0),
                     parseNumber(getAttr(node, "width"), 0.0),
                     parseNumber(getAttr(node, "height"), 0.0)};
    SVGNumber rx = parseNumber(getAttr(node, "rx"), 0.0);
    SVGNumber ry = parseNumber(getAttr(node, "ry"), 0.0);
    newElement = std::make_unique<SVGRect>(rect, rx, ry);
  }
  else if (strcmp(name, "circle") == 0) {
    SVGPointF center = {parseNumber(getAttr(node, "cx"), 0.0),
                        parseNumber(getAttr(node, "cy"), 0.0)};
    SVGNumber r = parseNumber(getAttr(node, "r"), 0.0);
    newElement = std::make_unique<SVGCircle>(center, r);
  }
  else if (strcmp(name, "ellipse") == 0) {
    SVGPointF center = {parseNumber(getAttr(node, "cx"), 0.0),
                        parseNumber(getAttr(node, "cy"), 0.0)};
    SVGNumber rx = parseNumber(getAttr(node, "rx"), 0.0);
    SVGNumber ry = parseNumber(getAttr(node, "ry"), 0.0);
    newElement = std::make_unique<SVGEllipse>(center, rx, ry);
  }
  else if (strcmp(name, "line") == 0) {
    SVGPointF p1 = {parseNumber(getAttr(node, "x1"), 0.0), parseNumber(getAttr(node, "y1"), 0.0)};
    SVGPointF p2 = {parseNumber(getAttr(node, "x2"), 0.0), parseNumber(getAttr(node, "y2"), 0.0)};
    newElement = std::make_unique<SVGLine>(p1, p2);
  }
  else if (strcmp(name, "polygon") == 0) {
    newElement = std::make_unique<SVGPolygon>(parsePoints(getAttr(node, "points")));
  }
  else if (strcmp(name, "polyline") == 0) {
    newElement = std::make_unique<SVGPolyline>(parsePoints(getAttr(node, "points")));
  }
  else if (strcmp(name, "path") == 0) {
      const char* dAttr = getAttr(node, "d");
      newElement = std::make_unique<SVGPath>(dAttr);
  }
  else if (strcmp(name, "g") == 0) {
    newElement = std::make_unique<SVGGroup>();
  }
  else if (strcmp(name, "text") == 0) {
    SVGPointF pos = {parseNumber(getAttr(node, "x"), 0.0), parseNumber(getAttr(node, "y"), 0.0)};
    newElement = std::make_unique<SVGText>(pos, node->value());
  }

    // Apply style and transform to the element
    if (newElement) {
        SVGStyle style = parseStyle(node, parentStyle);
        newElement->setStyle(style);

    SVGTransform transform = parseTransform(node, SVGTransform());
    newElement->setTransform(transform);
  }

    return newElement;
}

/**
 * @brief Gets an attribute value from an XML node.
 *
 * Algorithm:
 * - Searches for an attribute with the given name
 * - Returns the attribute's value if found, empty string if not found
 * - Returns empty string if node is null
 *
 * @param node The XML node to search
 * @param attrName The name of the attribute to find
 * @return The attribute value, or empty string if not found
 */
const char* SVGFactoryImpl::getAttr(rapidxml::xml_node<char>* node, const char* attrName)
{
    if (!node)
        return "";
    rapidxml::xml_attribute<char>* attr = node->first_attribute(attrName);
    return (attr) ? attr->value() : "";
}

/**
 * @brief Removes leading and trailing whitespace from a string.
 *
 * Algorithm:
 * 1. Find first non-whitespace character (from start)
 * 2. Find last non-whitespace character (from end)
 * 3. Extract substring between these positions
 * 4. If string is all whitespace, return as-is
 *
 * Whitespace characters: space, tab, newline, carriage return
 *
 * @param str The string to trim
 * @return Trimmed string with leading/trailing whitespace removed
 */
std::string SVGFactoryImpl::trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first)
        return str; // String is all whitespace
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

/**
 * @brief Parses a numeric string to a floating-point number.
 *
 * Algorithm:
 * 1. Check if value is null or empty -> return default
 * 2. Trim whitespace from the string
 * 3. Check for "px" unit suffix and remove it if present
 * 4. Use std::stod to convert string to double
 * 5. Return default value if parsing fails
 *
 * Supported formats:
 * - "10" -> 10.0
 * - "10.5" -> 10.5
 * - "10px" -> 10.0 (removes px unit)
 * - "-5.2" -> -5.2
 *
 * @param value The string to parse
 * @param defaultValue Value to return if parsing fails
 * @return Parsed number, or defaultValue if parsing fails
 */
SVGNumber SVGFactoryImpl::parseNumber(const char* value, SVGNumber defaultValue)
{
    if (!value || *value == '\0') {
        return defaultValue;
    }
    std::string strValue = trim(std::string(value));
    if (strValue.empty()) {
        return defaultValue;
    }
    try {
        // Check for "px" unit and remove it
        size_t unitPos = std::string::npos;
        unitPos = strValue.rfind("px");
        if (unitPos != std::string::npos && unitPos == strValue.length() - 2) {
            return std::stod(strValue.substr(0, unitPos));
        }

        return std::stod(strValue);
    }
    catch (...) {
        return defaultValue; // Parsing failed
    }
}

/**
 * @brief Parses a points string into a vector of points.
 *
 * Algorithm:
 * 1. Parse coordinate pairs (x, y) from the string
 * 2. Skip whitespace and commas between numbers
 * 3. Read x coordinate, then y coordinate
 * 4. Add point (x, y) to vector
 * 5. Continue until no more coordinates can be read
 *
 * Points format: "x1,y1 x2,y2 x3,y3" or "x1 y1 x2 y2 x3 y3"
 * - Commas and whitespace are both allowed as separators
 * - Pairs must be complete (x and y), incomplete pairs are ignored
 *
 * Example: "10,20 30,40 50,60" -> [(10,20), (30,40), (50,60)]
 *
 * @param pointsStr The points string to parse
 * @return Vector of parsed points
 */
std::vector<SVGPointF> SVGFactoryImpl::parsePoints(const char* pointsStr)
{
    std::vector<SVGPointF> points;
    if (!pointsStr || *pointsStr == '\0') {
        return points; // Empty string
    }
    std::stringstream ss(pointsStr);
    SVGNumber x, y;
    while (ss >> x) {
        // Skip separators (commas, whitespace)
        while (ss.peek() == ',' || std::isspace(ss.peek())) {
            ss.get();
        }
        if (!(ss >> y)) {
            break; // No y coordinate found, stop
        }
        points.push_back({x, y});
    }
    return points;
}

/**
 * @brief Parses a color string into an SVGColor object.
 *
 * Algorithm:
 * 1. Convert string to lowercase for case-insensitive matching
 * 2. Check for special color names: "none", "black", "red", "green", "blue", "white"
 * 3. Check for RGB format: "rgb(r, g, b)"
 *    - Extract r, g, b values (0-255)
 *    - Clamp values to valid range [0, 255]
 * 4. Check for hex format: "#RRGGBB" or "#RGB"
 *    - 6-digit: "#FF0000" -> (255, 0, 0)
 *    - 3-digit: "#F00" -> (255, 0, 0) (each digit duplicated)
 *    - Parse hex digits using base-16 conversion
 * 5. Return default value if format is not recognized
 *
 * Supported formats:
 * - Named colors: "none", "black", "red", "green", "blue", "white"
 * - RGB: "rgb(255, 0, 0)" -> red
 * - Hex 6-digit: "#ff0000" or "#FF0000" -> red
 * - Hex 3-digit: "#f00" -> red (shorthand)
 *
 * @param colorStr The color string to parse
 * @param defaultValue Value to return if parsing fails
 * @return SVGColor object representing the parsed color
 */
SVGColor SVGFactoryImpl::parseColor(std::string colorStr, const SVGColor& defaultValue)
{
    if (colorStr.empty())
        return defaultValue;

    // 1. Xử lý trường hợp "none" hoặc Gradient "url(...)" 
    if (colorStr == "none" || colorStr.find("url(") != std::string::npos) {
        return {0, 0, 0, 0, true}; // isNone = true
    }

    // 2. Xử lý thủ công cho "rgb(r, g, b)" vì QColor không hỗ trợ cú pháp này(có lẽ)
    if (colorStr.find("rgb(") != std::string::npos) {
        SVGColor color{0, 0, 0, 255, false};
        try {
            size_t start = colorStr.find('(') + 1;
            size_t end = colorStr.find(')');
            if (end == std::string::npos)
                return defaultValue;

            std::string args = colorStr.substr(start, end - start);
            // Thay thế dấu phẩy bằng khoảng trắng để dễ parse
            std::replace(args.begin(), args.end(), ',', ' ');

            std::stringstream ss(args);
            int r, g, b;
            ss >> r >> g >> b; // stringstream tự bỏ qua khoảng trắng

            if (!ss.fail()) {
                // Clamp giá trị trong khoảng 0-255
                color.r = static_cast<unsigned char>(std::max(0, std::min(r, 255)));
                color.g = static_cast<unsigned char>(std::max(0, std::min(g, 255)));
                color.b = static_cast<unsigned char>(std::max(0, std::min(b, 255)));
                return color;
            }
        }
        catch (...) {
        }
        return defaultValue; // Parse lỗi thì trả về mặc định
    }

    // 3. Các trường hợp còn lại (Tên màu: "red", Hex: "#F00") cho QColor 
    QColor qc(QString::fromStdString(colorStr));
    if (qc.isValid()) {
        return {static_cast<unsigned char>(qc.red()), static_cast<unsigned char>(qc.green()),
                static_cast<unsigned char>(qc.blue()), static_cast<unsigned char>(qc.alpha()),
                false};
    }

    // Nếu QColor cũng không xác định được
    return defaultValue;
}

// QColor không đọc được mã rgb nên tự viết thủ công

// --- NEW: Parse nội dung thẻ <style> ---
void SVGFactoryImpl::parseCssContent(const std::string& content)
{
    if (content.empty())
        return;
    size_t pos = 0;
    while (pos < content.length()) {
        // 1. Tìm tên class (bắt đầu bằng dấu chấm)
        size_t startClass = content.find('.', pos);
        if (startClass == std::string::npos)
            break;

        size_t startBrace = content.find('{', startClass);
        if (startBrace == std::string::npos)
            break;

        // Lấy tên class (bỏ dấu chấm)
        std::string className = content.substr(startClass + 1, startBrace - startClass - 1);
        className = trim(className); // Xóa khoảng trắng thừa

        // 2. Tìm nội dung style bên trong {}
        size_t endBrace = content.find('}', startBrace);
        if (endBrace == std::string::npos)
            break;

        std::string body = content.substr(startBrace + 1, endBrace - startBrace - 1);

        // 3. Parse từng thuộc tính
        std::stringstream ss(body);
        std::string item;
        while (std::getline(ss, item, ';')) {
            size_t colon = item.find(':');
            if (colon != std::string::npos) {
                std::string key = trim(item.substr(0, colon));
                std::string val = trim(item.substr(colon + 1));
                if (!key.empty()) {
                    // Lưu vào biến thành viên m_cssClasses
                    m_cssClasses[className][key] = val;
                }
            }
        }
        pos = endBrace + 1;
    }
}
// UPDATE: CSS CLASS ADDED
SVGStyle SVGFactoryImpl::parseStyle(rapidxml::xml_node<char>* node, const SVGStyle& parentStyle)
{
    SVGStyle style;
    style.inheritFrom(parentStyle);           // Start with inherited values
    std::map<std::string, std::string> attrs; // Collect all style attributes
    
    const char* classAttr = getAttr(node, "class");
    if (classAttr && *classAttr != '\0') {
        std::string className(classAttr);
        std::stringstream ss(className);
        std::string singleClass;
        while (ss >> singleClass) {
            if (m_cssClasses.count(singleClass)) {
                for (const auto& pair : m_cssClasses[singleClass]) {
                    attrs[pair.first] = pair.second;
                }
            }
        }
    }

    if (auto* attr = node->first_attribute("fill"))
        attrs["fill"] = attr->value();
    if (auto* attr = node->first_attribute("stroke"))
        attrs["stroke"] = attr->value();
    if (auto* attr = node->first_attribute("stroke-width"))
        attrs["stroke-width"] = attr->value();
    if (auto* attr = node->first_attribute("font-size"))
        attrs["font-size"] = attr->value();
    if (auto* attr = node->first_attribute("font-family"))
        attrs["font-family"] = attr->value();
    if (auto* attr = node->first_attribute("fill-rule"))
        attrs["fill-rule"] = attr->value();
    if (auto* attr = node->first_attribute("display"))
        attrs["display"] = attr->value();
    if (auto* attr = node->first_attribute("fill-opacity"))
        attrs["fill-opacity"] = attr->value();
    if (auto* attr = node->first_attribute("stroke-opacity"))
        attrs["stroke-opacity"] = attr->value();
    if (auto* attr = node->first_attribute("opacity"))
        attrs["opacity"] = attr->value();

    const char* styleStr = getAttr(node, "style");
    if (styleStr && *styleStr != '\0') {
        std::stringstream ss(styleStr);
        std::string declaration;
        while (std::getline(ss, declaration, ';')) {
            size_t colonPos = declaration.find(':');
            if (colonPos != std::string::npos) {
                std::string key = trim(declaration.substr(0, colonPos));
                std::string value = trim(declaration.substr(colonPos + 1));
                attrs[key] = value;
            }
        }
    }

    if (attrs.count("fill"))
        style.fillColor = parseColor(attrs["fill"], style.fillColor);
    if (attrs.count("stroke"))
        style.strokeColor = parseColor(attrs["stroke"], style.strokeColor);
    if (attrs.count("stroke-width"))
        style.strokeWidth = parseNumber(attrs["stroke-width"].c_str(), style.strokeWidth);
    if (attrs.count("font-size"))
        style.fontSize = parseNumber(attrs["font-size"].c_str(), style.fontSize);
    if (attrs.count("font-family"))
        style.fontFamily = attrs["font-family"];

    if (attrs.count("fill-rule")) {
        if (attrs["fill-rule"] == "evenodd") {
            style.fillRule = SVGFillRule::EvenOdd;
        }
        else {
            style.fillRule = SVGFillRule::NonZero;
        }
    }

    auto parseOpacity = [&](const std::string& value, SVGNumber current) -> SVGNumber {
        SVGNumber parsed = parseNumber(value.c_str(), current);
        if (parsed < 0.0)
            parsed = 0.0;
        if (parsed > 1.0)
            parsed = 1.0;
        return parsed;
    };

    if (attrs.count("fill-opacity")) {
        style.fillOpacity = parseOpacity(attrs["fill-opacity"], style.fillOpacity);
    }
    if (attrs.count("stroke-opacity")) {
        style.strokeOpacity = parseOpacity(attrs["stroke-opacity"], style.strokeOpacity);
    }
    if (attrs.count("opacity")) {
        SVGNumber opacity = parseOpacity(attrs["opacity"], 1.0);
        if (style.fillOpacity < 0.0) {
            style.fillOpacity = opacity;
        }
        else {
            style.fillOpacity = std::clamp(style.fillOpacity * opacity, 0.0, 1.0);
        }
        if (style.strokeOpacity < 0.0) {
            style.strokeOpacity = opacity;
        }
        else {
            style.strokeOpacity = std::clamp(style.strokeOpacity * opacity, 0.0, 1.0);
        }
    }

  if (attrs.count("display")) {
    if (attrs["display"] == "none") {
      style.isDisplayed = false;
    }
    else {
      style.isDisplayed = true;
    }
  }

// 1. Parse Text Anchor (Căn lề)
  const char* anchor = getAttr(node, "text-anchor");
  if (anchor) {
      style.textAnchor = anchor;
  }

  // 2. Parse Font Style (In nghiêng)
  const char* fStyle = getAttr(node, "font-style");
  if (fStyle) {
      std::string fs(fStyle);
      // Nếu gặp italic hoặc oblique thì bật cờ isItalic
      if (fs.find("italic") != std::string::npos || fs.find("oblique") != std::string::npos) {
          style.isItalic = true;
      }
      else if (fs == "normal") {
          style.isItalic = false;
      }
  }

  // 3. Parse Font Weight (In đậm)
  const char* fWeight = getAttr(node, "font-weight");
  if (fWeight) {
      std::string fw(fWeight);
      if (fw == "bold" || fw == "bolder") {
          style.isBold = true;
          style.fontWeight = 75; // Qt::Bold tương đương 75
      }
      else if (fw == "normal") {
          style.isBold = false;
          style.fontWeight = 50; // Qt::Normal tương đương 50
      }
  }
  
  style.applyDefaults();
  return style;
}

/**
 * @brief Parses the transform attribute and accumulates with parent transform.
 *
 * Algorithm:
 * 1. Start with parent transform (accumulated from ancestors)
 * 2. Parse the transform attribute string (if present)
 * 3. Parse transform functions: translate(), scale(), rotate(), matrix()
 * 4. Build local transform by multiplying functions in order
 * 5. Multiply parent transform with local transform: result = local * parent
 *    - This ensures parent transforms are applied first, then local
 *    - SVG applies transforms left-to-right, so we multiply left-to-right
 *
 * Transform string format: "translate(10,20) rotate(45) scale(2)"
 * - Functions are separated by whitespace
 * - Each function has format: "functionName(arg1, arg2, ...)"
 * - Functions are applied in the order they appear
 *
 * Supported functions:
 * - translate(tx, ty): Move by (tx, ty). If ty omitted, ty=0
 * - scale(sx, sy): Scale by (sx, sy). If sy omitted, sy=sx (uniform scaling)
 * - rotate(angle): Rotate by angle degrees (counterclockwise)
 * - matrix(a, b, c, d, e, f): 2x3 transformation matrix
 *
 * Transform accumulation:
 * - Parent transforms are applied first
 * - Local transforms are applied after parent
 * - Matrix multiplication: result = localTransform * parentTransform
 *
 * @param node The XML node to parse transform from
 * @param parentTransform The parent's accumulated transform
 * @return SVGTransform representing the accumulated transform (parent + local)
 */
SVGTransform SVGFactoryImpl::parseTransform(rapidxml::xml_node<char>* node,
                                            const SVGTransform& parentTransform)
{
    SVGTransform worldTransform = parentTransform; // Start with parent transform
    const char* transformStr = getAttr(node, "transform");
    if (!transformStr || *transformStr == '\0') {
        return worldTransform; // No local transform, return parent
    }

    SVGTransform localTransform; // Build local transform from string
    std::string str(transformStr);
    size_t pos = 0;
    // Parse each transform function in the string
    while (pos < str.length()) {
        // Find start of function name (skip whitespace)
        size_t startFunc = str.find_first_not_of(" \t\r\n", pos);
        if (startFunc == std::string::npos)
            break;

        // Find opening parenthesis
        size_t startArgs = str.find('(', startFunc);
        if (startArgs == std::string::npos)
            break;

        // Find closing parenthesis
        size_t endArgs = str.find(')', startArgs);
        if (endArgs == std::string::npos)
            break;

        std::string funcName = str.substr(startFunc, startArgs - startFunc);
        std::string argsStr = str.substr(startArgs + 1, endArgs - (startArgs + 1));

        std::stringstream ss(argsStr);
        char comma;
        SVGNumber n1, n2, n3;

        std::replace(argsStr.begin(), argsStr.end(), ',', ' ');
        ss.str(argsStr);

        if (funcName == "translate") {
            ss >> n1;
            ss >> n2;
            if (ss.fail())
                n2 = 0;
            SVGTransform tempT;
            tempT.translate(n1, n2);
            localTransform.multiply(tempT);
        }
        else if (funcName == "scale") {
            ss >> n1;
            ss >> n2;
            if (ss.fail())
                n2 = n1;
            SVGTransform tempS;
            tempS.scale(n1, n2);
            localTransform.multiply(tempS);
        }
        else if (funcName == "rotate") {
            ss >> n1;
            SVGTransform tempR;
            tempR.rotate(n1);
            localTransform.multiply(tempR);
        }
        pos = endArgs + 1;
    }

    // In SVG, transforms are applied in order: parent first, then local.
    // When we have: <g transform="A"><path transform="B"/></g>
    // The path should be transformed by A then B (parent then local).
    //
    // In matrix multiplication, to apply transform A then B to point P:
    //   First: A * P
    //   Then: B * (A * P) = (B * A) * P
    // So the combined matrix is B * A (local * parent).
    //
    // The multiply function does: this = this * other
    // So: result.multiply(worldTransform) means result = result * worldTransform
    // If result = localTransform, then: result = localTransform * worldTransform = local * parent
    // This correctly applies parent first, then local.
    SVGTransform result = localTransform;
    result.multiply(worldTransform); // result = local * parent (correct: parent first, then local)
    return result;
}