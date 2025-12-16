#include "SVGDocument.h"

#include "SVGElement.h"
#include "SVGFactory.h"
#include "SVGGroup.h"
#include "SVGStyle.h"
#include "rapidxml.hpp"

#include <fstream>
#include <limits>
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
    if (!m_children.empty()) {
        SVGRectF contentBox = getContentBoundingBox();
        // Chỉ cập nhật nếu tìm được vùng bao hợp lệ (width, height > 0)
        if (contentBox.width > 0 && contentBox.height > 0) {
            m_viewBox = contentBox;
        }
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

// --- HÀM MỚI: Tính Bounding Box của toàn bộ tài liệu ---
SVGRectF SVGDocument::getContentBoundingBox() const
{
    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();
    bool hasContent = false;

    for (const auto& child : m_children) {
        if (!child)
            continue;

        // Lấy bounding box của element (đã tính cả transform)
        SVGRectF bbox = child->worldBox();

        // Bỏ qua các phần tử không có kích thước (như group rỗng hoặc định nghĩa)
        if (bbox.width <= 0 || bbox.height <= 0)
            continue;

        if (bbox.x < minX)
            minX = bbox.x;
        if (bbox.y < minY)
            minY = bbox.y;
        if (bbox.x + bbox.width > maxX)
            maxX = bbox.x + bbox.width;
        if (bbox.y + bbox.height > maxY)
            maxY = bbox.y + bbox.height;

        hasContent = true;
    }

    if (!hasContent) {
        return {0, 0, 0, 0};
    }

    // Thêm một chút padding (lề) khoảng 5% để hình không sát mép màn hình
    SVGNumber paddingX = (maxX - minX) * 0.05;
    SVGNumber paddingY = (maxY - minY) * 0.05;

    return {minX - paddingX, minY - paddingY, (maxX - minX) + 2 * paddingX,
            (maxY - minY) + 2 * paddingY};
}