#include "SVGFactoryImpl.h"

#include "SVGCircle.h"
#include "SVGEllipse.h"
#include "SVGGradient.h"
#include "SVGGroup.h"
#include "SVGLine.h"
#include "SVGPath.h"
#include "SVGPolygon.h"
#include "SVGPolyline.h"
#include "SVGRect.h"
#include "SVGText.h"

#include <QColor>
#include <QString>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::unique_ptr<SVGElement> SVGFactoryImpl::createElement(rapidxml::xml_node<char>* node,
                                                          const SVGStyle& baseStyle,
                                                          const SVGTransform& baseTransform)
{
    const char* name = node->name();
    if (!name || *name == '\0') {
        return nullptr; // Invalid node
    }

    // Use passed base style/transform
    const SVGStyle& parentStyle = baseStyle;
    const SVGTransform& parentTransform = baseTransform;

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
        SVGPointF p1 = {parseNumber(getAttr(node, "x1"), 0.0),
                        parseNumber(getAttr(node, "y1"), 0.0)};
        SVGPointF p2 = {parseNumber(getAttr(node, "x2"), 0.0),
                        parseNumber(getAttr(node, "y2"), 0.0)};
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
    else if (strcmp(name, "g") == 0 || strcmp(name, "defs") == 0) {
        newElement = std::make_unique<SVGGroup>();
    }
    else if (strcmp(name, "text") == 0) {
        SVGPointF pos = {parseNumber(getAttr(node, "x"), 0.0),
                         parseNumber(getAttr(node, "y"), 0.0)};
        newElement = std::make_unique<SVGText>(pos, node->value());
    }
    else if (strcmp(name, "linearGradient") == 0) {
        auto grad = std::make_unique<SVGLinearGradient>();
        grad->x1 = parseNumber(getAttr(node, "x1"), 0.0);
        grad->y1 = parseNumber(getAttr(node, "y1"), 0.0);
        grad->x2 = parseNumber(getAttr(node, "x2"), 1.0); // Default 100%
        grad->y2 = parseNumber(getAttr(node, "y2"), 0.0);

        const char* units = getAttr(node, "gradientUnits");
        if (units && strcmp(units, "userSpaceOnUse") == 0) {
            grad->gradientUnits = SVGGradientUnits::UserSpaceOnUse;
        }

        const char* spread = getAttr(node, "spreadMethod");
        if (spread) {
            if (strcmp(spread, "reflect") == 0)
                grad->spreadMethod = SVGSpreadMethod::Reflect;
            else if (strcmp(spread, "repeat") == 0)
                grad->spreadMethod = SVGSpreadMethod::Repeat;
        }

        // Parse xlink:href or href for gradient inheritance
        const char* href = getAttr(node, "xlink:href");
        if (!href || *href == '\0') {
            href = getAttr(node, "href");
        }
        if (href && *href != '\0') {
            std::string hrefStr(href);
            if (!hrefStr.empty() && hrefStr[0] == '#') {
                grad->href = hrefStr.substr(1); // Remove '#'
            }
            else {
                grad->href = hrefStr;
            }
        }

        // Parse gradientTransform
        const char* gradTransform = getAttr(node, "gradientTransform");
        std::cout << "DEBUG: linearGradient ID="
                  << (getAttr(node, "id") ? getAttr(node, "id") : "(none)")
                  << " gradientTransform=" << (gradTransform ? gradTransform : "(null)")
                  << std::endl;
        if (gradTransform && *gradTransform != '\0') {
            SVGTransform transform = parseTransformString(gradTransform);
            grad->setTransform(transform);
            std::cout << "DEBUG: Applied transform matrix: [" << transform.getMatrix()[0][0] << ","
                      << transform.getMatrix()[0][1] << "," << transform.getMatrix()[0][2] << "]"
                      << std::endl;
        }

        // Parse gradient stops
        parseStops(grad.get(), node);

        newElement = std::move(grad);
    }
    else if (strcmp(name, "radialGradient") == 0) {
        auto grad = std::make_unique<SVGRadialGradient>();
        grad->cx = parseNumber(getAttr(node, "cx"), 0.5);
        grad->cy = parseNumber(getAttr(node, "cy"), 0.5);
        grad->r = parseNumber(getAttr(node, "r"), 0.5);
        grad->fx = parseNumber(getAttr(node, "fx"), grad->cx); // Default to cx
        grad->fy = parseNumber(getAttr(node, "fy"), grad->cy); // Default to cy

        const char* units = getAttr(node, "gradientUnits");
        if (units && strcmp(units, "userSpaceOnUse") == 0) {
            grad->gradientUnits = SVGGradientUnits::UserSpaceOnUse;
        }

        const char* spread = getAttr(node, "spreadMethod");
        if (spread) {
            if (strcmp(spread, "reflect") == 0)
                grad->spreadMethod = SVGSpreadMethod::Reflect;
            else if (strcmp(spread, "repeat") == 0)
                grad->spreadMethod = SVGSpreadMethod::Repeat;
        }

        // Parse xlink:href or href for gradient inheritance
        const char* href = getAttr(node, "xlink:href");
        if (!href || *href == '\0') {
            href = getAttr(node, "href");
        }
        if (href && *href != '\0') {
            std::string hrefStr(href);
            if (!hrefStr.empty() && hrefStr[0] == '#') {
                grad->href = hrefStr.substr(1); // Remove '#'
            }
            else {
                grad->href = hrefStr;
            }
        }

        // Parse gradientTransform
        const char* gradTransform = getAttr(node, "gradientTransform");
        std::cout << "DEBUG: radialGradient ID="
                  << (getAttr(node, "id") ? getAttr(node, "id") : "(none)")
                  << " gradientTransform=" << (gradTransform ? gradTransform : "(null)")
                  << " ptr=" << (void*)gradTransform
                  << " strlen=" << (gradTransform ? strlen(gradTransform) : 0) << std::endl;
        if (gradTransform && *gradTransform != '\0') {
            std::cout << "DEBUG: About to parse transform string: '" << gradTransform << "'"
                      << std::endl;
            SVGTransform transform = parseTransformString(gradTransform);
            std::cout << "DEBUG: After parseTransformString, matrix: ["
                      << transform.getMatrix()[0][0] << "," << transform.getMatrix()[0][1] << ","
                      << transform.getMatrix()[0][2] << "]" << std::endl;
            grad->setTransform(transform);
            std::cout << "DEBUG: Transform set on gradient object" << std::endl;
        }

        // Parse gradient stops
        parseStops(grad.get(), node);

        newElement = std::move(grad);
    }
    else if (strcmp(name, "stop") == 0) {
        // stop elements are not SVGElement in our hierarchy (they are data),
        // but to fit in the recursion, we might need a dummy element or handle them differently.
        // However, the current architecture expects SVGElement.
        // A better way is to treat Gradient as a container (Group-like) and Stop as a child?
        // OR, handle stops inside the Gradient parsing manually?
        // Since specific structure requires `parseRecursive` to handle children, let's treat Stop
        // as an Element? But Stop is simple. Let's create a temporary solution: Check if parent is
        // a Gradient, and add stop directly to it? `parseRecursive` doesn't know about
        // Gradient-specifics easily without casting.

        // Let's assume we can't easily change `parseRecursive` right now.
        // I will NOT create an element for `stop` here.
        // Instead, `parseRecursive` should handle non-element children?
        // Or I make `SVGGradient` a `SVGGroup` subclass? No, `SVGGroup` has `m_children`.
        // `SVGGradient` has `m_stops`.

        // Let's modify `createElement` to return nullptr for "stop",
        // AND modify `SVGDocument::parseRecursive` to handle "stop" if parent is gradient.

        // BUT `createElement` is called first.

        // ALTERNATIVE: Make `SVGStopElement` that inherits `SVGElement`.
        // Then in `parseRecursive`, we can check type.
        // But `SVGStop` is a struct in header.

        // Let's just return nullptr here and let `parseRecursive` handle `stop` parsing manually?
        // No, `parseRecursive` calls `createElement`.

        // Let's go with the manual parsing of children inside `SVGFactoryImpl`? No, factory creates
        // ONE element.

        // BEST APPROACH: Add `parseStops` helper in `SVGDocument` or `SVGFactory`.
        // Recursion in `SVGDocument` iterates children.

        // FOR NOW: I will treat specific parsing in `SVGDocument::parseRecursive`.
        // So here, return nullptr for stop.
        return nullptr;
    }

    // Apply style and transform to the element
    if (newElement) {
        // Parse ID
        const char* id = getAttr(node, "id");
        if (id && *id != '\0') {
            newElement->setId(id);
        }

        SVGStyle style = parseStyle(node, parentStyle);
        newElement->setStyle(style);

        // Only parse "transform" attribute for non-gradient elements
        // Gradients use "gradientTransform" which is already parsed above
        if (!dynamic_cast<SVGGradient*>(newElement.get())) {
            SVGTransform transform = parseTransform(node, SVGTransform());
            newElement->setTransform(transform);
        }
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

        // Check for "%" unit
        if (strValue.back() == '%') {
            return std::stod(strValue.substr(0, strValue.size() - 1)) / 100.0;
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

SVGStyle SVGFactoryImpl::parseStyle(rapidxml::xml_node<char>* node, const SVGStyle& parentStyle)
{
    SVGStyle style;
    style.inheritFrom(parentStyle);           // Start with inherited values
    std::map<std::string, std::string> attrs; // Collect all style attributes

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

    if (attrs.count("fill")) {
        std::string val = attrs["fill"];
        if (val.find("url(") != std::string::npos) {
            style.fillUrl = val;
            style.fillColor = {0, 0, 0, 0, true}; // None
        }
        else {
            style.fillColor = parseColor(val, style.fillColor);
            style.fillUrl = ""; // Clear inherited
        }
    }

    if (attrs.count("stroke")) {
        std::string val = attrs["stroke"];
        if (val.find("url(") != std::string::npos) {
            style.strokeUrl = val;
            style.strokeColor = {0, 0, 0, 0, true}; // None
        }
        else {
            style.strokeColor = parseColor(val, style.strokeColor);
            style.strokeUrl = "";
        }
    }
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
    // FIX: Do NOT apply parent transform here.
    // The renderer applies parent logic by traversing the tree.
    // Baking parent transform here results in Double Transformation (Parent * Parent * Local).

    SVGTransform localTransform; // Identity start
    const char* transformStr = getAttr(node, "transform");
    if (!transformStr || *transformStr == '\0') {
        return localTransform; // Return Identity
    }

    std::string str(transformStr);
    size_t pos = 0;
    // ... rest of parsing logic ...
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

    // Do NOT multiply with parentTransform.
    // Return only the local transform for this node.
    return localTransform;
}

/**
 * @brief Parse transform from a string (used for gradientTransform).
 *
 * @param transformStr The transform string to parse
 * @return SVGTransform representing the parsed transform
 */
SVGTransform SVGFactoryImpl::parseTransformString(const char* transformStr)
{
    SVGTransform localTransform; // Identity start
    if (!transformStr || *transformStr == '\0') {
        return localTransform; // Return Identity
    }

    std::string str(transformStr);
    size_t pos = 0;

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
            ss >> n2;
            ss >> n3;

            SVGTransform tempR;
            if (!ss.fail() && ss.eof() == false) {
                // rotate(angle cx cy) - rotate around center point
                // Equivalent to: translate(cx, cy) rotate(angle) translate(-cx, -cy)
                SVGTransform trans1, rot, trans2;
                trans1.translate(n2, n3);
                rot.rotate(n1);
                trans2.translate(-n2, -n3);

                tempR = trans2;
                tempR.multiply(rot);
                tempR.multiply(trans1);
            }
            else {
                // rotate(angle) - simple rotation around origin
                tempR.rotate(n1);
            }
            localTransform.multiply(tempR);
        }
        else if (funcName == "matrix") {
            // matrix(a, b, c, d, e, f)
            // Creates transformation matrix:
            // | a  c  e |
            // | b  d  f |
            // | 0  0  1 |
            SVGNumber a, b, c, d, e, f;
            ss >> a >> b >> c >> d >> e >> f;
            if (!ss.fail()) {
                SVGTransform tempM;
                tempM.setMatrix(a, b, c, d, e, f);
                localTransform.multiply(tempM);
            }
        }
        pos = endArgs + 1;
    }

    return localTransform;
}

void SVGFactoryImpl::parseStops(SVGElement* gradientElement, rapidxml::xml_node<char>* node)
{
    SVGGradient* gradient = dynamic_cast<SVGGradient*>(gradientElement);
    if (!gradient)
        return;

    for (rapidxml::xml_node<char>* child = node->first_node(); child;
         child = child->next_sibling()) {
        if (strcmp(child->name(), "stop") == 0) {
            SVGStop stop;
            // Parse offset
            const char* offsetStr = getAttr(child, "offset");
            SVGNumber offset = 0.0;
            if (offsetStr) {
                std::string s(offsetStr);
                if (!s.empty() && s.back() == '%') {
                    try {
                        offset = std::stod(s.substr(0, s.size() - 1)) / 100.0;
                    }
                    catch (...) {
                        offset = 0.0;
                    }
                }
                else {
                    offset = parseNumber(offsetStr, 0.0);
                }
            }
            stop.offset = std::clamp(offset, 0.0, 1.0);

            // Parse color (stop-color or style)
            const char* stopColorAttr = getAttr(child, "stop-color");
            stop.stopColor = parseColor(stopColorAttr ? stopColorAttr : "black", {0, 0, 0, 255});

            // Check stop-opacity
            stop.stopOpacity = parseNumber(getAttr(child, "stop-opacity"), 1.0);

            // Handle style="..." for stop-color/opacity
            const char* styleStr = getAttr(child, "style");
            if (styleStr) {
                std::stringstream ss(styleStr);
                std::string item;
                while (std::getline(ss, item, ';')) {
                    size_t colon = item.find(':');
                    if (colon != std::string::npos) {
                        std::string key = trim(item.substr(0, colon));
                        std::string value = trim(item.substr(colon + 1));
                        if (key == "stop-color") {
                            stop.stopColor = parseColor(value, stop.stopColor);
                        }
                        else if (key == "stop-opacity") {
                            stop.stopOpacity = parseNumber(value.c_str(), stop.stopOpacity);
                        }
                    }
                }
            }

            gradient->stops.push_back(stop);

            // DEBUG LOG
            std::cout << "DEBUG: Parsed Stop for Gradient ID=" << gradient->getId()
                      << " Offset=" << stop.offset << " Color=(" << (int)stop.stopColor.r << ","
                      << (int)stop.stopColor.g << "," << (int)stop.stopColor.b << ","
                      << (int)stop.stopColor.a << ")" << std::endl;
        }
    }
}

SVGStyle SVGFactoryImpl::createStyleFromCSS(const std::string& cssContent)
{
    // Reusing parsing logic for key:value; pairs
    SVGStyle style;
    // Default style has "unset" values.

    std::map<std::string, std::string> attrs;

    if (!cssContent.empty()) {
        std::stringstream ss(cssContent);
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

    // Apply parsed attributes to style
    if (attrs.count("fill")) {
        std::string val = attrs["fill"];
        if (val.find("url(") != std::string::npos) {
            style.fillUrl = val;
            style.fillColor = {0, 0, 0, 0, true}; // None
        }
        else {
            style.fillColor = parseColor(val, style.fillColor);
        }
    }

    if (attrs.count("stroke")) {
        std::string val = attrs["stroke"];
        if (val.find("url(") != std::string::npos) {
            style.strokeUrl = val;
            style.strokeColor = {0, 0, 0, 0, true}; // None
        }
        else {
            style.strokeColor = parseColor(val, style.strokeColor);
        }
    }

    if (attrs.count("stroke-width"))
        style.strokeWidth = parseNumber(attrs["stroke-width"].c_str(), style.strokeWidth);
    if (attrs.count("font-size"))
        style.fontSize = parseNumber(attrs["font-size"].c_str(), style.fontSize);
    if (attrs.count("font-family"))
        style.fontFamily = attrs["font-family"];
    if (attrs.count("fill-rule")) {
        if (attrs["fill-rule"] == "evenodd")
            style.fillRule = SVGFillRule::EvenOdd;
        else
            style.fillRule = SVGFillRule::NonZero;
    }
    if (attrs.count("display")) {
        if (attrs["display"] == "none")
            style.isDisplayed = false;
        else
            style.isDisplayed = true;
    }
    if (attrs.count("fill-opacity"))
        style.fillOpacity = parseNumber(attrs["fill-opacity"].c_str(), style.fillOpacity);
    if (attrs.count("stroke-opacity"))
        style.strokeOpacity = parseNumber(attrs["stroke-opacity"].c_str(), style.strokeOpacity);
    if (attrs.count("opacity")) {
        SVGNumber opacity = parseNumber(attrs["opacity"].c_str(), 1.0);
        style.fillOpacity = std::clamp(style.fillOpacity * opacity, 0.0, 1.0);
        style.strokeOpacity = std::clamp(style.strokeOpacity * opacity, 0.0, 1.0);
    }

    if (attrs.count("text-anchor"))
        style.textAnchor = attrs["text-anchor"];

    if (attrs.count("font-style")) {
        std::string fs = attrs["font-style"];
        if (fs.find("italic") != std::string::npos || fs.find("oblique") != std::string::npos)
            style.isItalic = true;
        else if (fs == "normal")
            style.isItalic = false;
    }

    if (attrs.count("font-weight")) {
        std::string fw = attrs["font-weight"];
        if (fw == "bold" || fw == "bolder") {
            style.isBold = true;
            style.fontWeight = 75;
        }
        else if (fw == "normal") {
            style.isBold = false;
            style.fontWeight = 50;
        }
    }

    return style;
}