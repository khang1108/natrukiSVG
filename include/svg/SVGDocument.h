#ifndef SVG_DOCUMENT_H
#define SVG_DOCUMENT_H

#include "Types.h"

#include <memory>
#include <string>
#include <vector>

// --- Khai báo trước (Forward Declaration) ---
class SVGElement;
class NodeVisitor;
class SVGFactory;
// (Không cần SVGStyle ở đây nữa)

namespace rapidxml
{
  template<class Ch> class xml_node;
}

/**
 * @class SVGDocument
 */
class SVGDocument
{

private:
  SVGRectF m_viewBox;
  std::vector<std::unique_ptr<SVGElement>> m_children;
  std::unique_ptr<SVGFactory> m_factory;

  /**
   * @brief Hàm đệ quy chính
   * (Chỉ cần 2 tham số)
   */
  void parseRecursive(rapidxml::xml_node<char>* xmlNode, SVGElement* parentElement);

  /**
   * @brief Parse thuộc tính viewBox từ chuỗi "minX minY w h".
   */
  SVGRectF parseViewBox(const char* viewBoxStr) const;
  SVGRectF getContentBoundingBox() const;

public:
  SVGDocument();
  ~SVGDocument();
  void setViewBox(const SVGRectF& viewBox);
  void addChild(std::unique_ptr<SVGElement> child);
  void draw(NodeVisitor& visitor);
  const SVGRectF& getViewBox() const { return m_viewBox; }
  const std::vector<std::unique_ptr<SVGElement>>& getChildren() const;

  /**
   * @brief Tải, parse file SVG.
   */
  bool load(const std::string& filePath);
};

#endif // SVG_DOCUMENT_H
