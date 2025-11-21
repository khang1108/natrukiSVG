#include "svg/SVGPath.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

SVGPath::SVGPath(const std::string& dData) : m_cachedBBox{0, 0, 0, 0} { parseD(dData); }

void SVGPath::accept(NodeVisitor& visitor) { visitor.visit(*this); }

const std::vector<PathCommand>& SVGPath::getCommands() const { return m_commands; }

SVGRectF SVGPath::localBox() const
{
    // Trả về BBox bao quanh tất cả các điểm điều khiển (Control Points)
    // Đây là cách tính gần đúng (nhanh), đủ tốt cho UI.
    // Tính chính xác Bezier curve bbox rất phức tạp.
    return m_cachedBBox;
}

void SVGPath::parseD(const std::string& d)
{
    m_commands.clear();

    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();
    bool hasPoints = false;

    size_t i = 0;
    size_t len = d.length();

    auto skipDelimiters = [&]() {
        while (i < len && (std::isspace(d[i]) || d[i] == ',')) {
            i++;
        }
    };

    auto readNumber = [&]() -> SVGNumber {
        skipDelimiters();
        if (i >= len)
            return 0.0;

        size_t start = i;
        if (d[i] == '-' || d[i] == '+')
            i++;
        while (i < len && (isdigit(d[i]) || d[i] == '.'))
            i++;
        // Handle scientific notation (e.g., 1.2e-5)
        if (i < len && (d[i] == 'e' || d[i] == 'E')) {
            i++;
            if (i < len && (d[i] == '-' || d[i] == '+'))
                i++;
            while (i < len && isdigit(d[i]))
                i++;
        }

        std::string numStr = d.substr(start, i - start);
        try {
            return std::stod(numStr);
        }
        catch (...) {
            return 0.0;
        }
    };

    char currentCmd = '\0';

    while (i < len) {
        skipDelimiters();
        if (i >= len)
            break;

        if (isalpha(d[i])) {
            currentCmd = d[i];
            i++;
        }
        // Nếu không gặp chữ cái, nghĩa là lệnh lặp lại (implicit command)
        // Ví dụ: L 10 10 20 20 -> hiểu là L 10 10 rồi L 20 20
        else if (currentCmd == '\0') {
            // Lỗi format, bỏ qua
            i++;
            continue;
        }

        PathCommand cmd;
        cmd.type = currentCmd;

        // Xác định số lượng tham số dựa trên loại lệnh
        int argsCount = 0;
        char lowerCmd = tolower(currentCmd);

        if (lowerCmd == 'z')
            argsCount = 0;
        else if (lowerCmd == 'h' || lowerCmd == 'v')
            argsCount = 1;
        else if (lowerCmd == 'm' || lowerCmd == 'l' || lowerCmd == 't')
            argsCount = 2;
        else if (lowerCmd == 's' || lowerCmd == 'q')
            argsCount = 4;
        else if (lowerCmd == 'c')
            argsCount = 6;
        else if (lowerCmd == 'a')
            argsCount = 7;

        for (int k = 0; k < argsCount; ++k) {
            SVGNumber val = readNumber();
            cmd.args.push_back(val);

            // Cập nhật BBox (chỉ lấy toạ độ x,y, bỏ qua flag hay bán kính)
            // Logic này sơ sài để lấy bounds nhanh
            if (lowerCmd != 'a') { // Arc xử lý phức tạp hơn, tạm bỏ qua check bounds chi tiết
                if (k % 2 == 0) {  // X
                    if (val < minX)
                        minX = val;
                    if (val > maxX)
                        maxX = val;
                }
                else { // Y
                    if (val < minY)
                        minY = val;
                    if (val > maxY)
                        maxY = val;
                }
                hasPoints = true;
            }
        }

        m_commands.push_back(cmd);

        // Logic đặc biệt của SVG: Sau lệnh 'M' (Move), nếu còn số thì các số sau được hiểu là 'L'
        // (Line)
        if (currentCmd == 'M')
            currentCmd = 'L';
        if (currentCmd == 'm')
            currentCmd = 'l';
    }

    if (hasPoints) {
        m_cachedBBox = {minX, minY, maxX - minX, maxY - minY};
    }
}