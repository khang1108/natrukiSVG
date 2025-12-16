#ifndef UI_QT_RENDERER_H
#define UI_QT_RENDERER_H

#include "Renderer.h" // Kế thừa giao diện Strategy/Visitor
#include "svg/SVGStyle.h"
#include "svg/SVGTransform.h"

#include <QPainter> // SỬ DỤNG THƯ VIỆN QT6
#include <QPainterPath>

/**
 * @brief Một triển khai (Implementation) cụ thể của 'Renderer'.
 * Lớp này dùng 'QPainter' của Qt6 để thực hiện việc vẽ.
 *
 *
 * Đây là nơi Role C (UI) sẽ viết phần lớn
 * logic "backend" của mình (trong file .cpp).
 */
class QtRenderer : public Renderer
{
  private:
    /**
     * @brief Con trỏ đến "cây cọ vẽ" của Qt6.
     * 'QtRenderer' không sở hữu 'QPainter' này, nó chỉ "mượn"
     * 'QPainter' từ 'CanvasWidget::paintEvent'.
     */
    QPainter* m_painter;

  public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     * @param painter Con trỏ đến đối tượng QPainter đang hoạt động.
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic cho hàm này trong 'QtRenderer.cpp'.
     * Chủ yếu là để gán 'm_painter = painter;'.
     */
    QtRenderer(QPainter* painter);

    /**
     * @brief Hàm hủy (Destructor).
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic trong 'QtRenderer.cpp'.
     */
    virtual ~QtRenderer();

    // --- IMPLEMENT CÁC HÀM VISIT BẰNG QT6 ---

  /**
   * @brief Các hàm visit cho từng shape.
   *
   * * - Vai trò của Role C (Implement):
   * Phải viết logic .cpp cho TẤT CẢ các hàm này.
   * Ví dụ: Lấy dữ liệu từ 'rect.getStyle()',
   * chuyển 'SVGColor' thành 'QColor', và gọi 'm_painter->drawRect(...)'.
   */
  void visit(SVGRect& rect) override;
  void visit(SVGCircle& circle) override;
  void visit(SVGPolygon& polygon) override;
  void visit(SVGPolyline& polyline) override;
  void visit(SVGText& text) override;
  void visit(SVGEllipse& ellipse) override;
  void visit(SVGLine& line) override;
  void visit(SVGPath& path) override;

    /**
      * @brief Các hàm visit cho Group.
      *
      * * - Vai trò của Role C (Implement):
      * Phải viết logic .cpp cho 2 hàm này.
      * 'visitGroupBegin' sẽ gọi 'm_painter->save()' và áp dụng transform.
      * 'visitGroupEnd' sẽ gọi 'm_painter->restore()'.
      */
    void visitGroupBegin(SVGGroup& group) override;
    void visitGroupEnd(SVGGroup& group) override;

  private:
    bool prepareForDrawing(const SVGStyle& style) const;
    void drawPath(const QPainterPath& path, const SVGStyle& style, const SVGTransform& transform);
};

#endif // UI_QT_RENDERER_H