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

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <vector>

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

        return QTransform(m11, m12, 0.0, m21, m22, 0.0, dx, dy, 1.0);
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
        if (style.fillColor.isNone) {
            return false;
        }
        return normalizedOpacity(style.fillOpacity) > 0.0;
    }

    bool hasVisibleStroke(const SVGStyle& style)
    {
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
        bool readNumber(double& value)
        {
            skipSeparators();
            if (m_index >= m_data.size()) {
                return false;
            }
            const char* start = m_data.c_str() + m_index;
            char* end = nullptr;
            value = std::strtod(start, &end);
            if (start == end) {
                return false; // No number found
            }
            m_index = static_cast<size_t>(end - m_data.c_str());
            return true;
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
        void handleArc(bool relative)
        {
            constexpr int paramsPerArc = 7; // Arc requires 7 parameters
            while (true) {
                double values[paramsPerArc] = {};
                bool ok = true;
                // Read all 7 parameters for the arc
                for (int i = 0; i < paramsPerArc; ++i) {
                    ok &= readNumber(values[i]);
                }
                if (!ok) {
                    break; // Not enough parameters for an arc
                }
                // Extract end point (last two parameters)
                QPointF end(values[5], values[6]);
                if (relative) {
                    end.setX(m_current.x() + end.x());
                    end.setY(m_current.y() + end.y());
                }
                // Simplified: draw a line to the end point
                // Full implementation would calculate the actual elliptical arc
                m_path.lineTo(end);
                m_current = end;
                m_prevCommand = 'A';
                skipSeparators();
                // Stop if we've reached the end or encountered a new command
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
QtRenderer::QtRenderer(QPainter* painter) : m_painter(painter) {}

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

/**
 * @brief Renders an SVG text element.
 *
 * Algorithm:
 * 1. Check if element should be drawn (has visible fill or stroke)
 * 2. Get font properties from style (family, size)
 * 3. Create QFont with properties (default to "Times New Roman" if no family)
 * 4. Convert text to QPainterPath using addText (creates vector outline)
 * 5. Draw the path with style and transform applied
 *
 * Why convert text to path?
 * - Text as path ensures it scales and transforms correctly with other shapes
 * - Text position (x, y) is the baseline start position
 * - The path represents the text as vector outlines, not rasterized text
 *
 * @param text The text element to render
 */
void QtRenderer::visit(SVGText& text)
{
    if (!prepareForDrawing(text.getStyle()))
        return; // Nothing to draw
    const SVGStyle& style = text.getStyle();

    // Create font from style
    QFont font;
    if (!style.fontFamily.empty())
        font.setFamily(QString::fromStdString(style.fontFamily));
    else
        font.setFamily(QStringLiteral("Times New Roman")); // Default font
    font.setPointSizeF(style.fontSize > 0 ? style.fontSize : 16.0);

    // Convert text to path (vector outline)
    SVGPointF pos = text.getPosition();
    QPainterPath path;
    path.addText(QPointF(pos.x, pos.y), font, QString::fromStdString(text.getText()));
    drawPath(path, style, text.getTransform());
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

    m_painter->save(); // Save current state
    // Apply element's transform
    QTransform transform = m_painter->worldTransform();
    transform *= toQTransform(line.getTransform());
    m_painter->setWorldTransform(transform);

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
 * @param path The path element to render
 */
void QtRenderer::visit(SVGPath& path)
{
    if (!prepareForDrawing(path.getStyle())) {
        return; // Nothing to draw
    }

    const auto& d = path.getPath();
    if (d.empty()) {
        return; // No path data
    }

    // Parse path data and build QPainterPath
    QPainterPath qPath = buildPainterPath(d);
    if (qPath.isEmpty()) {
        return; // Path parsing failed or empty
    }
    drawPath(qPath, path.getStyle(), path.getTransform());
}

/**
 * @brief Called when entering a group element (Visitor pattern).
 *
 * Algorithm:
 * - Currently does nothing because transforms are accumulated during parsing
 * - Each child element already has the group's transform applied
 * - This avoids double-applying transforms
 *
 * Note: If transforms were not accumulated during parsing, we would:
 * 1. Save painter state
 * 2. Apply group's transform
 * 3. Render children (they would inherit the transform)
 * 4. Restore painter state in visitGroupEnd
 *
 * @param group The group element being entered
 */
void QtRenderer::visitGroupBegin(SVGGroup& group)
{
    Q_UNUSED(group);
    // Note: Transforms are already accumulated during parsing,
    // so each child element already has the group's transform.
    // We don't need to apply it here to avoid double-application.
    // However, we might need to save/restore for style inheritance.
    // For now, we do nothing since transforms are accumulated.
}

/**
 * @brief Called when leaving a group element (Visitor pattern).
 *
 * Algorithm:
 * - Currently does nothing (see visitGroupBegin for explanation)
 * - If transforms were applied in visitGroupBegin, we would restore painter state here
 *
 * @param group The group element being left
 */
void QtRenderer::visitGroupEnd(SVGGroup& group)
{
    Q_UNUSED(group);
    // See visitGroupBegin - no action needed
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

    m_painter->save(); // Save current state
    // Apply element's transform
    QTransform combined = m_painter->worldTransform();
    combined *= toQTransform(transform);
    m_painter->setWorldTransform(combined);

    // Create brush for fill
    QBrush brush(Qt::NoBrush);
    if (hasVisibleFill(style)) {
        QColor fillColor = toQColor(style.fillColor, normalizedOpacity(style.fillOpacity));
        brush = QBrush(fillColor);
    }

    // Create pen for stroke
    QPen pen(Qt::NoPen);
    if (hasVisibleStroke(style)) {
        QColor strokeColor = toQColor(style.strokeColor, normalizedOpacity(style.strokeOpacity));
        pen = QPen(strokeColor, style.strokeWidth > 0.0 ? style.strokeWidth : 1.0);
        pen.setJoinStyle(Qt::MiterJoin); // Sharp corners
        pen.setCapStyle(Qt::FlatCap);    // Flat line caps
    }

    m_painter->setBrush(brush);
    m_painter->setPen(pen);

    // Apply fill rule and draw
    QPainterPath pathCopy(path);
    pathCopy.setFillRule(toQtFillRule(style.fillRule));
    m_painter->drawPath(pathCopy);
    m_painter->restore(); // Restore state
}
