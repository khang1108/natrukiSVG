#ifndef UI_RENDERER_H
#define UI_RENDERER_H

// --- BAO GỒM "CẦU NỐI" VỚI LÕI SVG ---
// Bằng cách kế thừa 'NodeVisitor', lớp Renderer này
// có thể được "truyền" vào hàm 'SVGDocument::draw()'
#include "svg/NodeVisitor.h"

/**
 * @brief Giao diện (Interface) trừu tượng cho Strategy Pattern.
 *
 *
 * Đồng thời cũng là Giao diện Visitor cho phần Lõi SVG.
 *
 *
 * Lớp này định nghĩa một "Renderer" (Trình kết xuất) có thể
 * "visit" (thăm) các nút SVG và thực hiện một hành động
 * (ví dụ: vẽ chúng lên màn hình).
 *
 * * - Vai trò của Role C (Implement):
 * Sẽ tạo ra một lớp cụ thể (là 'QtRenderer')
 * kế thừa từ 'Renderer' và implement TẤT CẢ các hàm
 * 'visit' ảo thuần túy bằng code Qt6.
 */
class Renderer : public NodeVisitor
{
public:
  virtual ~Renderer() = default;

  // --- CÁC HÀM TỪ NODEVISITOR MÀ ROLE C PHẢI IMPLEMENT ---
  // (Đã được định nghĩa trong NodeVisitor.h)
  //
  // virtual void visit(SVGRect& rect) = 0;
  // virtual void visit(SVGCircle& circle) = 0;
  // virtual void visit(SVGPolygon& polygon) = 0;
  // virtual void visit(SVGPolyline& polyline) = 0;
  // virtual void visit(SVGText& text) = 0;
  // virtual void visit(SVGEllipse& ellipse) = 0;
  // virtual void visit(SVGLine& line) = 0;
  //
  // virtual void visitGroupBegin(SVGGroup& group) = 0;
  // virtual void visitGroupEnd(SVGGroup& group) = 0;
};

#endif // UI_RENDERER_H