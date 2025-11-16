#include "rapidxml.hpp" // (Đảm bảo đường dẫn include đã đúng)
#include "SVGDocument.h"

#include "SVGFactory.h"
#include "SVGGroup.h"
#include "SVGElement.h"
#include "SVGStyle.h" // Cần cho style gốc
#include <fstream>
#include <vector>
#include <stdexcept>
#include <sstream>

// --- Hàm khởi tạo (Constructor) ---
SVGDocument::SVGDocument() : m_viewBox{ 0, 0, 0, 0 } {
	m_factory = SVGFactory::createDefaultFactory();
	if (!m_factory) {
		throw std::runtime_error("Không thể tạo SVG Factory");
	}
}

SVGDocument::~SVGDocument() {
	// Thân hàm để trống
}

// --- Các hàm Setters / Getters ---
void SVGDocument::setViewBox(const SVGRectF& viewBox) {
	m_viewBox = viewBox;
}

void SVGDocument::addChild(std::unique_ptr<SVGElement> child) {
	if (child) m_children.push_back(std::move(child));
}

const std::vector<std::unique_ptr<SVGElement>>& SVGDocument::getChildren() const {
	// Hàm này (được 'main.cpp' gọi)
	// chỉ đơn giản là trả về danh sách các con
	return m_children;
}

void SVGDocument::draw(NodeVisitor& visitor) {
	for (const auto& child : m_children) {
		if (child) child->accept(visitor);
	}
}

// --- HÀM PARSING CỐT LÕI (PHIÊN BẢN ĐẦY ĐỦ) ---

bool SVGDocument::load(const std::string& filePath) {
	// 1. ĐỌC FILE VÀO BỘ ĐỆM
	rapidxml::xml_document<> doc;
	std::vector<char> buffer;

	std::ifstream file(filePath);
	if (!file) {
		// (Nên log lỗi ở đây)
		return false;
	}

	buffer.assign((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	buffer.push_back('\0');

	// 2. PARSE XML
	try {
		doc.parse<0>(&buffer[0]);
	}
	catch (rapidxml::parse_error& e) {
		// (Nên log lỗi ở đây: e.what())
		return false;
	}

	// 3. TÌM THẺ <svg> GỐC (ĐỊNH NGHĨA 'rootNode' Ở ĐÂY)
	rapidxml::xml_node<char>* rootNode = doc.first_node("svg");
	if (!rootNode) {
		// (Nên log lỗi: "Không tìm thấy thẻ <svg> gốc")
		return false;
	}

	// 4. PARSE VIEWBOX CỦA THẺ GỐC
	rapidxml::xml_attribute<char>* vbAttr = rootNode->first_attribute("viewBox");
	if (vbAttr) {
		this->m_viewBox = parseViewBox(vbAttr->value());
	}

	// 5. BẮT ĐẦU ĐỆ QUY XÂY DỰNG CÂY
	// (Bỏ 'rootStyle' vì logic đã được chuyển vào Factory)

	// Duyệt các con của thẻ <svg>
	for (rapidxml::xml_node<char>* childNode = rootNode->first_node(); // <--- 'rootNode' đã được định nghĩa
		childNode;
		childNode = childNode->next_sibling())
	{
		// Gọi hàm đệ quy (2 tham số)
		parseRecursive(childNode, nullptr);
	}

	return true;
}

/**
 * @brief Hàm đệ quy chính (2 tham số)
 */
void SVGDocument::parseRecursive(rapidxml::xml_node<char>* xmlNode,
	SVGElement* parentElement)
{
	// 1. TẠO Element C++ từ node XML (Dùng Factory)
	std::unique_ptr<SVGElement> newElement =
		m_factory->createElement(xmlNode, parentElement);

	if (!newElement) {
		return; // Thẻ không được hỗ trợ, bỏ qua
	}

	// 2. XỬ LÝ ĐỆ QUY (Nếu là Group)
	SVGElement* newElementPtr = newElement.get();
	SVGGroup* group = dynamic_cast<SVGGroup*>(newElementPtr);
	if (group) {
		// Nếu là Group, duyệt các con của nó trong XML
		for (rapidxml::xml_node<char>* childNode = xmlNode->first_node();
			childNode;
			childNode = childNode->next_sibling())
		{
			// Gọi đệ quy, 'parentElement' bây giờ là 'group'
			parseRecursive(childNode, group);
		}
	}

	// 3. THÊM VÀO CÂY
	if (parentElement) {
		// Nếu có cha, thêm 'newElement' làm con của cha
		SVGGroup* parentGroup = dynamic_cast<SVGGroup*>(parentElement);
		if (parentGroup) {
			parentGroup->addChild(std::move(newElement));
		}
	}
	else {
		// Nếu không có cha (cấp cao nhất), thêm vào Document
		this->addChild(std::move(newElement));
	}
}

/**
 * @brief Parse thuộc tính viewBox
 */
SVGRectF SVGDocument::parseViewBox(const char* viewBoxStr) const {
	if (!viewBoxStr || *viewBoxStr == '\0') {
		return { 0, 0, 0, 0 };
	}

	std::stringstream ss(viewBoxStr);
	SVGNumber minX, minY, width, height;

	ss >> minX;
	while (ss.peek() == ',' || std::isspace(ss.peek())) { ss.get(); }
	ss >> minY;
	while (ss.peek() == ',' || std::isspace(ss.peek())) { ss.get(); }
	ss >> width;
	while (ss.peek() == ',' || std::isspace(ss.peek())) { ss.get(); }
	ss >> height;

	if (ss.fail()) return { 0, 0, 0, 0 };

	return { minX, minY, width, height };
}