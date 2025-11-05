#ifndef SVG_TYPES_H
#define SVG_TYPES_H

#include <string>
#include <vector>
#include <map>

// Dùng double cho độ chính xác cao
using SVGNumber = double;

// Định nghĩa màu cơ bản
struct SVGColor {
    unsigned char r = 0, g = 0, b = 0, a = 255;
    bool isNone = false; // Dùng cho 'fill="none"'
};

// Định nghĩa điểm 2D
struct SVGPointF {
    SVGNumber x = 0.0;
    SVGNumber y = 0.0;
};

// Định nghĩa hình chữ nhật
struct SVGRectF {
    SVGNumber x = 0.0;
    SVGNumber y = 0.0;
    SVGNumber width = 0.0;
    SVGNumber height = 0.0;
};

// Map dùng để parse style hoặc transform (Role B dùng)
using SVGAttributeMap = std::map<std::string, std::string>;

// Enum cho 'fill-rule'
enum class SVGFillRule {
    NonZero,
    EvenOdd
};

#endif // SVG_TYPES_H