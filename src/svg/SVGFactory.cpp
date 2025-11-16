// (File này BÂY GIỜ chỉ định nghĩa hàm 'static' - cửa vào)

#include "SVGFactory.h"
#include "SVGFactoryImpl.h" // <--- Include lớp triển khai "bí mật"
#include <memory>

// HÀM STATIC "CÔNG KHAI" (Implement hàm trong .h)
std::unique_ptr<SVGFactory> SVGFactory::createDefaultFactory() {
    // Trả về một đối tượng của lớp "bí mật"
    return std::make_unique<SVGFactoryImpl>();
}