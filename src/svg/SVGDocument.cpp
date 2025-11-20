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

/**
 * @brief Constructor for SVG document - initializes the document structure.
 *
 * Algorithm:
 * - Initializes viewBox to empty (0, 0, 0, 0)
 * - Creates a default factory for parsing SVG elements
 * - The factory is responsible for creating element objects from XML nodes
 *
 * @throws std::runtime_error if factory creation fails
 */
SVGDocument::SVGDocument() : m_viewBox{0, 0, 0, 0}
{
    m_factory = SVGFactory::createDefaultFactory();
    if (!m_factory) {
        throw std::runtime_error("Không thể tạo SVG Factory");
    }
}

SVGDocument::~SVGDocument() {}

/**
 * @brief Sets the viewBox of the SVG document.
 *
 * Algorithm:
 * - viewBox defines the coordinate system and visible area of the SVG
 * - Format: "minX minY width height"
 * - Used to determine how SVG coordinates map to the viewport
 *
 * @param viewBox The viewBox rectangle: {minX, minY, width, height}
 */
void SVGDocument::setViewBox(const SVGRectF& viewBox) { m_viewBox = viewBox; }

/**
 * @brief Adds a top-level child element to the document.
 *
 * Algorithm:
 * - Takes ownership of the child element using std::move
 * - Adds it to the document's children vector
 * - Only adds if the child pointer is valid (not null)
 *
 * @param child Unique pointer to the child element (ownership is transferred)
 */
void SVGDocument::addChild(std::unique_ptr<SVGElement> child)
{
    if (child)
        m_children.push_back(std::move(child));
}

/**
 * @brief Gets a const reference to all top-level children.
 *
 * @return Const reference to the vector of child elements
 */
const std::vector<std::unique_ptr<SVGElement>>& SVGDocument::getChildren() const
{

    return m_children;
}

/**
 * @brief Draws the document using the Visitor pattern.
 *
 * Algorithm (Visitor Pattern):
 * - Iterates through all top-level children
 * - For each child, calls child->accept(visitor)
 * - This triggers the visitor to process the element and its descendants
 * - The visitor (e.g., QtRenderer) handles the actual rendering logic
 *
 * This separates the document structure from rendering operations.
 *
 * @param visitor The visitor object that will process all elements
 */
void SVGDocument::draw(NodeVisitor& visitor)
{
    for (const auto& child : m_children) {
        if (child)
            child->accept(visitor);
    }
}

/**
 * @brief Loads and parses an SVG file from disk.
 *
 * Algorithm:
 * 1. Clear existing children to prepare for new document
 * 2. Open the file and read its entire contents into a buffer
 * 3. Parse the XML using RapidXML parser
 * 4. Find the root <svg> element
 * 5. Parse the viewBox attribute if present
 * 6. Recursively parse all child elements of the root
 * 7. Build the SVG element tree structure
 *
 * Error handling:
 * - Returns false if file cannot be opened
 * - Returns false if XML parsing fails
 * - Returns false if root <svg> element is not found
 *
 * @param filePath Path to the SVG file to load
 * @return true if file was successfully loaded and parsed, false otherwise
 */
bool SVGDocument::load(const std::string& filePath)
{

    rapidxml::xml_document<> doc;
    std::vector<char> buffer;

    m_children.clear(); // Clear existing content

    // Open and read file
    std::ifstream file(filePath);
    if (!file) {

        return false; // File not found or cannot be opened
    }

    // Read entire file into buffer
    buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    buffer.push_back('\0'); // Null-terminate for RapidXML

    // Parse XML
    try {
        doc.parse<0>(&buffer[0]);
    }
    catch (rapidxml::parse_error& e) {

        return false; // XML parsing failed
    }

    // Find root <svg> element
    rapidxml::xml_node<char>* rootNode = doc.first_node("svg");
    if (!rootNode) {

        return false; // No <svg> root element found
    }

    // Parse viewBox attribute
    rapidxml::xml_attribute<char>* vbAttr = rootNode->first_attribute("viewBox");
    if (vbAttr) {
        this->m_viewBox = parseViewBox(vbAttr->value());
    }

    // Recursively parse all child elements
    for (rapidxml::xml_node<char>* childNode = rootNode->first_node(); childNode;
         childNode = childNode->next_sibling()) {

        parseRecursive(childNode, nullptr); // nullptr = top-level, no parent
    }

    return true;
}

/**
 * @brief Recursively parses XML nodes and builds the SVG element tree.
 *
 * Algorithm (Recursive Tree Building):
 * 1. Use factory to create an SVGElement from the XML node
 *    - Factory parses attributes, style, and transform
 *    - Factory handles style inheritance from parent
 *    - Factory handles transform accumulation from parent
 * 2. If element creation fails, return (skip this node)
 * 3. If the element is a group (<g>):
 *    - Recursively parse all child XML nodes
 *    - Pass the group as parent to children
 * 4. Add the element to its parent:
 *    - If parent exists and is a group: add to group's children
 *    - If no parent (top-level): add to document's children
 *
 * This builds a tree structure matching the XML hierarchy:
 * - Document
 *   - Group (with style/transform)
 *     - Rect (inherits group's style/transform)
 *     - Circle (inherits group's style/transform)
 *   - Path (top-level)
 *
 * Style and transform inheritance:
 * - Children inherit parent's style (if not overridden)
 * - Children accumulate parent's transform (parent transform applied first)
 *
 * @param xmlNode The XML node to parse
 * @param parentElement The parent SVG element (nullptr for top-level elements)
 */
void SVGDocument::parseRecursive(rapidxml::xml_node<char>* xmlNode, SVGElement* parentElement)
{

    // Create element from XML node (factory handles parsing)
    std::unique_ptr<SVGElement> newElement = m_factory->createElement(xmlNode, parentElement);

    if (!newElement) {
        return; // Failed to create element (unknown tag, etc.)
    }

    SVGElement* newElementPtr = newElement.get();
    SVGGroup* group = dynamic_cast<SVGGroup*>(newElementPtr);
    if (group) {
        // If this is a group, recursively parse its children
        for (rapidxml::xml_node<char>* childNode = xmlNode->first_node(); childNode;
             childNode = childNode->next_sibling()) {

            parseRecursive(childNode, group); // Pass group as parent
        }
    }

    // Add element to its parent
    if (parentElement) {
        // Has a parent - add to parent group
        SVGGroup* parentGroup = dynamic_cast<SVGGroup*>(parentElement);
        if (parentGroup) {
            parentGroup->addChild(std::move(newElement));
        }
    }
    else {
        // Top-level element - add to document
        this->addChild(std::move(newElement));
    }
}

/**
 * @brief Parses the viewBox attribute string into a rectangle.
 *
 * Algorithm:
 * 1. Check if string is valid (not null or empty)
 * 2. Use stringstream to parse four numbers: minX, minY, width, height
 * 3. Skip whitespace and commas between numbers (SVG allows both)
 * 4. Extract each number sequentially
 * 5. Return rectangle if parsing succeeds, empty rectangle if it fails
 *
 * ViewBox format: "minX minY width height"
 * - minX, minY: Top-left corner of the viewBox
 * - width, height: Dimensions of the viewBox
 *
 * Examples:
 * - "0 0 100 200" -> {0, 0, 100, 200}
 * - "0,0,100,200" -> {0, 0, 100, 200} (commas also allowed)
 * - "10 20 50 60" -> {10, 20, 50, 60}
 *
 * @param viewBoxStr The viewBox attribute string to parse
 * @return SVGRectF representing the viewBox, or {0,0,0,0} if parsing fails
 */
SVGRectF SVGDocument::parseViewBox(const char* viewBoxStr) const
{
    if (!viewBoxStr || *viewBoxStr == '\0') {
        return {0, 0, 0, 0}; // Invalid input
    }

    std::stringstream ss(viewBoxStr);
    SVGNumber minX, minY, width, height;

    // Parse minX
    ss >> minX;
    // Skip separators (whitespace or commas)
    while (ss.peek() == ',' || std::isspace(ss.peek())) {
        ss.get();
    }
    // Parse minY
    ss >> minY;
    while (ss.peek() == ',' || std::isspace(ss.peek())) {
        ss.get();
    }
    // Parse width
    ss >> width;
    while (ss.peek() == ',' || std::isspace(ss.peek())) {
        ss.get();
    }
    // Parse height
    ss >> height;

    if (ss.fail())
        return {0, 0, 0, 0}; // Parsing failed

    return {minX, minY, width, height};
}