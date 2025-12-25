#ifndef SVG_TRANSFORM_H
#define SVG_TRANSFORM_H

#include "Types.h" // Lấy SVGNumber, SVGPointF

/**
 * @brief Quản lý một ma trận biến đổi 2D (3x3).
 * Dùng để tính toán và gộp các phép 'translate', 'scale', 'rotate'.
 *
 */
class SVGTransform
{
  private:
    /**
     * @brief Nơi lưu trữ 9 giá trị của ma trận 3x3.
     */
    SVGNumber m_matrix[3][3];

  public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     *
     * * - Vai trò của Role B (Implement):
     * Phải viết logic trong 'SVGTransform.cpp' để khởi tạo
     * 'm_matrix' thành một "Ma trận đơn vị" (Identity matrix).
     */
    SVGTransform();

    /**
     * @brief Nhân ma trận hiện tại với một ma trận tịnh tiến (translate).
     *
     * * - Vai trò của Role B (Implement):
     * Phải viết logic toán học để tạo ma trận translate
     * và nhân nó (multiply) với 'm_matrix' hiện tại.
     */
    void translate(SVGNumber tx, SVGNumber ty);

    /**
     * @brief Nhân ma trận hiện tại với một ma trận co giãn (scale).
     * (Dùng cho cả 'flip' khi sx/sy là -1).
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic tạo ma trận scale và nhân nó (multiply)
     * với 'm_matrix' hiện tại.
     */
    void scale(SVGNumber sx, SVGNumber sy);

    /**
     * @brief Nhân ma trận hiện tại với một ma trận xoay (rotate).
     *
     * * - Vai trò của Role B (Implement):
     * Viết logic toán học (dùng sin, cos) để tạo
     * ma trận rotate và nhân nó (multiply) với 'm_matrix' hiện tại.
     */
    void rotate(SVGNumber angle);

    /**
     * @brief Nhân ma trận hiện tại với một ma trận khác.
     * Dùng để tính toán 'WorldTransform = parent * local'.
     *
     *
     * * - Vai trò của Role B (Implement):
     * Phải viết logic "nhân ma trận" (A * B) trong 'SVGTransform.cpp'.
     */
    void multiply(const SVGTransform& other);

    /**
     * @brief Áp dụng ma trận (đã gộp) lên một điểm (x, y).
     *
     * @param point Điểm (Point) đầu vào.
     * @return Điểm (Point) mới sau khi đã biến đổi.
     *
     * * - Vai trò của Role B (Implement):
     * Phải viết logic toán học "nhân ma trận với vector"
     * (m_matrix * point) để trả về tọa độ mới.
     * * - Vai trò của Role C (Sử dụng):
     * Dùng hàm này để tính toán tọa độ cuối cùng khi vẽ.
     */
    SVGPointF map(const SVGPointF& point) const;

    /**
     * @brief Get the transformation matrix.
     *
     * @return const SVGNumber(&)[3][3] Reference to the 3x3 matrix array
     */
    const SVGNumber (&getMatrix() const)[3][3];

    /**
     * @brief Set matrix directly with 6 values (a, b, c, d, e, f).
     * Matrix format: | a  c  e |
     *                | b  d  f |
     *                | 0  0  1 |
     */
    void setMatrix(SVGNumber a, SVGNumber b, SVGNumber c, SVGNumber d, SVGNumber e, SVGNumber f);
};

#endif // SVG_TRANSFORM_H