#include "SVGStyle.h"
#include <limits> // Dùng cho 'chưa được set'

// Helper: Chúng ta cần một cách để biết một màu đã được set hay chưa.
// Chúng ta có thể dùng 'isNone', nhưng một cách khác an toàn hơn
// là dùng giá trị 'alpha' (độ trong suốt)
// Giá trị 'a = 0' (hoàn toàn trong suốt) có thể coi là 'chưa set'.
// Giá trị 'a = 255' là mặc định (không trong suốt).
// 'isNone = true' sẽ đè lên tất cả (cho fill="none" hoặc stroke="none").

namespace {
	// Hàm helper nội bộ để kiểm tra xem màu đã được set chưa
	// Chúng ta quy ước: nếu 'isNone' là false và 'a' bằng 0,
	// thì màu này "chưa được set" và sẵn sàng để kế thừa.
	bool IsColorSet(const SVGColor& color) {
		if (color.isNone) {
			return true; // "none" là một giá trị đã set
		}
		// Nếu alpha > 0, nó đã được set (ví dụ: black (0,0,0,255) hoặc red (255,0,0,255))
		return color.a > 0;
	}

	// Giá trị 'chưa set' cho SVGNumber. 
	// Chúng ta dùng một giá trị không hợp lệ (ví dụ: số âm)
	const SVGNumber UNSET_NUMBER = -1.0;
}

SVGStyle::SVGStyle()
	: fillColor{ 0, 0, 0, 255 } // Mặc định fill là black
	, strokeColor{ 0, 0, 0, 0, true } // Mặc định stroke là 'none'
	, strokeWidth(1.0)
	, fillOpacity(1.0)
	, strokeOpacity(1.0)
	, fillRule(SVGFillRule::NonZero)
	, fontSize(16.0)
{
	// Các giá trị "chưa set" để chờ kế thừa
	// Chúng ta cần một cách để phân biệt "mặc định" và "chưa set"
	// Hãy sửa lại một chút:

	// Mặc định ban đầu, mọi thứ đều "chưa set"
	// để sẵn sàng kế thừa từ style gốc (user agent stylesheet)

	// fillColor: chưa set (chờ kế thừa, mặc định là black)
	fillColor = { 0, 0, 0, 0, false };
	// strokeColor: chưa set (chờ kế thừa, mặc định là none)
	strokeColor = { 0, 0, 0, 0, false };
	// strokeWidth: chưa set
	strokeWidth = UNSET_NUMBER;
	// fillOpacity: chưa set
	fillOpacity = UNSET_NUMBER;
	// strokeOpacity: chưa set
	strokeOpacity = UNSET_NUMBER;
	// fillRule: chưa set
	// (Chúng ta cần một giá trị 'UNSET' cho enum, tạm thời mặc định)
	fillRule = SVGFillRule::NonZero;

	// fontSize: chưa set
	fontSize = UNSET_NUMBER;
	// fontFamily: chưa set (chuỗi rỗng)
}

void SVGStyle::inheritFrom(const SVGStyle& parent) {
	// Đây là logic "Cascade Style" cốt lõi.
	// NẾU thuộc tính của 'this' (con) chưa được set,
	// THÌ lấy giá trị từ 'parent' (cha).

	// 1. Fill Color
	if (!IsColorSet(this->fillColor)) {
		this->fillColor = parent.fillColor;
	}

	// 2. Stroke Color
	if (!IsColorSet(this->strokeColor)) {
		this->strokeColor = parent.strokeColor;
	}

	// 3. Stroke Width
	if (this->strokeWidth == UNSET_NUMBER) {
		this->strokeWidth = parent.strokeWidth;
	}

	// 4. Fill Opacity
	if (this->fillOpacity == UNSET_NUMBER) {
		this->fillOpacity = parent.fillOpacity;
	}

	// 5. Stroke Opacity
	if (this->strokeOpacity == UNSET_NUMBER) {
		this->strokeOpacity = parent.strokeOpacity;
	}

	// 6. Fill Rule
	// (Giả sử NonZero là mặc định nếu cả hai đều chưa set)
	// Nếu chúng ta thêm một giá trị 'UNSET' cho enum thì sẽ tốt hơn
	if (this->fillRule != SVGFillRule::EvenOdd) { // Logic ví dụ
		if (parent.fillRule == SVGFillRule::EvenOdd) {
			this->fillRule = SVGFillRule::EvenOdd;
		}
	}

	// 7. Font Family
	if (this->fontFamily.empty()) {
		this->fontFamily = parent.fontFamily;
	}

	// 8. Font Size
	if (this->fontSize == UNSET_NUMBER) {
		this->fontSize = parent.fontSize;
	}
}