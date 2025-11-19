#ifndef SVG_STYLE_H
#define SVG_STYLE_H

#include "Types.h" // Lấy SVGColor, SVGNumber, SVGFillRule

#include <string>

/**
 * @brief Lưu trữ tất cả các thuộc tính style có thể được kế thừa.
 *
 * Đây là một cấu trúc dữ liệu thuần túy (data class).
 */
class SVGStyle
{
public:
  // --- Thuộc tính Fill & Stroke ---
  SVGColor fillColor;
  SVGColor strokeColor;
  SVGNumber strokeWidth = 1.0;
  SVGNumber fillOpacity = 1.0;
  SVGNumber strokeOpacity = 1.0;
  SVGFillRule fillRule = SVGFillRule::NonZero;

  // --- Thuộc tính Font ---
  std::string fontFamily;
  SVGNumber fontSize = 16.0; // Kích thước font chữ mặc định

  /**
   * @brief Lưu thuộc tính 'display'.
   * Mặc định là 'true' (hiển thị).
   * Nếu 'display="none"', nó sẽ là 'false'.
   */
  bool isDisplayed = true;

public:
  /**
   * @brief Hàm khởi tạo (Constructor).
   * Sẽ thiết lập các giá trị mặc định ban đầu
   * (ví dụ: fillColor là black, strokeColor là 'none', v.v.).
   *
   * * - Vai trò của Role B (Implement):
   * Phải viết logic cho hàm này trong file 'SVGStyle.cpp'.
   */
  SVGStyle();

  /**
   * @brief Implement logic "Cascade Style" (Kế thừa Style).
   *
   * Hàm này "trộn" style của cha vào style của con,
   * nhưng chỉ trộn những thuộc tính mà con chưa tự định nghĩa.
   *
   * @param parent Đối tượng style của phần tử cha (<g> hoặc <svg>).
   *
   * * - Vai trò của Role B (Implement):
   * Đây là một hàm logic CỰC KỲ QUAN TRỌNG. Role B phải viết
   * logic trong 'SVGStyle.cpp' để kiểm tra từng thuộc tính
   * (ví dụ: 'this->fillColor' đã được set chưa? Nếu chưa,
   * hãy lấy từ 'parent.fillColor').
   * còn nếu đã được set thì giữ nguyên giá trị hiện tại.
   * Tương tự cho các thuộc tính khác.
   */
  void inheritFrom(const SVGStyle& parent);

  /**
   * @brief Đảm bảo các thuộc tính chưa được thiết lập nhận giá trị mặc định.
   */
  void applyDefaults();
};

#endif // SVG_STYLE_H