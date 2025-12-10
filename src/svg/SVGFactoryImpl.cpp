#include "SVGFactoryImpl.h"

#include "SVGCircle.h"
#include "SVGEllipse.h"
#include "SVGGroup.h"
#include "SVGLine.h"
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
    return nullptr;
  }

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

  if (newElement) {
    SVGStyle style = parseStyle(node, parentStyle);
    newElement->setStyle(style);

    SVGTransform transform = parseTransform(node, SVGTransform());
    newElement->setTransform(transform);
  }

  return newElement;
}

const char* SVGFactoryImpl::getAttr(rapidxml::xml_node<char>* node, const char* attrName)
{
  if (!node)
    return "";
  rapidxml::xml_attribute<char>* attr = node->first_attribute(attrName);
  return (attr) ? attr->value() : "";
}

std::string SVGFactoryImpl::trim(const std::string& str)
{
  size_t first = str.find_first_not_of(" \t\n\r");
  if (std::string::npos == first)
    return str;
  size_t last = str.find_last_not_of(" \t\n\r");
  return str.substr(first, (last - first + 1));
}

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
    size_t unitPos = std::string::npos;
    unitPos = strValue.rfind("px");
    if (unitPos != std::string::npos && unitPos == strValue.length() - 2) {
      return std::stod(strValue.substr(0, unitPos));
    }

    return std::stod(strValue);
  }
  catch (...) {
    return defaultValue;
  }
}

std::vector<SVGPointF> SVGFactoryImpl::parsePoints(const char* pointsStr)
{
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
    points.push_back({x, y});
  }
  return points;
}

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
  style.inheritFrom(parentStyle);
  std::map<std::string, std::string> attrs;

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

SVGTransform SVGFactoryImpl::parseTransform(rapidxml::xml_node<char>* node,
                                            const SVGTransform& parentTransform)
{
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
    if (startFunc == std::string::npos)
      break;

    size_t startArgs = str.find('(', startFunc);
    if (startArgs == std::string::npos)
      break;

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

  worldTransform.multiply(localTransform);
  return worldTransform;
}