#ifndef SVG_NODE_VISITOR_H
#define SVG_NODE_VISITOR_H

// --- Khai báo trước (Forward Declarations) ---
// Tiết kiệm thời gian biên dịch và tránh #include vòng
class SVGRect;
class SVGCircle;
class SVGPolygon;
class SVGPolyline;
class SVGText;
class SVGGroup;
class SVGEllipse;
class SVGLine;
class SVGPath;
/**
 * @brief Giao diện (Interface) trừu tượng của mẫu Visitor Pattern.
 *
 * Định nghĩa một "hành động" có thể được thực hiện trên cây SVG.
 * Cho phép tách biệt logic (như 'vẽ') ra khỏi dữ liệu (các shape).
 */
class NodeVisitor
{
public:
  virtual ~NodeVisitor() = default;

  /**
   * @brief Các hàm 'visit' cho từng loại shape cụ thể.
   *
   * * - Vai trò của Role C (Implement):
   * Đây là NHIỆM VỤ CỐT LÕI của Role C (Backend).
   * Role C phải tạo một lớp mới (ví dụ: 'QtRenderer') kế thừa
   * từ 'NodeVisitor' và viết code 'QPainter' của Qt6
   * bên trong MỖI HÀM 'visit' này để thực sự VẼ hình lên màn hình.
   *
   * * - Vai trò của Role D (Implement - Test):
   * Có thể tạo một 'visitor' khác (ví dụ: 'TestBBoxVisitor')
   * để kiểm tra logic BBox của Role B.
   */
  virtual void visit(SVGRect& rect) = 0;
  virtual void visit(SVGCircle& circle) = 0;
  virtual void visit(SVGPolygon& polygon) = 0;
  virtual void visit(SVGPolyline& polyline) = 0;
  virtual void visit(SVGText& text) = 0;
  virtual void visit(SVGEllipse& ellipse) = 0;
  virtual void visit(SVGLine& line) = 0;
  virtual void visit(SVGPath& path) = 0;

  /**
   * @brief Được gọi KHI BẮT ĐẦU duyệt một <g> (group).
   *
   * * - Vai trò của Role C (Implement):
   * Role C sẽ implement hàm này để:
   * 1. Gọi QPainter->save() (Lưu trạng thái transform hiện tại).
   * 2. Lấy transform của 'group' và áp dụng nó
   * (ví dụ: QPainter->setTransform(...)).
   */
  virtual void visitGroupBegin(SVGGroup& group) = 0;

  /**
   * @brief Được gọi SAU KHI duyệt xong các con của <g> (group).
   *
   * * - Vai trò của Role C (Implement):
   * Role C sẽ implement hàm này để:
   * 1. Gọi QPainter->restore() (Khôi phục lại transform
   * của cấp cha trước đó).
   */
  virtual void visitGroupEnd(SVGGroup& group) = 0;
};

#endif // SVG_NODE_VISITOR_H