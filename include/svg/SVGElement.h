#ifndef SVG_ELEMENT_H
#define SVG_ELEMENT_H

#include "NodeVisitor.h"  // Vì mỗi element phải chấp nhận (accept) visitor
#include "SVGStyle.h"     // Vì mỗi element có style riêng
#include "SVGTransform.h" // Vì mỗi element có transform riêng

#include <memory>
#include <string>

/**
 * @brief Lớp cha trừu tượng (abstract base class) cho TẤT CẢ các
 * đối tượng trong cây SVG (như <rect>, <circle>, <g>).
 */
class SVGElement
{
protected:
  std::string m_id;
  SVGStyle m_style;
  SVGTransform m_transform;

public:
  virtual ~SVGElement() = default;

  /**
   * @brief Hàm "chấp nhận" của Visitor Pattern (Double-Dispatch).
   * Đây là một hàm ảo thuần túy (pure virtual),
   * BẮT BUỘC tất cả các lớp con (SVGRect, SVGCircle...)
   * phải implement hàm này.
   *
   * * - Vai trò của Role A (Implement):
   * Cao sẽ implement hàm này trong các file .h của shape con
   * (ví dụ: SVGRect.h) với code 1 dòng:
   * 'void accept(NodeVisitor& v) override { v.visit(*this); }'
   */
  virtual void accept(NodeVisitor& visitor) = 0;

  // --- CÁC HÀM BOUNDING BOX ---

  /**
   * @brief Tính BBox "cục bộ" (trước khi transform).
   * BẮT BUỘC tất cả các lớp con phải implement hàm này.
   *
   * * - Vai trò của Role B (Implement):
   * Phải viết logic .cpp cho hàm này ở TẤT CẢ các shape con.
   */
  virtual SVGRectF localBox() const = 0;

  /**
   * @brief Tính BBox "thế giới" (sau khi transform).
   * Hàm này có thể implement chung cho tất cả các shape.
   *
   * * - Vai trò của Role B (Implement):
   * Thường là gọi localBox() và áp dụng m_transform.
   */
  virtual SVGRectF worldBox() const;

  // --- Các hàm Setters / Getters ---

  /**
   * @brief Các hàm (setter/getter) đơn giản cho Style và Transform.
   *
   * * - Vai trò của Role B (Implement):
   * Phải viết logic (dù là đơn giản) cho các hàm này
   * trong file 'SVGElement.cpp' (trừ khi chúng là inline).
   *
   * * - Vai trò của Role C (Sử dụng):
   * Sẽ gọi 'getStyle()' và 'getTransform()' bên trong các hàm
   * 'visit(...)' của họ để lấy dữ liệu vẽ.
   */
  void setStyle(const SVGStyle& style);
  void setTransform(const SVGTransform& transform);
  SVGStyle& getStyle();
  const SVGStyle& getStyle() const; // Phiên bản const
  SVGTransform& getTransform();
  const SVGTransform& getTransform() const; // Phiên bản const

  // <-------------------Getter / Setter------------------->
  std::string getId() const;

  void setId(const std::string& id);
};

#endif // SVG_ELEMENT_H
