#include "SVGFactoryImpl.h" // Include header MỚI của chính nó

// Bao gồm TẤT CẢ các "sản phẩm" mà nhà máy này tạo ra
#include "SVGGroup.h"
#include "SVGRect.h"
#include "SVGCircle.h"
#include "SVGEllipse.h"
#include "SVGLine.h"
#include "SVGPolygon.h"
#include "SVGPolyline.h"
#include "SVGText.h"

// Bao gồm các thư viện C++ chuẩn
#include <sstream>      
#include <stdexcept>    
#include <algorithm>    
#include <cstring>      
#include <cctype>       

// --- HÀM FACTORY CỐT LÕI (Implement "hợp đồng") ---
std::unique_ptr<SVGElement> SVGFactoryImpl::createElement(
    rapidxml::xml_node<char>* node,
    SVGElement* parentElement
)
{
    const char* name = node->name();
    if (!name || *name == '\0') {
        return nullptr;
    }

    // **BƯỚC 1: LẤY STYLE VÀ TRANSFORM CỦA CHA**
    const SVGStyle& parentStyle = (parentElement) ?
        parentElement->getStyle() :
        SVGStyle(); // Lấy style mặc định

    const SVGTransform& parentTransform = (parentElement) ?
        parentElement->getTransform() :
        SVGTransform(); // Lấy transform mặc định

    std::unique_ptr<SVGElement> newElement = nullptr;

    // --- Logic Factory (Tạo đối tượng) ---
    if (strcmp(name, "rect") == 0) {
        SVGRectF rect = {
            parseNumber(getAttr(node, "x"), 0.0),
            parseNumber(getAttr(node, "y"), 0.0),
            parseNumber(getAttr(node, "width"), 0.0),
            parseNumber(getAttr(node, "height"), 0.0)
        };
        SVGNumber rx = parseNumber(getAttr(node, "rx"), 0.0);
        SVGNumber ry = parseNumber(getAttr(node, "ry"), 0.0);
        newElement = std::make_unique<SVGRect>(rect, rx, ry);
    }
    else if (strcmp(name, "circle") == 0) {
        SVGPointF center = {
            parseNumber(getAttr(node, "cx"), 0.0),
            parseNumber(getAttr(node, "cy"), 0.0)
        };
        SVGNumber r = parseNumber(getAttr(node, "r"), 0.0);
        newElement = std::make_unique<SVGCircle>(center, r);
    }
    else if (strcmp(name, "ellipse") == 0) {
        SVGPointF center = {
            parseNumber(getAttr(node, "cx"), 0.0),
            parseNumber(getAttr(node, "cy"), 0.0)
        };
        SVGNumber rx = parseNumber(getAttr(node, "rx"), 0.0);
        SVGNumber ry = parseNumber(getAttr(node, "ry"), 0.0);
        newElement = std::make_unique<SVGEllipse>(center, rx, ry);
    }
    else if (strcmp(name, "line") == 0) {
        SVGPointF p1 = {
            parseNumber(getAttr(node, "x1"), 0.0),
            parseNumber(getAttr(node, "y1"), 0.0)
        };
        SVGPointF p2 = {
            parseNumber(getAttr(node, "x2"), 0.0),
            parseNumber(getAttr(node, "y2"), 0.0)
        };
        newElement = std::make_unique<SVGLine>(p1, p2);
    }
    else if (strcmp(name, "polygon") == 0) {
        newElement = std::make_unique<SVGPolygon>(parsePoints(getAttr(node, "points")));
    }
    else if (strcmp(name, "polyline") == 0) {
        newElement = std::make_unique<SVGPolyline>(parsePoints(getAttr(node, "points")));
    }
    else if (strcmp(name, "g") == 0) {
        newElement = std::make_unique<SVGGroup>();
    }
    else if (strcmp(name, "text") == 0) {
        SVGPointF pos = {
            parseNumber(getAttr(node, "x"), 0.0),
            parseNumber(getAttr(node, "y"), 0.0)
        };
        newElement = std::make_unique<SVGText>(pos, node->value());
    }

    // --- Gán Style & Transform (SAU KHI TẠO) ---
    if (newElement) {
        SVGStyle style = parseStyle(node, parentStyle);
        newElement->setStyle(style);

        SVGTransform transform = parseTransform(node, parentTransform);
        newElement->setTransform(transform);
    }

    return newElement;
}


// --- CÁC HÀM TIỆN ÍCH (HELPERS) ĐỂ PARSE ---
// (Đây là định nghĩa của các hàm private trong .h)

const char* SVGFactoryImpl::getAttr(rapidxml::xml_node<char>* node, const char* attrName) {
    if (!node) return "";
    rapidxml::xml_attribute<char>* attr = node->first_attribute(attrName);
    return (attr) ? attr->value() : "";
}

std::string SVGFactoryImpl::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

SVGNumber SVGFactoryImpl::parseNumber(const char* value, SVGNumber defaultValue) {
    if (!value || *value == '\0') {
        return defaultValue;
    }
    std::string strValue = trim(std::string(value));
    if (strValue.empty()) {
        return defaultValue;
    }
    try {
        size_t unitPos = std::string::npos;
        unitPos = strValue.rfind("px");
        if (unitPos != std::string::npos && unitPos == strValue.length() - 2) {
            return std::stod(strValue.substr(0, unitPos));
        }
        // (Bỏ qua logic 'em', '%' như trong code cũ)
        return std::stod(strValue);
    }
    catch (...) {
        return defaultValue;
    }
}

std::vector<SVGPointF> SVGFactoryImpl::parsePoints(const char* pointsStr) {
    std::vector<SVGPointF> points;
    if (!pointsStr || *pointsStr == '\0') {
        return points;
    }
    std::stringstream ss(pointsStr);
    SVGNumber x, y;
    while (ss >> x) {
        while (ss.peek() == ',' || std::isspace(ss.peek())) {
            ss.get();
        }
        if (!(ss >> y)) {
            break;
        }
        points.push_back({ x, y });
    }
    return points;
}

SVGColor SVGFactoryImpl::parseColor(std::string colorStr, const SVGColor& defaultValue) {
    if (colorStr.empty()) {
        return defaultValue;
    }
    std::transform(colorStr.begin(), colorStr.end(), colorStr.begin(), ::tolower);
    if (colorStr == "none") {
        SVGColor color;
        color.isNone = true;
        color.a = 0;
        return color;
    }
    if (colorStr == "black") return { 0, 0, 0, 255, false };
    if (colorStr == "red") return { 255, 0, 0, 255, false };
    if (colorStr == "green") return { 0, 128, 0, 255, false };
    if (colorStr == "blue") return { 0, 0, 255, 255, false };
    if (colorStr == "white") return { 255, 255, 255, 255, false };
    //Thêm màu khác...

    if (colorStr.rfind("rgb(", 0) == 0) {
        SVGColor color{ 0, 0, 0, 255, false };
        try {
            std::string args = colorStr.substr(4, colorStr.length() - 5);
            std::stringstream ss(args);
            int r, g, b;
            char comma;
            ss >> r >> comma >> g >> comma >> b;
            if (ss.fail()) {
                return defaultValue;
            }
            color.r = (unsigned char)std::max(0, std::min(r, 255));
            color.g = (unsigned char)std::max(0, std::min(g, 255));
            color.b = (unsigned char)std::max(0, std::min(b, 255));
            return color;
        }
        catch (...) {
            return defaultValue;
        }
    }

    if (colorStr[0] == '#') {
        SVGColor color{ 0, 0, 0, 255, false };
        std::string hex = colorStr.substr(1);
        try {
            if (hex.length() == 6) {
                color.r = (unsigned char)std::stoul(hex.substr(0, 2), nullptr, 16);
                color.g = (unsigned char)std::stoul(hex.substr(2, 2), nullptr, 16);
                color.b = (unsigned char)std::stoul(hex.substr(4, 2), nullptr, 16);
            }
            else if (hex.length() == 3) {
                color.r = (unsigned char)std::stoul(hex.substr(0, 1) + hex.substr(0, 1), nullptr, 16);
                color.g = (unsigned char)std::stoul(hex.substr(1, 1) + hex.substr(1, 1), nullptr, 16);
                color.b = (unsigned char)std::stoul(hex.substr(2, 2) + hex.substr(2, 2), nullptr, 16);
            }
            return color;
        }
        catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

SVGStyle SVGFactoryImpl::parseStyle(rapidxml::xml_node<char>* node, const SVGStyle& parentStyle) {
    SVGStyle style = parentStyle;
    std::map<std::string, std::string> attrs;

    if (auto* attr = node->first_attribute("fill")) attrs["fill"] = attr->value();
    if (auto* attr = node->first_attribute("stroke")) attrs["stroke"] = attr->value();
    if (auto* attr = node->first_attribute("stroke-width")) attrs["stroke-width"] = attr->value();
    if (auto* attr = node->first_attribute("font-size")) attrs["font-size"] = attr->value();
    if (auto* attr = node->first_attribute("fill-rule")) attrs["fill-rule"] = attr->value();
    if (auto* attr = node->first_attribute("display")) attrs["display"] = attr->value();

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

    if (attrs.count("fill")) style.fillColor = parseColor(attrs["fill"], style.fillColor);
    if (attrs.count("stroke")) style.strokeColor = parseColor(attrs["stroke"], style.strokeColor);
    if (attrs.count("stroke-width")) style.strokeWidth = parseNumber(attrs["stroke-width"].c_str(), style.strokeWidth);
    if (attrs.count("font-size")) style.fontSize = parseNumber(attrs["font-size"].c_str(), style.fontSize);

    if (attrs.count("fill-rule")) {
        if (attrs["fill-rule"] == "evenodd") {
            style.fillRule = SVGFillRule::EvenOdd;
        }
        else {
            style.fillRule = SVGFillRule::NonZero;
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

    return style;
}

SVGTransform SVGFactoryImpl::parseTransform(rapidxml::xml_node<char>* node, const SVGTransform& parentTransform) {
    SVGTransform worldTransform = parentTransform;
    const char* transformStr = getAttr(node, "transform");
    if (!transformStr || *transformStr == '\0') {
        return worldTransform;
    }

    SVGTransform localTransform;
    std::string str(transformStr);
    size_t pos = 0;
    while (pos < str.length()) {
        size_t startFunc = str.find_first_not_of(" \t\r\n", pos);
        if (startFunc == std::string::npos) break;

        size_t startArgs = str.find('(', startFunc);
        if (startArgs == std::string::npos) break;

        size_t endArgs = str.find(')', startArgs);
        if (endArgs == std::string::npos) break;

        std::string funcName = str.substr(startFunc, startArgs - startFunc);
        std::string argsStr = str.substr(startArgs + 1, endArgs - (startArgs + 1));

        std::stringstream ss(argsStr);
        char comma;
        SVGNumber n1, n2, n3;

        std::replace(argsStr.begin(), argsStr.end(), ',', ' ');
        ss.str(argsStr);

        if (funcName == "translate") {
            ss >> n1; ss >> n2;
            if (ss.fail()) n2 = 0;
            SVGTransform tempT;
            tempT.translate(n1, n2);
            localTransform.multiply(tempT);
        }
        else if (funcName == "scale") {
            ss >> n1; ss >> n2;
            if (ss.fail()) n2 = n1;
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

    worldTransform.multiply(localTransform);
    return worldTransform;
}