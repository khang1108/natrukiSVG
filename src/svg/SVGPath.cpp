#include "svg/SVGPath.h"

#include <algorithm>
#include <cctype>
#include <cmath> 
#include <limits>
#include <sstream>

SVGPath::SVGPath(const std::string& dData) : m_cachedBBox{0, 0, 0, 0} { parseD(dData); }

void SVGPath::accept(NodeVisitor& visitor) { visitor.visit(*this); }

SVGRectF SVGPath::localBox() const { return m_cachedBBox; }

const std::vector<PathCommand>& SVGPath::getCommands() const { return m_commands; }

void SVGPath::parseD(const std::string& d)
{
    m_commands.clear();

    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();
    bool hasPoints = false;

    // Virtual Pen để theo dõi tọa độ thực tế khi tính BBox
    SVGNumber curX = 0.0;
    SVGNumber curY = 0.0;
    SVGNumber startX = 0.0; // Điểm bắt đầu của sub-path (để dùng cho lệnh Z)
    SVGNumber startY = 0.0;

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

        // 1. Xử lý dấu +/-
        if (d[i] == '-' || d[i] == '+')
            i++;

        // 2. Đọc phần số nguyên và thập phân (Chỉ cho phép 1 dấu chấm)
        bool dotSeen = false;
        while (i < len) {
            if (isdigit(d[i])) {
                i++;
            }
            else if (d[i] == '.') {
                if (dotSeen)
                    break; // Gặp dấu chấm thứ 2 -> DỪNG LẠI (Đây là bắt đầu số mới)
                dotSeen = true;
                i++;
            }
            else {
                break; // Ký tự khác -> Dừng
            }
        }

        // 3. Xử lý số mũ (Scientific notation: 1.2e-5)
        if (i < len && (d[i] == 'e' || d[i] == 'E')) {
            size_t eStart = i;
            i++;
            if (i < len && (d[i] == '-' || d[i] == '+'))
                i++;

            // Phải có ít nhất 1 số sau e/E
            if (i < len && isdigit(d[i])) {
                while (i < len && isdigit(d[i]))
                    i++;
            }
            else {
                // Nếu sau e không có số, rollback lại trước chữ e
                i = eStart;
            }
        }

        if (start == i)
            return 0.0;

        try {
            return std::stod(d.substr(start, i - start));
        }
        catch (...) {
            return 0.0;
        }
    };

    auto updateBounds = [&](SVGNumber x, SVGNumber y) {
        if (x < minX)
            minX = x;
        if (x > maxX)
            maxX = x;
        if (y < minY)
            minY = y;
        if (y > maxY)
            maxY = y;
        hasPoints = true;
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
        // Nếu không có chữ cái, dùng lại lệnh trước đó (implicit command)
        else if (currentCmd == '\0') {
            i++;
            continue;
        }

        PathCommand cmd;
        cmd.type = currentCmd;

        bool isRelative = islower(currentCmd);
        char type = tolower(currentCmd);

        // Xác định số lượng tham số và đọc chúng
        int argsCount = 0;
        if (type == 'z')
            argsCount = 0;
        else if (type == 'h' || type == 'v')
            argsCount = 1;
        else if (type == 'm' || type == 'l' || type == 't')
            argsCount = 2;
        else if (type == 's' || type == 'q')
            argsCount = 4;
        else if (type == 'c')
            argsCount = 6;
        else if (type == 'a')
            argsCount = 7;

        for (int k = 0; k < argsCount; ++k) {
            cmd.args.push_back(readNumber());
        }
        m_commands.push_back(cmd);

        SVGNumber nx = curX; // next X
        SVGNumber ny = curY; // next Y

        switch (type) {
        case 'm': // MoveTo
        case 'l': // LineTo
        case 't': // Smooth Quad
            nx = cmd.args[0];
            ny = cmd.args[1];
            if (isRelative) {
                nx += curX;
                ny += curY;
            }
            updateBounds(nx, ny);
            curX = nx;
            curY = ny;
            if (type == 'm') {
                startX = curX;
                startY = curY;
            } // M bắt đầu subpath mới
            break;

        case 'h': // Horizontal
            nx = cmd.args[0];
            if (isRelative)
                nx += curX;
            updateBounds(nx, curY);
            curX = nx;
            break;

        case 'v': // Vertical
            ny = cmd.args[0];
            if (isRelative)
                ny += curY;
            updateBounds(curX, ny);
            curY = ny;
            break;

        case 'c': // Cubic Bezier (x1 y1 x2 y2 x y)
            // Cập nhật bounds cho điểm đích (x,y)
            nx = cmd.args[4];
            ny = cmd.args[5];
            if (isRelative) {
                nx += curX;
                ny += curY;
            }
            updateBounds(nx, ny);

            // Cập nhật bounds cho các điểm điều khiển (Control points)
            // Để BBox bao trọn đường cong
            {
                SVGNumber c1x = cmd.args[0], c1y = cmd.args[1];
                SVGNumber c2x = cmd.args[2], c2y = cmd.args[3];
                if (isRelative) {
                    c1x += curX;
                    c1y += curY;
                    c2x += curX;
                    c2y += curY;
                }
                updateBounds(c1x, c1y);
                updateBounds(c2x, c2y);
            }
            curX = nx;
            curY = ny;
            break;

        case 's': // Smooth Cubic (x2 y2 x y)
            nx = cmd.args[2];
            ny = cmd.args[3];
            if (isRelative) {
                nx += curX;
                ny += curY;
            }
            updateBounds(nx, ny);

            {
                SVGNumber c2x = cmd.args[0], c2y = cmd.args[1];
                if (isRelative) {
                    c2x += curX;
                    c2y += curY;
                }
                updateBounds(c2x, c2y);
            }
            curX = nx;
            curY = ny;
            break;

        case 'q': // Quadratic Bezier (x1 y1 x y)
            nx = cmd.args[2];
            ny = cmd.args[3];
            if (isRelative) {
                nx += curX;
                ny += curY;
            }
            updateBounds(nx, ny);
            {
                SVGNumber c1x = cmd.args[0], c1y = cmd.args[1];
                if (isRelative) {
                    c1x += curX;
                    c1y += curY;
                }
                updateBounds(c1x, c1y);
            }
            curX = nx;
            curY = ny;
            break;

        case 'z': // Close Path
            curX = startX;
            curY = startY;
            break;

        case 'a': // Arc 
            nx = cmd.args[5];
            ny = cmd.args[6];
            if (isRelative) {
                nx += curX;
                ny += curY;
            }
            updateBounds(nx, ny);
            curX = nx;
            curY = ny;
            break;        }

        // Xử lý trường hợp đặc biệt: Sau M/m mà còn số thì các số sau là L/l
        if (currentCmd == 'M')
            currentCmd = 'L';
        if (currentCmd == 'm')
            currentCmd = 'l';
    }

    if (hasPoints) {
        m_cachedBBox = {minX, minY, maxX - minX, maxY - minY};
    }
    else {
        m_cachedBBox = {0, 0, 0, 0};
    }
}
