#include "SVGPath.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace
{
    inline bool isCommand(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0; }
} // namespace

SVGPath::SVGPath(const std::string& pathData) : m_pathData(pathData) { computeLocalBounds(); }

void SVGPath::skipSeparators(const std::string& data, size_t& index)
{
    while (index < data.size()) {
        char c = data[index];
        if (c == ',' || std::isspace(static_cast<unsigned char>(c))) {
            ++index;
        }
        else {
            break;
        }
    }
}

bool SVGPath::readNumber(const std::string& data, size_t& index, SVGNumber& value)
{
    skipSeparators(data, index);
    if (index >= data.size()) {
        return false;
    }

    const char* start = data.c_str() + index;
    char* end = nullptr;
    value = std::strtod(start, &end);
    if (start == end) {
        return false;
    }

    index = static_cast<size_t>(end - data.c_str());
    return true;
}

void SVGPath::computeLocalBounds()
{
    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();
    bool hasPoint = false;

    auto updateBounds = [&](SVGNumber x, SVGNumber y) {
        if (!std::isfinite(x) || !std::isfinite(y)) {
            return;
        }
        hasPoint = true;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    };

    const std::string& data = m_pathData;
    size_t index = 0;
    char command = 0;
    SVGPointF current{0, 0};
    SVGPointF subpathStart{0, 0};

    auto readPoint = [&](SVGPointF& out, bool relativeToCurrent) -> bool {
        SVGNumber x = 0;
        SVGNumber y = 0;
        if (!readNumber(data, index, x) || !readNumber(data, index, y)) {
            return false;
        }
        if (relativeToCurrent) {
            out.x = current.x + x;
            out.y = current.y + y;
        }
        else {
            out.x = x;
            out.y = y;
        }
        return true;
    };

    while (index < data.size()) {
        skipSeparators(data, index);
        if (index >= data.size()) {
            break;
        }

        char c = data[index];
        if (isCommand(c)) {
            command = c;
            ++index;
        }
        else if (command == 0) {
            ++index;
            continue;
        }

        bool isRelative = std::islower(static_cast<unsigned char>(command)) != 0;
        char upperCmd = static_cast<char>(std::toupper(static_cast<unsigned char>(command)));

        switch (upperCmd) {
        case 'M':
        case 'L':
        case 'T': {
            while (true) {
                SVGPointF point;
                if (!readPoint(point, isRelative)) {
                    break;
                }
                if (upperCmd == 'M') {
                    current = point;
                    subpathStart = point;
                    updateBounds(point.x, point.y);
                    upperCmd = 'L';
                    command = isRelative ? 'l' : 'L';
                    continue;
                }
                current = point;
                updateBounds(point.x, point.y);
                skipSeparators(data, index);
                if (index >= data.size() || isCommand(data[index])) {
                    break;
                }
            }
            break;
        }
        case 'H': {
            while (true) {
                SVGNumber x = 0;
                if (!readNumber(data, index, x)) {
                    break;
                }
                current.x = isRelative ? current.x + x : x;
                updateBounds(current.x, current.y);
                skipSeparators(data, index);
                if (index >= data.size() || isCommand(data[index])) {
                    break;
                }
            }
            break;
        }
        case 'V': {
            while (true) {
                SVGNumber y = 0;
                if (!readNumber(data, index, y)) {
                    break;
                }
                current.y = isRelative ? current.y + y : y;
                updateBounds(current.x, current.y);
                skipSeparators(data, index);
                if (index >= data.size() || isCommand(data[index])) {
                    break;
                }
            }
            break;
        }
        case 'C': {
            while (true) {
                SVGPointF control1;
                SVGPointF control2;
                SVGPointF endPoint;
                if (!readPoint(control1, isRelative) || !readPoint(control2, isRelative) ||
                    !readPoint(endPoint, isRelative)) {
                    break;
                }
                updateBounds(control1.x, control1.y);
                updateBounds(control2.x, control2.y);
                current = endPoint;
                updateBounds(current.x, current.y);
                skipSeparators(data, index);
                if (index >= data.size() || isCommand(data[index])) {
                    break;
                }
            }
            break;
        }
        case 'Q': {
            while (true) {
                SVGPointF control;
                SVGPointF endPoint;
                if (!readPoint(control, isRelative) || !readPoint(endPoint, isRelative)) {
                    break;
                }
                updateBounds(control.x, control.y);
                current = endPoint;
                updateBounds(current.x, current.y);
                skipSeparators(data, index);
                if (index >= data.size() || isCommand(data[index])) {
                    break;
                }
            }
            break;
        }
        case 'S': {
            while (true) {
                SVGPointF control;
                SVGPointF endPoint;
                if (!readPoint(control, isRelative) || !readPoint(endPoint, isRelative)) {
                    break;
                }
                updateBounds(control.x, control.y);
                current = endPoint;
                updateBounds(current.x, current.y);
                skipSeparators(data, index);
                if (index >= data.size() || isCommand(data[index])) {
                    break;
                }
            }
            break;
        }
        case 'A': {
            constexpr int paramsPerArc = 7;
            while (true) {
                SVGNumber values[paramsPerArc] = {};
                bool ok = true;
                for (int i = 0; i < paramsPerArc; ++i) {
                    ok &= readNumber(data, index, values[i]);
                }
                if (!ok) {
                    break;
                }
                SVGNumber x = values[5];
                SVGNumber y = values[6];
                if (isRelative) {
                    x += current.x;
                    y += current.y;
                }
                current = {x, y};
                updateBounds(current.x, current.y);
                skipSeparators(data, index);
                if (index >= data.size() || isCommand(data[index])) {
                    break;
                }
            }
            break;
        }
        case 'Z': {
            current = subpathStart;
            updateBounds(current.x, current.y);
            break;
        }
        default: {
            // Unsupported commands: skip until next command letter.
            while (index < data.size() && !isCommand(data[index])) {
                ++index;
            }
            break;
        }
        }
    }

    if (!hasPoint) {
        m_localBounds = {0, 0, 0, 0};
    }
    else {
        m_localBounds = {minX, minY, maxX - minX, maxY - minY};
    }
}

SVGRectF SVGPath::localBox() const { return m_localBounds; }

SVGRectF SVGPath::worldBox() const { return SVGElement::worldBox(); }