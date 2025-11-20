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

/**
 * @brief Calculates the bounding box (local bounds) of the SVG path.
 *
 * Algorithm:
 * 1. Parse the path data string similar to rendering, but track min/max coordinates instead
 * 2. For each path command, track all points that the path passes through:
 *    - M/L: Track the point coordinates
 *    - H: Track the X coordinate (Y stays same)
 *    - V: Track the Y coordinate (X stays same)
 *    - C/S: Track control points and end point (approximate bounds for curves)
 *    - Q/T: Track control point and end point
 *    - A: Track end point (simplified - full implementation would calculate arc bounds)
 *    - Z: Track subpath start point
 * 3. Update min/max X and Y values as we encounter points
 * 4. Return a rectangle containing all points
 *
 * Note: For curves (C, S, Q, T, A), this is an approximation. A more accurate implementation
 * would calculate the actual extrema of the curves.
 *
 * The bounds are calculated in local coordinate space (before any transforms are applied).
 */
void SVGPath::computeLocalBounds()
{
    // Initialize bounds to extreme values
    SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
    SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();
    bool hasPoint = false;

    // Lambda function to update bounding box with a new point
    auto updateBounds = [&](SVGNumber x, SVGNumber y) {
        if (!std::isfinite(x) || !std::isfinite(y)) {
            return; // Skip invalid coordinates
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
        case 'M': {
            bool firstPoint = true;
            while (true) {
                SVGPointF point;
                if (!readPoint(point, isRelative)) {
                    break;
                }
                if (firstPoint) {
                    current = point;
                    subpathStart = point;
                    updateBounds(point.x, point.y);
                    firstPoint = false;
                }
                else {
                    current = point;
                    updateBounds(point.x, point.y);
                }
                skipSeparators(data, index);
                if (index >= data.size() || isCommand(data[index])) {
                    break;
                }
            }
            // After M, subsequent coordinates are treated as L commands
            // Check if there are more coordinates (not a command) to process as L
            // Note: skipSeparators was already called in the loop, so index should be
            // pointing to the next character (command or coordinate)
            if (index < data.size() && !isCommand(data[index])) {
                upperCmd = 'L';
                command = isRelative ? 'l' : 'L';
            }
            else {
                // Next character is a command, clear so it will be processed in next iteration
                command = 0;
            }
            break;
        }
        case 'L':
        case 'T': {
            while (true) {
                SVGPointF point;
                if (!readPoint(point, isRelative)) {
                    break;
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
        case 'Z':
        case 'z': {
            current = subpathStart;
            updateBounds(current.x, current.y);
            // Clear command after Z so we look for a new one
            command = 0;
            break;
        }
        default: {
            // Unsupported commands: skip until next command letter.
            while (index < data.size() && !isCommand(data[index])) {
                ++index;
            }
            // Clear command if we hit unknown
            command = 0;
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