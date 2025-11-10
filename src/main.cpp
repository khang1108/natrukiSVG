//Main for testing
#include <iostream>
#include <string>
#include <memory>
#include <iomanip>    // Dùng để in màu fill VÀ std::fixed/setprecision
#include <sstream>    // Dùng cho std::stringstream
#include <cstdlib>    // Dùng cho system()

// Include các "sản phẩm" của Role B
#include "SVGDocument.h"
#include "SVGElement.h"
#include "SVGGroup.h"
#include "SVGRect.h"
#include "SVGCircle.h"
#include "SVGEllipse.h"
#include "SVGLine.h"
#include "SVGPolygon.h"
#include "SVGPolyline.h"
#include "SVGText.h"

// Helper để in màu
std::string printFill(const SVGColor& color) {
    if (color.isNone) return "none";
    std::stringstream ss;
    ss << "rgb(" << (int)color.r << "," << (int)color.g << "," << (int)color.b << ")";
    return ss.str();
}

// Helper để in BBox
void printBox(const std::string& prefix, const SVGRectF& box, const std::string& indent) {
    // std::cout đã được set std::fixed trong hàm main
    std::cout << indent << "  " << prefix
        << ": (x=" << box.x << ", y=" << box.y
        << ", w=" << box.width << ", h=" << box.height << ")\n";
}


/**
 * @brief Hàm helper đệ quy để "in" cây Scene Graph
 * (Đã hỗ trợ tất cả 8 loại shape)
 */
void printTree(SVGElement* element, const std::string& indent = "") {
    if (!element) return;

    std::cout << indent << "Fill: " << printFill(element->getStyle().fillColor);

    if (SVGRect* rect = dynamic_cast<SVGRect*>(element)) {
        std::cout << " [Rect] w=" << rect->getRect().width << "\n";
        printBox("LocalBox", rect->localBox(), indent);
        printBox("WorldBox", rect->worldBox(), indent);
    }
    else if (SVGCircle* circle = dynamic_cast<SVGCircle*>(element)) {
        std::cout << " [Circle] r=" << circle->getRadius() << "\n";
        printBox("LocalBox", circle->localBox(), indent);
        printBox("WorldBox", circle->worldBox(), indent);
    }
    else if (SVGEllipse* ellipse = dynamic_cast<SVGEllipse*>(element)) {
        std::cout << " [Ellipse] rx=" << ellipse->getRx() << "\n";
        printBox("LocalBox", ellipse->localBox(), indent);
        printBox("WorldBox", ellipse->worldBox(), indent);
    }
    else if (SVGLine* line = dynamic_cast<SVGLine*>(element)) {
        std::cout << " [Line] x1=" << line->getP1().x << "\n";
        printBox("LocalBox", line->localBox(), indent);
        printBox("WorldBox", line->worldBox(), indent);
    }
    else if (SVGPolygon* polygon = dynamic_cast<SVGPolygon*>(element)) {
        std::cout << " [Polygon] Points=" << polygon->getPoints().size() << "\n";
        printBox("LocalBox", polygon->localBox(), indent);
        printBox("WorldBox", polygon->worldBox(), indent);
    }
    else if (SVGPolyline* polyline = dynamic_cast<SVGPolyline*>(element)) {
        std::cout << " [Polyline] Points=" << polyline->getPoints().size() << "\n";
        printBox("LocalBox", polyline->localBox(), indent);
        printBox("WorldBox", polyline->worldBox(), indent);
    }
    else if (SVGText* text = dynamic_cast<SVGText*>(element)) {
        std::cout << " [Text] Content: \"" << text->getText() << "\"\n";
        printBox("LocalBox", text->localBox(), indent);
        printBox("WorldBox", text->worldBox(), indent);
    }
    else if (SVGGroup* group = dynamic_cast<SVGGroup*>(element)) {
        std::cout << " [Group]\n";
        printBox("LocalBox (cua con)", group->localBox(), indent);
        printBox("WorldBox (cua Group)", group->worldBox(), indent);

        for (const auto& child : group->getChildren()) {
            printTree(child.get(), indent + "  ");
        }
    }
    else {
        std::cout << " [Unknown Element]\n";
    }
}

// === HÀM MAIN ===
int main(int argc, char* argv[]) {
    // Sửa lỗi font chữ "cá»§a" trên Console Windows
    system("chcp 65001 > nul"); // "> nul" để ẩn output của lệnh system

    // Sửa lỗi định dạng "8e+02"
    // Ép std::cout luôn dùng định dạng thập phân (fixed)
    // và hiển thị 1 chữ số sau dấu phẩy.
    std::cout << std::fixed << std::setprecision(1);

    std::cout << "=== Bat dau Test Parser (Role B) ===\n" << std::endl;

    // --- CHỌN FILE TEST Ở ĐÂY ---
    // (Hãy copy các file test vào thư mục build (ví dụ: 'out/build/x64-Debug')
    // hoặc nằm cùng cấp với file .exe)

    std::string filePath = "sample.svg";
    // std::string filePath = "test_style.svg";
    // std::string filePath = "test_transform.svg";

    std::cout << "--- Dang load file: " << filePath << " ---\n" << std::endl;

    SVGDocument myDocument;
    bool success = false;
    try {
        success = myDocument.load(filePath);
    }
    catch (const std::exception& e) {
        std::cerr << "Loi NGHIEM TRONG: " << e.what() << std::endl;
        return -1;
    }

    if (!success) {
        std::cerr << "Loi: Khong the load file: " << filePath << std::endl;
        std::cerr << "(Hay dam bao file test nam trong thu muc build cua ban)" << std::endl;
        return -1;
    }

    std::cout << "Load file thanh cong." << std::endl;
    std::cout << "\n--- In Cay Scene Graph ---" << std::endl;

    for (const auto& child : myDocument.getChildren()) {
        printTree(child.get());
    }

    std::cout << "\n=== Test Parser Hoan Tat ===\n" << std::endl;
    std::cout << "Nhan Enter de thoat...";
    std::cin.get();
    return 0;
}