#include "SVGTransform.h"
#include <cmath>     // Dùng cho std::cos, std::sin
#include <cstring>   // Dùng cho std::memset hoặc std::memcpy
#include <algorithm> // Dùng cho std::swap (nếu cần)

// Định nghĩa hằng số PI nếu chưa có
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SVGTransform::SVGTransform() {
	// Khởi tạo ma trận đơn vị (Identity Matrix)
	// [ 1  0  0 ]
	// [ 0  1  0 ]
	// [ 0  0  1 ]

	// Xóa sạch bộ nhớ (gán tất cả bằng 0)
	std::memset(m_matrix, 0, sizeof(m_matrix));

	// Gán đường chéo chính là 1.0
	m_matrix[0][0] = 1.0;
	m_matrix[1][1] = 1.0;
	m_matrix[2][2] = 1.0;
}

void SVGTransform::multiply(const SVGTransform& other) {
	// Thực hiện phép nhân ma trận: C = A * B
	// A là 'm_matrix' (hiện tại)
	// B là 'other.m_matrix'
	// C là 'result' (tạm thời)

	SVGNumber result[3][3] = { {0.0, 0.0, 0.0},
							{0.0, 0.0, 0.0},
							{0.0, 0.0, 0.0} };

	for (int i = 0; i < 3; ++i) { // Hàng của A (this)
		for (int j = 0; j < 3; ++j) { // Cột của B (other)
			// Tính toán giá trị cho result[i][j]
			for (int k = 0; k < 3; ++k) {
				result[i][j] += m_matrix[i][k] * other.m_matrix[k][j];
			}
		}
	}

	// Sao chép kết quả 'result' vào 'm_matrix'
	std::memcpy(m_matrix, result, sizeof(m_matrix));
}

void SVGTransform::translate(SVGNumber tx, SVGNumber ty) {
	// Tạo một ma trận tịnh tiến (Translate)
	// [ 1  0  tx ]
	// [ 0  1  ty ]
	// [ 0  0  1  ]
	SVGTransform translateMatrix; // Khởi tạo là ma trận đơn vị
	translateMatrix.m_matrix[0][2] = tx;
	translateMatrix.m_matrix[1][2] = ty;

	// Nhân ma trận hiện tại với ma trận tịnh tiến
	// A = A * B (áp dụng phép biến đổi mới 'B' vào 'A')
	this->multiply(translateMatrix);
}

void SVGTransform::scale(SVGNumber sx, SVGNumber sy) {
	// Tạo ma trận co giãn (Scale)
	// [ sx 0  0 ]
	// [ 0  sy 0 ]
	// [ 0  0  1 ]
	SVGTransform scaleMatrix;
	scaleMatrix.m_matrix[0][0] = sx;
	scaleMatrix.m_matrix[1][1] = sy;

	// Nhân
	this->multiply(scaleMatrix);
}

void SVGTransform::rotate(SVGNumber angle) {
	// 1. Chuyển đổi góc (độ) sang radians, vì std::cos/sin dùng radians
	SVGNumber rad = angle * M_PI / 180.0;
	SVGNumber c = std::cos(rad);
	SVGNumber s = std::sin(rad);

	// 2. Tạo ma trận xoay (Rotate)
	// [ cos  -sin  0 ]
	// [ sin   cos  0 ]
	// [ 0     0   1 ]
	SVGTransform rotateMatrix;
	rotateMatrix.m_matrix[0][0] = c;
	rotateMatrix.m_matrix[0][1] = -s;
	rotateMatrix.m_matrix[1][0] = s;
	rotateMatrix.m_matrix[1][1] = c;

	// 3. Nhân
	this->multiply(rotateMatrix);
}

SVGPointF SVGTransform::map(const SVGPointF& point) const {
	// Áp dụng ma trận lên điểm (x, y)
	// Chúng ta dùng tọa độ đồng nhất (x, y, 1)
	//
	// [ new_x ]   [ m[0][0]  m[0][1]  m[0][2] ]   [ x ]
	// [ new_y ] = [ m[1][0]  m[1][1]  m[1][2] ] * [ y ]
	// [  1    ]   [ m[2][0]  m[2][1]  m[2][2] ]   [ 1 ]
	//
	// new_x = m[0][0]*x + m[0][1]*y + m[0][2]*1
	// new_y = m[1][0]*x + m[1][1]*y + m[1][2]*1
	// (Bỏ qua hàng thứ 3 vì chỉ làm 2D)

	SVGPointF newPoint;

	// Lấy x, y từ struct SVGPointF
	SVGNumber x = point.x;
	SVGNumber y = point.y;

	newPoint.x = m_matrix[0][0] * x + m_matrix[0][1] * y + m_matrix[0][2];
	newPoint.y = m_matrix[1][0] * x + m_matrix[1][1] * y + m_matrix[1][2];

	return newPoint;
}