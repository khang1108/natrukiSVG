#include "SVGDocument.h"

#include "SVGElement.h"
#include "SVGFactory.h"
#include "SVGGroup.h"
#include "SVGStyle.h"
#include "rapidxml.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

SVGDocument::SVGDocument() : m_viewBox{0, 0, 0, 0}
{
  m_factory = SVGFactory::createDefaultFactory();
  if (!m_factory) {
    throw std::runtime_error("Không thể tạo SVG Factory");
  }
}

SVGDocument::~SVGDocument() {}

void SVGDocument::setViewBox(const SVGRectF& viewBox) { m_viewBox = viewBox; }

void SVGDocument::addChild(std::unique_ptr<SVGElement> child)
{
  if (child)
    m_children.push_back(std::move(child));
}

const std::vector<std::unique_ptr<SVGElement>>& SVGDocument::getChildren() const
{

  return m_children;
}

void SVGDocument::draw(NodeVisitor& visitor)
{
  for (const auto& child : m_children) {
    if (child)
      child->accept(visitor);
  }
}

bool SVGDocument::load(const std::string& filePath)
{

  rapidxml::xml_document<> doc;
  std::vector<char> buffer;

  m_children.clear();

  std::ifstream file(filePath);
  if (!file) {

    return false;
  }

  buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  buffer.push_back('\0');

  try {
    doc.parse<0>(&buffer[0]);
  }
  catch (rapidxml::parse_error& e) {

    return false;
  }

  rapidxml::xml_node<char>* rootNode = doc.first_node("svg");
  if (!rootNode) {

    return false;
  }

  rapidxml::xml_attribute<char>* vbAttr = rootNode->first_attribute("viewBox");
  if (vbAttr) {
    this->m_viewBox = parseViewBox(vbAttr->value());
  }

  for (rapidxml::xml_node<char>* childNode = rootNode->first_node(); childNode;
       childNode = childNode->next_sibling()) {

    parseRecursive(childNode, nullptr);
  }

  return true;
}

/**
 * @brief Hàm đệ quy chính (2 tham số)
 */
void SVGDocument::parseRecursive(rapidxml::xml_node<char>* xmlNode, SVGElement* parentElement)
{

  std::unique_ptr<SVGElement> newElement = m_factory->createElement(xmlNode, parentElement);

  if (!newElement) {
    return;
  }

  SVGElement* newElementPtr = newElement.get();
  SVGGroup* group = dynamic_cast<SVGGroup*>(newElementPtr);
  if (group) {

    for (rapidxml::xml_node<char>* childNode = xmlNode->first_node(); childNode;
         childNode = childNode->next_sibling()) {

      parseRecursive(childNode, group);
    }
  }

  if (parentElement) {

    SVGGroup* parentGroup = dynamic_cast<SVGGroup*>(parentElement);
    if (parentGroup) {
      parentGroup->addChild(std::move(newElement));
    }
  }
  else {

    this->addChild(std::move(newElement));
  }
}

/**
 * @brief Parse thuộc tính viewBox
 */
SVGRectF SVGDocument::parseViewBox(const char* viewBoxStr) const
{
  if (!viewBoxStr || *viewBoxStr == '\0') {
    return {0, 0, 0, 0};
  }

  std::stringstream ss(viewBoxStr);
  SVGNumber minX, minY, width, height;

  ss >> minX;
  while (ss.peek() == ',' || std::isspace(ss.peek())) {
    ss.get();
  }
  ss >> minY;
  while (ss.peek() == ',' || std::isspace(ss.peek())) {
    ss.get();
  }
  ss >> width;
  while (ss.peek() == ',' || std::isspace(ss.peek())) {
    ss.get();
  }
  ss >> height;

  if (ss.fail())
    return {0, 0, 0, 0};

  return {minX, minY, width, height};
}