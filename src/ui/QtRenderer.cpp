#include "ui/QtRenderer.h"

#include "svg/SVGCircle.h"
#include "svg/SVGEllipse.h"
#include "svg/SVGGroup.h"
#include "svg/SVGLine.h"
#include "svg/SVGPath.h"
#include "svg/SVGPolygon.h"
#include "svg/SVGPolyline.h"
#include "svg/SVGRect.h"
#include "svg/SVGStyle.h"
#include "svg/SVGText.h"
#include "svg/SVGTransform.h"
#include "svg/SVGDocument.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <iostream>
#include <QLocale>
#include "svg/SVGGradient.h"

namespace
{

    /**
     * @brief Converts SVGTransform to QTransform (Qt's transformation matrix).
     *
     * Algorithm:
     * - Maps three reference points through the SVG transform to extract matrix components
     * - Reference points: origin (0,0), x-axis (1,0), y-axis (0,1)
     * - Calculate matrix elements from the transformed points:
     *   - m11, m21: X-axis transformation (how (1,0) is transformed)
     *   - m12, m22: Y-axis transformation (how (0,1) is transformed)
     *   - dx, dy: Translation (how (0,0) is transformed)
     *
     * Matrix format:
     * QTransform: [m11 m12 dx]   SVGTransform: [m[0][0] m[0][1] m[0][2]]
     *             [m21 m22 dy]                  [m[1][0] m[1][1] m[1][2]]
     *             [0   0   1 ]                  [m[2][0] m[2][1] m[2][2]]
     *
     * Formula:
     * - m11 = (transformed(1,0).x - transformed(0,0).x) = X-axis X component
     * - m21 = (transformed(1,0).y - transformed(0,0).y) = X-axis Y component
     * - m12 = (transformed(0,1).x - transformed(0,0).x) = Y-axis X component
     * - m22 = (transformed(0,1).y - transformed(0,0).y) = Y-axis Y component
     * - dx = transformed(0,0).x = translation X
     * - dy = transformed(0,0).y = translation Y
     *
     * @param transform The SVG transform to convert
     * @return QTransform representing the same transformation
     */
    QTransform toQTransform(const SVGTransform& transform)
    {
        // Map reference points to extract matrix components
        SVGPointF origin{0.0, 0.0};
        SVGPointF xAxis{1.0, 0.0};
        SVGPointF yAxis{0.0, 1.0};

        SVGPointF mappedOrigin = transform.map(origin);
        SVGPointF mappedX = transform.map(xAxis);
        SVGPointF mappedY = transform.map(yAxis);

        // Extract matrix components from transformed points
        qreal m11 = mappedX.x - mappedOrigin.x; // X-axis X component
        qreal m21 = mappedX.y - mappedOrigin.y; // X-axis Y component
        qreal m12 = mappedY.x - mappedOrigin.x; // Y-axis X component
        qreal m22 = mappedY.y - mappedOrigin.y; // Y-axis Y component
        qreal dx = mappedOrigin.x;              // Translation X
        qreal dy = mappedOrigin.y;              // Translation Y

        return QTransform(m11, m21, 0.0, m12, m22, 0.0, dx, dy, 1.0);
    }

    double normalizedOpacity(SVGNumber value)
    {
        if (value < 0.0) {
            return 1.0;
        }
        return std::clamp(static_cast<double>(value), 0.0, 1.0);
    }

    QColor toQColor(const SVGColor& color, double opacityFactor)
    {
        if (color.isNone) {
            return QColor(Qt::transparent);
        }

        double alpha = normalizedOpacity(static_cast<double>(color.a) / 255.0) * opacityFactor;
        QColor qColor(color.r, color.g, color.b);
        qColor.setAlphaF(std::clamp(alpha, 0.0, 1.0));
        return qColor;
    }

    bool hasVisibleFill(const SVGStyle& style)
    {
        // If a gradient URL is present, it is visible (unless opacity is 0)
        if (!style.fillUrl.empty()) {
            return normalizedOpacity(style.fillOpacity) > 0.0;
        }
        if (style.fillColor.isNone) {
            return false;
        }
        return normalizedOpacity(style.fillOpacity) > 0.0;
    }

    bool hasVisibleStroke(const SVGStyle& style)
    {
        // If a gradient URL is present, it is visible
        if (!style.strokeUrl.empty()) {
             if (style.strokeWidth <= 0.0) return false;
             return normalizedOpacity(style.strokeOpacity) > 0.0;
        }
        if (style.strokeColor.isNone) {
            return false;
        }
        if (style.strokeWidth <= 0.0) {
            return false;
        }
        return normalizedOpacity(style.strokeOpacity) > 0.0;
    }

    Qt::FillRule toQtFillRule(SVGFillRule rule)
    {
        return (rule == SVGFillRule::EvenOdd) ? Qt::OddEvenFill : Qt::WindingFill;
    }

    bool isCommand(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0; }

    /**
     * @class SvgPathBuilder
     * @brief Parses SVG path data string and builds a QPainterPath object.
     *
     * This class implements the SVG path data parsing algorithm according to the SVG specification.
     * It handles all path commands (M, L, H, V, C, S, Q, T, A, Z) in both absolute (uppercase)
     * and relative (lowercase) forms.
     *
     * Algorithm Overview:
     * 1. Parse the path data string character by character
     * 2. Detect command letters (M, L, C, etc.) and set the current command
     * 3. For each command, read the required number of coordinates
     * 4. Convert relative coordinates to absolute by adding to current position
     * 5. Build the QPainterPath by calling appropriate methods (moveTo, lineTo, cubicTo, etc.)
     * 6. Handle command repetition: if coordinates follow a command without a new command letter,
     *    they are treated as the same command (e.g., "M 10 20 30 40" = move to (10,20) then line to
     * (30,40))
     */
    class SvgPathBuilder
    {
      public:
        explicit SvgPathBuilder(const std::string& data) : m_data(data) {}

        /**
         * @brief Main parsing function that converts SVG path data string to QPainterPath.
         *
         * Algorithm:
         * 1. Loop through the path data string
         * 2. Skip whitespace and commas (separators)
         * 3. If we encounter a command letter (M, L, C, etc.):
         *    - Store it as the current command
         *    - Determine if it's relative (lowercase) or absolute (uppercase)
         *    - Call the appropriate handler function
         * 4. If we encounter numbers without a command:
         *    - Use the last command (for command repetition)
         *    - Parse coordinates and apply them to the current command
         * 5. Continue until the entire string is processed
         *
         * Special handling:
         * - 'M' (moveto): First coordinate pair moves, subsequent pairs are treated as 'L' (lineto)
         * - 'Z' (closepath): Closes the current subpath and resets to subpath start
         * - Command repetition: Multiple coordinate sets after a command use the same command
         *
         * @return QPainterPath representing the parsed path
         */
        QPainterPath build()
        {
            while (m_index < m_data.size()) {
                skipSeparators();
                if (m_index >= m_data.size()) {
                    break;
                }

                char c = m_data[m_index];
                if (isCommand(c)) {
                    // Found a command letter - store it and advance past it
                    m_command = c;
                    ++m_index;
                }
                else if (m_command == 0) {
                    // No command set yet, skip this character (invalid input)
                    ++m_index;
                    continue;
                }

                // Determine if command is relative (lowercase) or absolute (uppercase)
                // Relative commands: coordinates are relative to current position
                // Absolute commands: coordinates are in absolute coordinate space
                bool relative = std::islower(static_cast<unsigned char>(m_command)) != 0;
                char cmd = static_cast<char>(std::toupper(static_cast<unsigned char>(m_command)));

                switch (cmd) {
                case 'M': // Moveto command - starts a new subpath
                    // Algorithm: First coordinate pair moves to that position.
                    // Subsequent coordinate pairs are treated as implicit 'L' (lineto) commands.
                    // This is per SVG spec: "M 10 20 30 40" = move to (10,20), then line to (30,40)
                    handleMove(relative);
                    // After M, subsequent coordinates are treated as L commands
                    // Check if there are more coordinates (not a command) to process as L
                    // Note: handleMove already called skipSeparators(), so m_index should be
                    // pointing to the next character (command or coordinate)
                    if (m_index < m_data.size() && !isCommand(m_data[m_index])) {
                        // More coordinates follow - treat them as line commands
                        if (relative) {
                            m_command = 'l'; // Keep relative mode
                        }
                        else {
                            m_command = 'L'; // Keep absolute mode
                        }
                    }
                    else {
                        // Next character is a command, clear so it will be processed in next
                        // iteration
                        m_command = 0;
                    }
                    break;
                case 'L':
                    handleLine(relative);
                    if (m_index < m_data.size() && isCommand(m_data[m_index])) {
                        m_command = 0;
                    }
                    break;
                case 'H':
                    handleHorizontal(relative);
                    if (m_index < m_data.size() && isCommand(m_data[m_index])) {
                        m_command = 0;
                    }
                    break;
                case 'V':
                    handleVertical(relative);
                    if (m_index < m_data.size() && isCommand(m_data[m_index])) {
                        m_command = 0;
                    }
                    break;
                case 'C':
                    handleCubic(relative);
                    // After handler breaks, if we encountered a new command, clear m_command
                    // so next iteration can detect it. Otherwise, keep command for more
                    // coordinates.
                    if (m_index < m_data.size() && isCommand(m_data[m_index])) {
                        m_command = 0; // Clear so next iteration detects the new command
                    }
                    break;
                case 'S':
                    handleSmoothCubic(relative);
                    if (m_index < m_data.size() && isCommand(m_data[m_index])) {
                        m_command = 0;
                    }
                    break;
                case 'Q':
                    handleQuadratic(relative);
                    if (m_index < m_data.size() && isCommand(m_data[m_index])) {
                        m_command = 0;
                    }
                    break;
                case 'T':
                    handleSmoothQuadratic(relative);
                    if (m_index < m_data.size() && isCommand(m_data[m_index])) {
                        m_command = 0;
                    }
                    break;
                case 'A':
                    handleArc(relative);
                    if (m_index < m_data.size() && isCommand(m_data[m_index])) {
                        m_command = 0;
                    }
                    break;
                case 'Z':
                case 'z':
                    m_path.closeSubpath();
                    m_current = m_subpathStart;
                    m_prevCommand = 'Z';
                    // Clear command after Z so we look for a new one
                    m_command = 0;
                    break;
                default:
                    skipUnknown();
                    // Clear command if we hit unknown
                    m_command = 0;
                    break;
                }
            }

            return m_path;
        }

      private:
        const std::string& m_data;
        size_t m_index = 0;
        char m_command = 0;
        char m_prevCommand = 0;
        QPointF m_current{0.0, 0.0};
        QPointF m_subpathStart{0.0, 0.0};
        QPointF m_lastCubicControl{0.0, 0.0};
        QPointF m_lastQuadControl{0.0, 0.0};
        QPainterPath m_path;

        /**
         * @brief Skips whitespace characters and commas in the path data string.
         *
         * SVG path data allows whitespace and commas as separators between numbers and commands.
         * This function advances m_index past any consecutive separators.
         *
         * Algorithm: Loop while current character is whitespace or comma, incrementing m_index.
         */
        void skipSeparators()
        {
            while (m_index < m_data.size()) {
                char c = m_data[m_index];
                if (c == ',' || std::isspace(static_cast<unsigned char>(c))) {
                    ++m_index;
                }
                else {
                    break;
                }
            }
        }

        /**
         * @brief Reads a single floating-point number from the path data string.
         *
         * Algorithm:
         * 1. Skip any leading separators (whitespace, commas)
         * 2. Use strtod to parse the number (handles scientific notation, decimals, etc.)
         * 3. Update m_index to point after the parsed number
         * 4. Return false if no valid number found
         *
         * @param value Output parameter for the parsed number
         * @return true if a number was successfully parsed, false otherwise
         */
        /**
         * @brief Reads a single floating-point number from the path data string.
         *
         * Algorithm:
         * 1. Skip any leading separators (whitespace, commas)
         * 2. Parse number manually to ensure locale independence (always use dot for decimal)
         *    and specific SVG number rules.
         * 3. Update m_index to point after the parsed number
         * 4. Return false if no valid number found
         *
         * @param value Output parameter for the parsed number
         * @return true if a number was successfully parsed, false otherwise
         */
        bool readNumber(double& value)
        {
            skipSeparators();
            if (m_index >= m_data.size()) {
                return false;
            }

            size_t start = m_index;
            // Handle sign
            if (m_index < m_data.size() && (m_data[m_index] == '+' || m_data[m_index] == '-')) {
                ++m_index;
            }
            // Handle digits before dot
            bool hasMsg = false;
            while (m_index < m_data.size() && std::isdigit(static_cast<unsigned char>(m_data[m_index]))) {
                ++m_index;
                hasMsg = true;
            }
            // Handle dot and digits after dot
            if (m_index < m_data.size() && m_data[m_index] == '.') {
                ++m_index;
                while (m_index < m_data.size() && std::isdigit(static_cast<unsigned char>(m_data[m_index]))) {
                    ++m_index;
                    hasMsg = true;
                }
            }
            // Handle exponent
            if (hasMsg && m_index < m_data.size() && (m_data[m_index] == 'e' || m_data[m_index] == 'E')) {
                size_t eStart = m_index;
                ++m_index;
                if (m_index < m_data.size() && (m_data[m_index] == '+' || m_data[m_index] == '-')) {
                    ++m_index;
                }
                bool hasExpDigits = false;
                while (m_index < m_data.size() && std::isdigit(static_cast<unsigned char>(m_data[m_index]))) {
                    ++m_index;
                    hasExpDigits = true;
                }
                if (!hasExpDigits) {
                    // Invalid exponent part, rollback
                    m_index = eStart; 
                }
            }

            if (!hasMsg) {
                m_index = start;
                return false;
            }

            std::string numStr = m_data.substr(start, m_index - start);
            // Use QLocale::c() to ensure '.' is always treated as decimal separator
            bool ok = false;
            value = QLocale::c().toDouble(QString::fromStdString(numStr), &ok);
            return ok;
        }

        /**
         * @brief Reads a single flag (0 or 1) for Arc commands.
         * 
         * SVG Arc flags (large-arc and sweep) are strictly '0' or '1'.
         * They must NOT be followed by a comma (though a comma separator is allowed *after* them).
         * Crucially, they do not consume more characters. e.g. "10" is flag '1' then number '0'.
         * 
         * @param value Output parameter (input as boolean)
         * @return true if success
         */
        bool readFlag(bool& value) {
            skipSeparators();
            if (m_index >= m_data.size()) return false;
            
            char c = m_data[m_index];
            if (c == '0') {
                value = false;
                m_index++;
                return true;
            } else if (c == '1') {
                value = true;
                m_index++;
                return true;
            }
            return false;
        }

        /**
         * @brief Reads a point (x, y coordinate pair) from the path data string.
         *
         * Algorithm:
         * 1. Read two numbers (x and y coordinates)
         * 2. If relative mode: add coordinates to current position (m_current)
         * 3. If absolute mode: use coordinates as-is
         * 4. Return false if coordinates cannot be read
         *
         * Relative coordinates are relative to the current pen position, which is useful
         * for creating paths that can be easily translated or scaled.
         *
         * @param out Output parameter for the parsed point
         * @param relative If true, coordinates are relative to current position; if false, absolute
         * @return true if a point was successfully parsed, false otherwise
         */
        bool readPoint(QPointF& out, bool relative)
        {
            double x = 0.0;
            double y = 0.0;
            if (!readNumber(x) || !readNumber(y)) {
                return false;
            }
            if (relative) {
                // Relative coordinates: add to current position
                out.setX(m_current.x() + x);
                out.setY(m_current.y() + y);
            }
            else {
                // Absolute coordinates: use as-is
                out.setX(x);
                out.setY(y);
            }
            return true;
        }

        /**
         * @brief Handles the 'M' (moveto) and 'm' (relative moveto) commands.
         *
         * Algorithm (per SVG specification):
         * 1. First coordinate pair: Moves the pen to that position (starts a new subpath)
         * 2. Subsequent coordinate pairs: Treated as implicit 'L' (lineto) commands
         *    This allows "M 10 20 30 40" to mean: move to (10,20), then line to (30,40)
         *
         * Implementation:
         * - Track if this is the first point (use moveTo) or subsequent (use lineTo)
         * - Store the subpath start point for the 'Z' (closepath) command
         * - Update current position after each point
         * - Continue reading points until a new command is encountered or data ends
         *
         * @param relative If true, coordinates are relative to current position; if false, absolute
         */
        void handleMove(bool relative)
        {
            bool firstPoint = true;
            while (true) {
                QPointF point;
                if (!readPoint(point, relative)) {
                    break; // No more points to read
                }
                if (firstPoint) {
                    // First point: start a new subpath
                    m_path.moveTo(point);
                    m_subpathStart = point; // Store for 'Z' command
                    m_current = point;
                    firstPoint = false;
                    m_prevCommand = 'M';
                }
                else {
                    // Subsequent points: treat as line commands (per SVG spec)
                    m_path.lineTo(point);
                    m_current = point;
                    m_prevCommand = 'L';
                }
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        /**
         * @brief Handles the 'L' (lineto) and 'l' (relative lineto) commands.
         *
         * Algorithm:
         * - Read coordinate pairs (x, y) repeatedly
         * - Draw straight lines from current position to each point
         * - Update current position after each line
         * - Continue until a new command is encountered or data ends
         *
         * SVG allows multiple coordinate pairs after 'L' command:
         * "L 10 20 30 40 50 60" draws lines: current->(10,20)->(30,40)->(50,60)
         *
         * @param relative If true, coordinates are relative to current position; if false, absolute
         */
        void handleLine(bool relative)
        {
            while (true) {
                QPointF point;
                if (!readPoint(point, relative)) {
                    break; // No more points to read
                }
                m_path.lineTo(point);
                m_current = point;
                m_prevCommand = 'L';
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        /**
         * @brief Handles the 'H' (horizontal lineto) and 'h' (relative horizontal lineto) commands.
         *
         * Algorithm:
         * - Draws a horizontal line (only X coordinate changes, Y stays the same)
         * - Reads only X coordinate values (not full points)
         * - If relative: new X = current X + value
         * - If absolute: new X = value
         * - Y coordinate remains unchanged from current position
         * - Continue reading X values until a new command is encountered
         *
         * SVG allows multiple X values after 'H' command:
         * "H 10 20 30" draws horizontal lines: current->(10,y)->(20,y)->(30,y)
         *
         * @param relative If true, X coordinate is relative to current X; if false, absolute
         */
        void handleHorizontal(bool relative)
        {
            while (true) {
                double value = 0.0; // X coordinate value
                if (!readNumber(value)) {
                    break; // No more X values to read
                }
                QPointF point = m_current;
                point.setX(relative ? m_current.x() + value : value);
                // Y coordinate stays the same (horizontal line)
                m_path.lineTo(point);
                m_current = point;
                m_prevCommand = 'L';
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        /**
         * @brief Handles the 'V' (vertical lineto) and 'v' (relative vertical lineto) commands.
         *
         * Algorithm:
         * - Draws a vertical line (only Y coordinate changes, X stays the same)
         * - Reads only Y coordinate values (not full points)
         * - If relative: new Y = current Y + value
         * - If absolute: new Y = value
         * - X coordinate remains unchanged from current position
         * - Continue reading Y values until a new command is encountered
         *
         * SVG allows multiple Y values after 'V' command:
         * "V 10 20 30" draws vertical lines: current->(x,10)->(x,20)->(x,30)
         *
         * @param relative If true, Y coordinate is relative to current Y; if false, absolute
         */
        void handleVertical(bool relative)
        {
            while (true) {
                double value = 0.0; // Y coordinate value
                if (!readNumber(value)) {
                    break; // No more Y values to read
                }
                QPointF point = m_current;
                point.setY(relative ? m_current.y() + value : value);
                // X coordinate stays the same (vertical line)
                m_path.lineTo(point);
                m_current = point;
                m_prevCommand = 'L';
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        /**
         * @brief Handles the 'C' (cubic Bezier) and 'c' (relative cubic Bezier) commands.
         *
         * Algorithm:
         * - Cubic Bezier curves require 3 points: control1, control2, and end point
         * - The curve starts at current position, uses c1 and c2 as control points, ends at 'end'
         * - Formula: B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
         *   where P₀=current, P₁=c1, P₂=c2, P₃=end
         * - Store c2 as lastCubicControl for 'S' (smooth cubic) command
         * - Continue reading curve definitions until a new command is encountered
         *
         * SVG allows multiple curves after 'C' command:
         * "C c1x c1y c2x c2y x y c1x c1y c2x c2y x y" draws two consecutive curves
         *
         * @param relative If true, coordinates are relative to current position; if false, absolute
         */
        void handleCubic(bool relative)
        {
            while (true) {
                QPointF c1;  // First control point
                QPointF c2;  // Second control point
                QPointF end; // End point
                if (!readPoint(c1, relative) || !readPoint(c2, relative) ||
                    !readPoint(end, relative)) {
                    break; // Not enough points for a cubic Bezier
                }
                m_path.cubicTo(c1, c2, end);
                m_lastCubicControl = c2; // Store for smooth cubic ('S') command
                m_current = end;
                m_prevCommand = 'C';
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        /**
         * @brief Handles the 'S' (smooth cubic Bezier) and 's' (relative smooth cubic) commands.
         *
         * Algorithm:
         * - Smooth cubic Bezier is a shorthand for cubic Bezier where the first control point
         *   is automatically calculated to create a smooth curve
         * - If previous command was 'C' or 'S', calculate c1 as reflection of last control point:
         *   c1 = 2*current - lastCubicControl (creates smooth transition)
         * - If previous command was not 'C' or 'S', use current position as c1 (straight line
         * start)
         * - Only need to read c2 (second control) and end point
         * - Store c2 for next smooth cubic command
         *
         * This creates smooth, continuous curves when chaining multiple 'S' commands.
         *
         * @param relative If true, coordinates are relative to current position; if false, absolute
         */
        void handleSmoothCubic(bool relative)
        {
            while (true) {
                QPointF c2;  // Second control point (first is auto-calculated)
                QPointF end; // End point
                if (!readPoint(c2, relative) || !readPoint(end, relative)) {
                    break; // Not enough points
                }
                // Calculate first control point for smooth transition
                QPointF c1 = m_current; // Default: use current position
                if (m_prevCommand == 'C' || m_prevCommand == 'S') {
                    // Reflect the last control point through current position for smoothness
                    c1 = QPointF(2 * m_current.x() - m_lastCubicControl.x(),
                                 2 * m_current.y() - m_lastCubicControl.y());
                }
                m_path.cubicTo(c1, c2, end);
                m_lastCubicControl = c2; // Store for next smooth cubic
                m_current = end;
                m_prevCommand = 'S';
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        /**
         * @brief Handles the 'Q' (quadratic Bezier) and 'q' (relative quadratic Bezier) commands.
         *
         * Algorithm:
         * - Quadratic Bezier curves require 2 points: control point and end point
         * - The curve starts at current position, uses 'control' as control point, ends at 'end'
         * - Formula: B(t) = (1-t)²P₀ + 2(1-t)tP₁ + t²P₂
         *   where P₀=current, P₁=control, P₂=end
         * - Store control point for 'T' (smooth quadratic) command
         * - Continue reading curve definitions until a new command is encountered
         *
         * Quadratic Bezier is simpler than cubic (one control point instead of two) but less
         * flexible.
         *
         * @param relative If true, coordinates are relative to current position; if false, absolute
         */
        void handleQuadratic(bool relative)
        {
            while (true) {
                QPointF control; // Control point
                QPointF end;     // End point
                if (!readPoint(control, relative) || !readPoint(end, relative)) {
                    break; // Not enough points for a quadratic Bezier
                }
                m_path.quadTo(control, end);
                m_lastQuadControl = control; // Store for smooth quadratic ('T') command
                m_current = end;
                m_prevCommand = 'Q';
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        /**
         * @brief Handles the 'T' (smooth quadratic Bezier) and 't' (relative smooth quadratic)
         * commands.
         *
         * Algorithm:
         * - Smooth quadratic Bezier is a shorthand for quadratic Bezier where the control point
         *   is automatically calculated to create a smooth curve
         * - If previous command was 'Q' or 'T', calculate control as reflection of last control
         * point: control = 2*current - lastQuadControl (creates smooth transition)
         * - If previous command was not 'Q' or 'T', use current position as control (straight line)
         * - Only need to read the end point
         * - Store calculated control for next smooth quadratic command
         *
         * This creates smooth, continuous curves when chaining multiple 'T' commands.
         *
         * @param relative If true, coordinates are relative to current position; if false, absolute
         */
        void handleSmoothQuadratic(bool relative)
        {
            while (true) {
                QPointF end; // End point (control is auto-calculated)
                if (!readPoint(end, relative)) {
                    break; // No end point found
                }
                // Calculate control point for smooth transition
                QPointF control = m_current; // Default: use current position
                if (m_prevCommand == 'Q' || m_prevCommand == 'T') {
                    // Reflect the last control point through current position for smoothness
                    control = QPointF(2 * m_current.x() - m_lastQuadControl.x(),
                                      2 * m_current.y() - m_lastQuadControl.y());
                }
                m_path.quadTo(control, end);
                m_lastQuadControl = control; // Store for next smooth quadratic
                m_current = end;
                m_prevCommand = 'T';
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        /**
         * @brief Handles the 'A' (elliptical arc) and 'a' (relative elliptical arc) commands.
         *
         * Algorithm:
         * - Elliptical arc requires 7 parameters:
         *   1. rx: X-radius of the ellipse
         *   2. ry: Y-radius of the ellipse
         *   3. x-axis-rotation: Rotation angle of the ellipse (in degrees)
         *   4. large-arc-flag: 0 or 1, determines which arc to draw (large or small)
         *   5. sweep-flag: 0 or 1, determines direction (clockwise or counterclockwise)
         *   6. x: X coordinate of end point
         *   7. y: Y coordinate of end point
         * - The arc starts at current position and ends at (x, y)
         * - Note: This is a simplified implementation that approximates arcs as lines.
         *   A full implementation would calculate the actual elliptical arc path.
         *
         * SVG allows multiple arcs after 'A' command by repeating the 7 parameters.
         *
         * @param relative If true, end coordinates are relative to current position; if false,
         * absolute
         */
        /**
         * @brief Handles the 'A' (elliptical arc) and 'a' (relative elliptical arc) commands.
         *
         * Algorithm:
         * - Reads rx, ry, rotation, large-arc-flag, sweep-flag, x, y.
         * - Converts Endpoint Parameterization (SVG) -> Center Parameterization.
         * - Approximates the arc using Cubic Bezier curves.
         */
        void handleArc(bool relative)
        {
            while (true) {
                double rx, ry, xAxisRotation;
                bool largeArcFlag, sweepFlag;
                QPointF end;

                if (!readNumber(rx) || !readNumber(ry) || !readNumber(xAxisRotation) ||
                    !readFlag(largeArcFlag) || !readFlag(sweepFlag) ||
                    !readPoint(end, relative)) {
                    break; 
                }

                // Current point (start of arc)
                double x1 = m_current.x();
                double y1 = m_current.y();
                double x2 = end.x();
                double y2 = end.y();

                // 1. Handle degenerate cases
                if (x1 == x2 && y1 == y2) {
                    // End points are same, nothing to draw
                    continue; 
                }
                if (rx == 0.0 || ry == 0.0) {
                    // Zero radius -> treat as straight line
                    m_path.lineTo(end);
                    m_current = end;
                    m_prevCommand = 'A';
                    skipSeparators();
                     if (m_index >= m_data.size() || isCommand(m_data[m_index])) break;
                    continue;
                }

                rx = std::abs(rx);
                ry = std::abs(ry);

                // 2. Convert rotation to radians
                double phi = xAxisRotation * M_PI / 180.0;
                double cosPhi = std::cos(phi);
                double sinPhi = std::sin(phi);

                // 3. Compute (x1', y1')
                double dx = (x1 - x2) / 2.0;
                double dy = (y1 - y2) / 2.0;
                double x1p = cosPhi * dx + sinPhi * dy;
                double y1p = -sinPhi * dx + cosPhi * dy;

                // 4. Adjust radii if needed
                double lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
                if (lambda > 1.0) {
                    double sqrtLambda = std::sqrt(lambda);
                    rx *= sqrtLambda;
                    ry *= sqrtLambda;
                }

                // 5. Compute (cx', cy')
                double numerator = (rx*rx*ry*ry) - (rx*rx*y1p*y1p) - (ry*ry*x1p*x1p);
                if (numerator < 0.0) numerator = 0.0; // Float precision check
                double denominator = (rx*rx*y1p*y1p) + (ry*ry*x1p*x1p);
                
                double coef = std::sqrt(numerator / denominator);
                if (largeArcFlag == sweepFlag) coef = -coef;
                
                double cxp = coef * ((rx * y1p) / ry);
                double cyp = coef * (-(ry * x1p) / rx);

                // 6. Compute Center (cx, cy)
                double cx = cosPhi * cxp - sinPhi * cyp + (x1 + x2) / 2.0;
                double cy = sinPhi * cxp + cosPhi * cyp + (y1 + y2) / 2.0;

                // 7. Compute Start Angle (theta1) and Sweep Angle (dTheta)
                auto angle = [](double ux, double uy, double vx, double vy) {
                    double sign = (ux * vy - uy * vx < 0) ? -1.0 : 1.0;
                    double dot = ux * vx + uy * vy;
                    double mag = std::sqrt(ux*ux + uy*uy) * std::sqrt(vx*vx + vy*vy);
                    double ratio = dot / mag;
                    // clamp ratio
                    if (ratio > 1.0) ratio = 1.0;
                    if (ratio < -1.0) ratio = -1.0;
                    return sign * std::acos(ratio);
                };

                // Vector (1, 0)
                // Vector ( (x1' - cx')/rx, (y1' - cy')/ry )
                double vx1 = (x1p - cxp) / rx;
                double vy1 = (y1p - cyp) / ry;
                // Vector ( (-x1' - cx')/rx, (-y1' - cy')/ry )
                double vx2 = (-x1p - cxp) / rx;
                double vy2 = (-y1p - cyp) / ry;

                double theta1 = angle(1.0, 0.0, vx1, vy1);
                double dTheta = angle(vx1, vy1, vx2, vy2);

                if (!sweepFlag && dTheta > 0) dTheta -= 2.0 * M_PI;
                else if (sweepFlag && dTheta < 0) dTheta += 2.0 * M_PI;

                // 8. Approximate Arc with Cubic Bezier Segments
                // Split into segments <= 90 degrees
                int segments = static_cast<int>(std::ceil(std::abs(dTheta) / (M_PI / 2.0)));
                double delta = dTheta / segments;
                double t = 8.0 / 3.0 * std::sin(delta / 4.0) * std::sin(delta / 4.0) / std::sin(delta / 2.0);

                double theta = theta1;
                
                for (int i = 0; i < segments; ++i) {
                    double cosTheta = std::cos(theta);
                    double sinTheta = std::sin(theta);
                    double thetaNext = theta + delta;
                    double cosThetaNext = std::cos(thetaNext);
                    double sinThetaNext = std::sin(thetaNext);

                    // End point of this segment (on unit circle)
                    double epx = cosThetaNext;
                    double epy = sinThetaNext;

                    // Starting point on unit circle
                    double spx = cosTheta;
                    double spy = sinTheta;

                    // Control points on unit circle
                    double cp1x = spx - t * spy;
                    double cp1y = spy + t * spx;
                    double cp2x = epx + t * epy;
                    double cp2y = epy - t * epx;

                    // Map all points to actual coordinates
                    auto mapPoint = [&](double u, double v) {
                        double rotX = rx * u;
                        double rotY = ry * v;
                        return QPointF(
                            cosPhi * rotX - sinPhi * rotY + cx,
                            sinPhi * rotX + cosPhi * rotY + cy
                        );
                    };

                    QPointF c1 = mapPoint(cp1x, cp1y);
                    QPointF c2 = mapPoint(cp2x, cp2y);
                    QPointF p2 = mapPoint(epx, epy);

                    m_path.cubicTo(c1, c2, p2);
                    theta = thetaNext;
                }

                m_current = end;
                m_prevCommand = 'A';
                skipSeparators();
                if (m_index >= m_data.size() || isCommand(m_data[m_index])) {
                    break;
                }
            }
        }

        void skipUnknown()
        {
            while (m_index < m_data.size() && !isCommand(m_data[m_index])) {
                ++m_index;
            }
        }
    };

    QPainterPath buildPainterPath(const std::string& data)
    {
        SvgPathBuilder builder(data);
        return builder.build();
    }

} // namespace

/**
 * @brief Constructor for QtRenderer - initializes with a QPainter.
 *
 * Algorithm:
 * - Stores a pointer to the QPainter that will be used for drawing
 * - The painter must remain valid for the lifetime of the renderer
 *
 * @param painter Pointer to the QPainter to use for rendering
 */
QtRenderer::QtRenderer(QPainter* painter, const SVGDocument& document) : m_painter(painter), m_document(document) {}

QtRenderer::~QtRenderer() = default;

/**
 * @brief Renders an SVG rectangle element.
 *
 * Algorithm:
 * 1. Check if element should be drawn (has visible fill or stroke)
 * 2. Get rectangle dimensions and corner radii
 * 3. Create QPainterPath:
 *    - If rx or ry > 0: create rounded rectangle
 *    - Otherwise: create regular rectangle
 * 4. Draw the path with style and transform applied
 *
 * @param rect The rectangle element to render
 */
void QtRenderer::visit(SVGRect& rect)
{
    if (!prepareForDrawing(rect.getStyle())) {
        return; // Nothing to draw
    }

    const SVGRectF& r = rect.getRect();
    QPainterPath path;
    if (rect.getRx() > 0.0 || rect.getRy() > 0.0) {
        // Rounded rectangle
        path.addRoundedRect(QRectF(r.x, r.y, r.width, r.height), rect.getRx(), rect.getRy());
    }
    else {
        // Regular rectangle
        path.addRect(QRectF(r.x, r.y, r.width, r.height));
    }
    drawPath(path, rect.getStyle(), rect.getTransform());
}

/**
 * @brief Renders an SVG circle element.
 *
 * Algorithm:
 * 1. Check if element should be drawn (has visible fill or stroke)
 * 2. Get circle center and radius
 * 3. Create QPainterPath as an ellipse with equal radii (circle)
 * 4. Draw the path with style and transform applied
 *
 * Note: A circle is an ellipse with rx = ry = radius
 *
 * @param circle The circle element to render
 */
void QtRenderer::visit(SVGCircle& circle)
{
    if (!prepareForDrawing(circle.getStyle())) {
        return; // Nothing to draw
    }

    SVGPointF center = circle.getCenter();
    QPainterPath path;
    // Create ellipse with equal radii (circle)
    path.addEllipse(QPointF(center.x, center.y), circle.getRadius(), circle.getRadius());
    drawPath(path, circle.getStyle(), circle.getTransform());
}

/**
 * @brief Renders an SVG polygon element.
 *
 * Algorithm:
 * 1. Check if element should be drawn (has visible fill or stroke)
 * 2. Get polygon points
 * 3. Convert SVG points to QPointF and build QPolygonF
 * 4. Create QPainterPath from polygon and close it (connect last to first)
 * 5. Draw the path with style and transform applied
 *
 * Note: Polygon is a closed shape - the last point is automatically connected to the first
 *
 * @param polygon The polygon element to render
 */
void QtRenderer::visit(SVGPolygon& polygon)
{
    if (!prepareForDrawing(polygon.getStyle())) {
        return; // Nothing to draw
    }

    const auto& pts = polygon.getPoints();
    if (pts.empty()) {
        return; // No points to draw
    }

    // Convert SVG points to QPointF
    QPolygonF pathPoints;
    pathPoints.reserve(static_cast<int>(pts.size()));
    for (const auto& p : pts) {
        pathPoints << QPointF(p.x, p.y);
    }

    QPainterPath path;
    path.addPolygon(pathPoints);
    path.closeSubpath(); // Close the polygon (connect last to first)
    drawPath(path, polygon.getStyle(), polygon.getTransform());
}

/**
 * @brief Renders an SVG polyline element.
 *
 * Algorithm:
 * 1. Check if element should be drawn (has visible fill or stroke)
 * 2. Get polyline points (need at least 2 points)
 * 3. Create QPainterPath starting at first point
 * 4. Add lines connecting consecutive points
 * 5. Draw the path with style and transform applied
 *
 * Note: Polyline is an open shape - the last point is NOT connected to the first
 *
 * @param polyline The polyline element to render
 */
void QtRenderer::visit(SVGPolyline& polyline)
{
    if (!prepareForDrawing(polyline.getStyle())) {
        return; // Nothing to draw
    }

    const auto& pts = polyline.getPoints();
    if (pts.size() < 2) {
        return; // Need at least 2 points for a line
    }

    // Start path at first point
    QPainterPath path(QPointF(pts[0].x, pts[0].y));
    // Add lines to subsequent points
    for (size_t i = 1; i < pts.size(); ++i) {
        path.lineTo(QPointF(pts[i].x, pts[i].y));
    }
    drawPath(path, polyline.getStyle(), polyline.getTransform());
}

void QtRenderer::visit(SVGText& text)
{
    if (!prepareForDrawing(text.getStyle()))
        return; // Nothing to draw
    const SVGStyle& style = text.getStyle();

    m_painter->save();

    // 1. Transform
    QTransform localTransform = toQTransform(text.getTransform());
    QTransform currentWorld = m_painter->worldTransform();
    QTransform combined = localTransform * currentWorld;
    m_painter->setWorldTransform(combined);

    // 2. Setup Font
    QFont font;
    font.setFamily(
        QString::fromStdString(style.fontFamily.empty() ? "Times New Roman" : style.fontFamily));

    // Set size
    if (style.fontSize > 0) {
        font.setPixelSize(static_cast<int>(style.fontSize));
    }
    else {
        font.setPixelSize(16);
    }

    // Set Italic (Nghiêng)
    if (style.isItalic) {
        font.setItalic(true);
    }

    // Set Bold (Đậm)
    if (style.isBold) {
        font.setBold(true);
    }

    // 3. Xử lý căn lề (Text Anchor)
    QString qText = QString::fromStdString(text.getText());
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(qText); // Đo chiều dài chữ

    double dx = 0.0;
    if (style.textAnchor == "middle") {
        dx = -textWidth / 2.0; // Dịch sang trái 50%
    }
    else if (style.textAnchor == "end") {
        dx = -textWidth; // Dịch sang trái 100%
    }

    // 4. Vẽ Text dưới dạng Path (Để có cả Fill và Stroke)
    SVGPointF pos = text.getPosition();
    QPainterPath path;
    // Cộng thêm dx vào tọa độ x
    path.addText(pos.x + dx, pos.y, font, qText);

    // 5. Tô màu (Fill)
    if (hasVisibleFill(style)) {
        QColor fillColor = toQColor(style.fillColor, normalizedOpacity(style.fillOpacity));
        m_painter->fillPath(path, QBrush(fillColor));
    }

    // 6. Vẽ viền (Stroke)
    if (hasVisibleStroke(style)) {
        QColor strokeColor = toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity));
        QPen pen(strokeColor);
        pen.setWidthF(style.strokeWidth > 0 ? style.strokeWidth : 1.0);
        m_painter->strokePath(path, pen);
    }

    m_painter->restore();
}

/**
 * @brief Renders an SVG ellipse element.
 *
 * Algorithm:
 * 1. Check if element should be drawn (has visible fill or stroke)
 * 2. Get ellipse center and radii (rx, ry)
 * 3. Create QPainterPath as an ellipse centered at (cx, cy) with radii (rx, ry)
 * 4. Draw the path with style and transform applied
 *
 * Note: If rx == ry, the ellipse is a circle
 *
 * @param ellipse The ellipse element to render
 */
void QtRenderer::visit(SVGEllipse& ellipse)
{
    if (!prepareForDrawing(ellipse.getStyle())) {
        return; // Nothing to draw
    }

    SVGPointF center = ellipse.getCenter();
    QPainterPath path;
    // Create ellipse with different radii (rx, ry)
    path.addEllipse(QPointF(center.x, center.y), ellipse.getRx(), ellipse.getRy());
    drawPath(path, ellipse.getStyle(), ellipse.getTransform());
}

/**
 * @brief Renders an SVG line element.
 *
 * Algorithm:
 * 1. Check if painter is valid
 * 2. Check if element should be drawn (has visible stroke - lines don't have fill)
 * 3. Save painter state
 * 4. Apply element's transform to painter
 * 5. Create pen with stroke color, width, and style
 * 6. Draw line from p1 to p2
 * 7. Restore painter state
 *
 * Note: Lines only have stroke, no fill. Stroke width must be > 0.
 *
 * @param line The line element to render
 */
void QtRenderer::visit(SVGLine& line)
{
    if (!m_painter) {
        return;
    }
    const SVGStyle& style = line.getStyle();
    if (!prepareForDrawing(style)) {
        return; // Nothing to draw
    }
    if (!hasVisibleStroke(style)) {
        return; // Lines need visible stroke
    }

    m_painter->save();
    QTransform localTransform = toQTransform(line.getTransform());
    QTransform currentWorld = m_painter->worldTransform();

    m_painter->setWorldTransform(localTransform * currentWorld);

    // Create pen for stroke
    QPen pen(toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity)));
    pen.setWidthF(style.strokeWidth > 0.0 ? style.strokeWidth : 1.0);
    pen.setJoinStyle(Qt::MiterJoin); // Sharp corners
    pen.setCapStyle(Qt::FlatCap);    // Flat line caps
    m_painter->setPen(pen);
    m_painter->setBrush(Qt::NoBrush); // Lines don't have fill

    // Draw the line
    SVGPointF p1 = line.getP1();
    SVGPointF p2 = line.getP2();
    m_painter->drawLine(QPointF(p1.x, p1.y), QPointF(p2.x, p2.y));
    m_painter->restore(); // Restore state
}
/**
 * @brief Renders an SVG path element.
 *
 * Algorithm:
 * 1. Check if element should be drawn (has visible fill or stroke)
 * 2. Get path data string (the 'd' attribute)
 * 3. Parse path data and build QPainterPath using SvgPathBuilder
 * 4. Draw the path with style and transform applied
 *
 * Path data parsing:
 * - Parses SVG path commands (M, L, C, S, Q, T, A, Z, etc.)
 * - Handles both absolute (uppercase) and relative (lowercase) commands
 * - Builds a QPainterPath representing the complete path
 *
 * @param svgPath The path element to render
 */
void QtRenderer::visit(SVGPath& svgPath)
{
    if (!prepareForDrawing(svgPath.getStyle())) {
        return;
    }

    const auto& commands = svgPath.getCommands();
    if (commands.empty())
        return;

    QPainterPath path;

    QPointF currentPos(0, 0);
    QPointF subPathStart(0, 0);

    QPointF lastControlPoint(0, 0);
    char lastCmd = '\0';

    for (const auto& cmd : commands) {
        bool isRelative = islower(cmd.type);
        char type = tolower(cmd.type);
        const auto& args = cmd.args;

        switch (type) {
        case 'm': { // MoveTo (x, y)
            double x = args[0];
            double y = args[1];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }
            path.moveTo(x, y);
            currentPos = QPointF(x, y);
            subPathStart = currentPos; // Store subpath start
            lastControlPoint = currentPos;
            break;
        }
        case 'l': { // LineTo (x, y)
            double x = args[0];
            double y = args[1];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }
            path.lineTo(x, y);
            currentPos = QPointF(x, y);
            break;
        }
        case 'h': { // Horizontal LineTo (x)
            double x = args[0];
            if (isRelative)
                x += currentPos.x();
            path.lineTo(x, currentPos.y());
            currentPos.setX(x);
            break;
        }
        case 'v': { // Vertical LineTo (y)
            double y = args[0];
            if (isRelative)
                y += currentPos.y();
            path.lineTo(currentPos.x(), y);
            currentPos.setY(y);
            break;
        }
        case 'c': { // Cubic Bezier (x1, y1, x2, y2, x, y)
            double x1 = args[0], y1 = args[1];
            double x2 = args[2], y2 = args[3];
            double x = args[4], y = args[5];

            if (isRelative) {
                x1 += currentPos.x();
                y1 += currentPos.y();
                x2 += currentPos.x();
                y2 += currentPos.y();
                x += currentPos.x();
                y += currentPos.y();
            }
            path.cubicTo(x1, y1, x2, y2, x, y);
            lastControlPoint = QPointF(x2, y2); // Lưu điểm điều khiển thứ 2
            currentPos = QPointF(x, y);
            break;
        }
        case 's': { // Smooth Cubic (x2, y2, x, y)
            // Điểm điều khiển 1 là điểm phản chiếu của lastControlPoint qua currentPos
            double x1, y1;
            if (lastCmd == 'c' || lastCmd == 's') {
                x1 = 2 * currentPos.x() - lastControlPoint.x();
                y1 = 2 * currentPos.y() - lastControlPoint.y();
            }
            else {
                x1 = currentPos.x();
                y1 = currentPos.y();
            }

            double x2 = args[0], y2 = args[1];
            double x = args[2], y = args[3];

            if (isRelative) {
                x2 += currentPos.x();
                y2 += currentPos.y();
                x += currentPos.x();
                y += currentPos.y();
            }

            path.cubicTo(x1, y1, x2, y2, x, y);
            lastControlPoint = QPointF(x2, y2);
            currentPos = QPointF(x, y);
            break;
        }
        case 'z': { // ClosePath
            path.closeSubpath();
            // Fix: update currentPos to subpath start (SVG Spec) so subsequent relative moves work correctly.
            currentPos = subPathStart;
            lastControlPoint = currentPos;
            break;
        }
        case 'q': { // Quadratic Bezier (x1, y1, x, y)
            // Fix: Implement 'q' command. Previously missing, causing gaps in paths (e.g. KHTN text letters).
            // Without this, currentPos is not updated, causing subsequent relative commands to draw from wrong position.
            double x1 = args[0], y1 = args[1];
            double x = args[2], y = args[3];

            if (isRelative) {
                x1 += currentPos.x();
                y1 += currentPos.y();
                x += currentPos.x();
                y += currentPos.y();
            }
            path.quadTo(x1, y1, x, y);
            lastControlPoint = QPointF(x1, y1);
            currentPos = QPointF(x, y);
            break;
        }
        case 't': { // Smooth Quadratic (x, y)
             // Fix: Implement 't' command for smooth quadratic curves.
             // Auto-calculates control point by reflecting previous control point.
            double x1, y1;
            if (lastCmd == 'q' || lastCmd == 't') {
                x1 = 2 * currentPos.x() - lastControlPoint.x();
                y1 = 2 * currentPos.y() - lastControlPoint.y();
            }
            else {
                x1 = currentPos.x();
                y1 = currentPos.y();
            }

            double x = args[0], y = args[1];
            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }

            path.quadTo(x1, y1, x, y);
            lastControlPoint = QPointF(x1, y1); // Store calculated control point
            currentPos = QPointF(x, y);
            break;
        }
        case 'a': { // Arc (rx, ry, rot, large, sweep, x, y)
            // Fix: Implement 'a' command. Previously missing.
            // Converts SVG Arc (Endpoint Parameterization) to Cubic Beziers (Center Parameterization).
            double rx = std::abs(args[0]);
            double ry = std::abs(args[1]);
            double xAxisRotation = args[2];
            bool largeArcFlag = (args[3] != 0.0);
            bool sweepFlag = (args[4] != 0.0);
            double x = args[5], y = args[6];

            if (isRelative) {
                x += currentPos.x();
                y += currentPos.y();
            }

            double x1 = currentPos.x();
            double y1 = currentPos.y();
            double x2 = x;
            double y2 = y;

            if (x1 == x2 && y1 == y2) {
                // End points are same, nothing to draw
            }
            else if (rx == 0.0 || ry == 0.0) {
                // Zero radius -> treat as straight line
                path.lineTo(x, y);
            }
            else {
                 // 2. Convert rotation to radians
                double phi = xAxisRotation * M_PI / 180.0;
                double cosPhi = std::cos(phi);
                double sinPhi = std::sin(phi);

                // 3. Compute (x1', y1')
                double dx = (x1 - x2) / 2.0;
                double dy = (y1 - y2) / 2.0;
                double x1p = cosPhi * dx + sinPhi * dy;
                double y1p = -sinPhi * dx + cosPhi * dy;

                // 4. Adjust radii if needed
                double lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
                if (lambda > 1.0) {
                    double sqrtLambda = std::sqrt(lambda);
                    rx *= sqrtLambda;
                    ry *= sqrtLambda;
                }

                // 5. Compute (cx', cy')
                double numerator = (rx * rx * ry * ry) - (rx * rx * y1p * y1p) - (ry * ry * x1p * x1p);
                if (numerator < 0.0) numerator = 0.0;
                double denominator = (rx * rx * y1p * y1p) + (ry * ry * x1p * x1p);

                double coef = std::sqrt(numerator / denominator);
                if (largeArcFlag == sweepFlag)
                    coef = -coef;

                double cxp = coef * ((rx * y1p) / ry);
                double cyp = coef * (-(ry * x1p) / rx);

                // 6. Compute Center (cx, cy)
                double cx = cosPhi * cxp - sinPhi * cyp + (x1 + x2) / 2.0;
                double cy = sinPhi * cxp + cosPhi * cyp + (y1 + y2) / 2.0;

                // 7. Compute Start Angle (theta1) and Sweep Angle (dTheta)
                auto angle = [](double ux, double uy, double vx, double vy) {
                    double sign = (ux * vy - uy * vx < 0) ? -1.0 : 1.0;
                    double dot = ux * vx + uy * vy;
                    double mag = std::sqrt(ux * ux + uy * uy) * std::sqrt(vx * vx + vy * vy);
                    double ratio = dot / mag;
                    if (ratio > 1.0) ratio = 1.0;
                    if (ratio < -1.0) ratio = -1.0;
                    return sign * std::acos(ratio);
                };

                double vx1 = (x1p - cxp) / rx;
                double vy1 = (y1p - cyp) / ry;
                double vx2 = (-x1p - cxp) / rx;
                double vy2 = (-y1p - cyp) / ry;

                double theta1 = angle(1.0, 0.0, vx1, vy1);
                double dTheta = angle(vx1, vy1, vx2, vy2);

                if (!sweepFlag && dTheta > 0) dTheta -= 2.0 * M_PI;
                else if (sweepFlag && dTheta < 0) dTheta += 2.0 * M_PI;

                // 8. Approximate Arc with Cubic Bezier Segments
                int segments = static_cast<int>(std::ceil(std::abs(dTheta) / (M_PI / 2.0)));
                double delta = dTheta / segments;
                double t = 8.0 / 3.0 * std::sin(delta / 4.0) * std::sin(delta / 4.0) / std::sin(delta / 2.0);

                double theta = theta1;

                for (int i = 0; i < segments; ++i) {
                    double cosTheta = std::cos(theta);
                    double sinTheta = std::sin(theta);
                    double thetaNext = theta + delta;
                    double cosThetaNext = std::cos(thetaNext);
                    double sinThetaNext = std::sin(thetaNext);

                    double epx = cosThetaNext;
                    double epy = sinThetaNext;

                    double spx = cosTheta;
                    double spy = sinTheta;

                    double cp1x = spx - t * spy;
                    double cp1y = spy + t * spx;
                    double cp2x = epx + t * epy;
                    double cp2y = epy - t * epx;

                    auto mapPoint = [&](double u, double v) {
                        double rotX = rx * u;
                        double rotY = ry * v;
                        return QPointF(
                            cosPhi * rotX - sinPhi * rotY + cx,
                            sinPhi * rotX + cosPhi * rotY + cy
                        );
                    };

                    QPointF c1 = mapPoint(cp1x, cp1y);
                    QPointF c2 = mapPoint(cp2x, cp2y);
                    QPointF p2 = mapPoint(epx, epy);

                    path.cubicTo(c1, c2, p2);
                    theta = thetaNext;
                }
            }
            currentPos = QPointF(x, y);
            lastControlPoint = currentPos;
            break;
        }
    }

        // Cập nhật lastCmd để dùng cho logic Smooth Curve
        // Fix: Always update lastCmd. Even 'm' should be tracked so that subsequent 's' or 't'
        // know that the previous command was NOT a curve (and thus won't reflect a control point).
        lastCmd = type;
    }

    // Gọi hàm vẽ chung
    drawPath(path, svgPath.getStyle(), svgPath.getTransform());
}

void QtRenderer::visitGroupBegin(SVGGroup& group)
{
    if (!m_painter) {
        return;
    }
    m_painter->save();

    SVGTransform groupTransform = group.getTransform();

    QTransform localTransform = toQTransform(groupTransform);
    QTransform currentWorld = m_painter->worldTransform();

    QTransform combined = localTransform * currentWorld;

    m_painter->setWorldTransform(combined);
}

void QtRenderer::visitGroupEnd(SVGGroup& group)
{
    Q_UNUSED(group);
    if (!m_painter) {
        return;
    }

    m_painter->restore();
}

/**
 * @brief Checks if an element should be drawn based on its style.
 *
 * Algorithm:
 * - Element should be drawn if:
 *   1. Painter is valid
 *   2. Element is displayed (style.isDisplayed == true)
 *   3. Element has visible fill OR visible stroke
 *
 * This is used to skip rendering elements that are hidden or have no visible content.
 *
 * @param style The style to check
 * @return true if element should be drawn, false otherwise
 */
bool QtRenderer::prepareForDrawing(const SVGStyle& style) const
{
    if (!m_painter) {
        return false; // No painter available
    }
    if (!style.isDisplayed) {
        return false; // Element is hidden (display="none")
    }
    return hasVisibleFill(style) || hasVisibleStroke(style); // Must have something to draw
}

/**
 * @brief Draws a QPainterPath with the given style and transform.
 *
 * Algorithm:
 * 1. Save painter state (for restoration after drawing)
 * 2. Apply element's transform to the painter's world transform
 * 3. Create brush for fill (if fill is visible)
 * 4. Create pen for stroke (if stroke is visible)
 * 5. Set fill rule (winding or even-odd)
 * 6. Draw the path
 * 7. Restore painter state
 *
 * Transform application:
 * - Combines the element's transform with the current painter transform
 * - This allows nested transforms (e.g., group transform + element transform)
 *
 * Style application:
 * - Fill: Applied if fillColor is not "none" and fillOpacity > 0
 * - Stroke: Applied if strokeColor is not "none", strokeWidth > 0, and strokeOpacity > 0
 * - Fill rule: Determines how overlapping paths are filled (winding or even-odd)
 *
 * @param path The QPainterPath to draw
 * @param style The style to apply (fill, stroke, opacity, etc.)
 * @param transform The transform to apply to the path
 */
void QtRenderer::drawPath(const QPainterPath& path, const SVGStyle& style,
                          const SVGTransform& transform)
{
    if (!m_painter) {
        return;
    }

    m_painter->save();
    QTransform localTransform = toQTransform(transform);
    QTransform currentWorld = m_painter->worldTransform();

    // Đảo ngược thứ tự: Local * World
    QTransform combined = localTransform * currentWorld;
    m_painter->setWorldTransform(combined);

    // Calculate BBox for gradient (relative to untransformed path)
    SVGRectF bbox = {0,0,0,0};
    QRectF r = path.boundingRect();
    bbox.x = r.x();
    bbox.y = r.y();
    bbox.width = r.width();
    bbox.height = r.height();

    // Create brush and pen using helpers
    QBrush brush = getBrush(style, bbox);
    QPen pen = getPen(style, bbox);

    m_painter->setBrush(brush);
    m_painter->setPen(pen);

    // Apply fill rule and draw
    QPainterPath pathCopy(path);
    pathCopy.setFillRule(toQtFillRule(style.fillRule));
    m_painter->drawPath(pathCopy);
    m_painter->restore(); // Restore state
}

// --- Helper Methods ---

QBrush QtRenderer::createBrushFromGradient(SVGElement* gradientElement, const SVGRectF& bbox, double opacity)
{
    SVGGradient* svgGrad = dynamic_cast<SVGGradient*>(gradientElement);
    if (!svgGrad) return QBrush(Qt::NoBrush);

    QGradient* qGrad = nullptr;
    
    // Determine coordinate system
    bool userSpace = (svgGrad->gradientUnits == SVGGradientUnits::UserSpaceOnUse);

    if (auto* linear = dynamic_cast<SVGLinearGradient*>(svgGrad)) {
        double x1 = linear->x1;
        double y1 = linear->y1;
        double x2 = linear->x2;
        double y2 = linear->y2;
        
        // Use raw coordinates (0..1 for bbox, or user units)
        qGrad = new QLinearGradient(x1, y1, x2, y2);
    }
    else if (auto* radial = dynamic_cast<SVGRadialGradient*>(svgGrad)) {
        double cx = radial->cx;
        double cy = radial->cy;
        double r = radial->r;
        double fx = radial->fx;
        double fy = radial->fy;
        
        // Use raw coordinates
        qGrad = new QRadialGradient(cx, cy, r, fx, fy);
    }

    if (!qGrad) return QBrush(Qt::NoBrush);

    // Set coordinate mode (Logical mode for brush, but QGradient modes are for QPainter usage directly? 
    // Actually QBrush takes QGradient. QBrush transform handles the mapping.
    // We treat the gradient as being in its local definition space.)
    // But QLinearGradient assumes Logical Coordinates.
    
    switch(svgGrad->spreadMethod) {
        case SVGSpreadMethod::Pad: qGrad->setSpread(QGradient::PadSpread); break;
        case SVGSpreadMethod::Reflect: qGrad->setSpread(QGradient::ReflectSpread); break;
        case SVGSpreadMethod::Repeat: qGrad->setSpread(QGradient::RepeatSpread); break;
    }

    for (const auto& stop : svgGrad->stops) {
        QColor color = toQColor(stop.stopColor, normalizedOpacity(stop.stopOpacity) * opacity);
        qGrad->setColorAt(stop.offset, color);
        
        // std::cout << "DEBUG: Render Gradient Stop: Offset=" << stop.offset 
        //           << " Color=" << color.name().toStdString() << " Alpha=" << color.alpha() << std::endl;
    }
    
    QBrush brush(*qGrad);
    delete qGrad;
    
    // Calculate final transform
    // Final = GradientTransform * (BBoxMapping if applicable)
    // Order applied to point P: P_user = M * P_grad
    // If objectBoundingBox: P_user = BBoxMatrix * GradMatrix * P_grad
    // Wait. GradMatrix applies first in the chain (local deformation of unit square).
    // So QTransform Combined = GradMatrix * BBoxMatrix.
    
    QTransform gradTransform = toQTransform(svgGrad->getTransform());
    
    if (userSpace) {
        brush.setTransform(gradTransform);
    } else {
        // Create BBox mapping: Map Unit Square (0..1) to BBox (x,y, w,h)
        // Operation: Scale(w,h) then Translate(x,y).
        // Qt Matrix Multiplication Order: M1 * M2 = apply M1 then M2? 
        // Wait, earlier I concluded: QTransform().translate.scale -> T * S.
        // And vector v * T * S -> (v+d)*s. This is Trans then Scale. WRONG for BBox.
        // We want Scale then Trans.
        // So QTransform().translate(x,y).scale(w,h) ? 
        // -> Matrix is Translate * Scale.
        // v * T * S -> (v*T) * S ?? No.
        // If I use the builder: t.translate(tx, ty).scale(sx, sy).
        // It produces a matrix M = T * S (if T is left).
        // Qt uses Row Vectors. v' = v * M.
        // v' = v * (T * S) = (v * T) * S.
        // v * T -> (x+tx, y+ty).
        // Result * S -> ((x+tx)*sx, (y+ty)*sy).
        // This is Translate THEN Scale.
        
        // We want Scale THEN Translate.
        // v * S -> (x*sx, y*sy).
        // Result * T -> (x*sx + tx, y*sy + ty).
        // So we need Matrix M = S * T.
        // Builder: t.scale(sx,sy).translate(tx,ty).
        
        // Create BBox mapping: Map Unit Square (0..1) to BBox (x,y, w,h)
        // Operation: Scale(w,h) then Translate(x,y).
        
        QTransform bboxTransform(bbox.width, 0, 0, bbox.height, bbox.x, bbox.y);
        
        // Combined = Grad * BBox.
        // Apply Grad deformation, then Map to BBox.
        // v' = v * Grad * BBox.
        
        brush.setTransform(gradTransform * bboxTransform);
    }
    
    return brush;
}

QBrush QtRenderer::getBrush(const SVGStyle& style, const SVGRectF& bbox)
{
    if (!style.fillUrl.empty()) {
        std::string id = style.fillUrl;
        if (id.size() > 5 && id.substr(0,4) == "url(") {
            id = id.substr(4, id.size() - 5);
             if (!id.empty() && id.front() == '#') id = id.substr(1);
        } else if (!id.empty() && id.front() == '#') {
             id = id.substr(1);
        }
        
        if (!id.empty() && id.back() == ')') id.pop_back();

        SVGElement* elem = m_document.getElementById(id);
        if (elem) {
             std::cout << "DEBUG: Found Gradient ID=" << id << " for Fill" << std::endl;
             QBrush brush = createBrushFromGradient(elem, bbox, normalizedOpacity(style.fillOpacity));
             if (brush.style() != Qt::NoBrush) {
                 return brush;
             } else {
                 std::cout << "DEBUG: Failed to create brush from Gradient ID=" << id << std::endl;
             }
        } else {
            std::cout << "DEBUG: Gradient ID=" << id << " NOT FOUND!" << std::endl;
        }
    }

    if (hasVisibleFill(style)) {
        QColor fillColor = toQColor(style.fillColor, normalizedOpacity(style.fillOpacity));
        return QBrush(fillColor);
    }
    return QBrush(Qt::NoBrush);
}

QPen QtRenderer::getPen(const SVGStyle& style, const SVGRectF& bbox)
{
    if (!style.strokeUrl.empty()) {
         std::string id = style.strokeUrl;
         if (id.size() > 5 && id.substr(0,4) == "url(") {
            id = id.substr(4, id.size() - 5);
             if (!id.empty() && id.front() == '#') id = id.substr(1);
        } else if (!id.empty() && id.front() == '#') {
             id = id.substr(1);
        }
        if (!id.empty() && id.back() == ')') id.pop_back();

        SVGElement* elem = m_document.getElementById(id);
        if (elem) {
             QBrush brush = createBrushFromGradient(elem, bbox, normalizedOpacity(style.strokeOpacity));
             if (brush.style() != Qt::NoBrush) {
                 QPen pen(brush, style.strokeWidth);
                  pen.setJoinStyle(Qt::MiterJoin);
                  pen.setCapStyle(Qt::FlatCap);
                 return pen;
             }
        }
    }
    
    if (hasVisibleStroke(style)) {
        QColor strokeColor = toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity));
        QPen pen(strokeColor, style.strokeWidth > 0.0 ? style.strokeWidth : 1.0);
        pen.setJoinStyle(Qt::MiterJoin);
        pen.setCapStyle(Qt::FlatCap);
        return pen;
    }
    return QPen(Qt::NoPen);
}
