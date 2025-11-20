#include "SVGTransform.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief Constructor for SVG transform - initializes to identity matrix.
 *
 * Algorithm:
 * - Creates a 3x3 homogeneous transformation matrix
 * - Initializes all elements to 0, then sets diagonal to 1 (identity matrix)
 * - Identity matrix means no transformation (point maps to itself)
 *
 * Matrix format (homogeneous coordinates):
 * [m[0][0]  m[0][1]  m[0][2]]   [a  b  tx]
 * [m[1][0]  m[1][1]  m[1][2]] = [c  d  ty]
 * [m[2][0]  m[2][1]  m[2][2]]   [0  0  1 ]
 *
 * Identity matrix:
 * [1  0  0]
 * [0  1  0]
 * [0  0  1]
 */
SVGTransform::SVGTransform()
{

    std::memset(m_matrix, 0, sizeof(m_matrix));

    m_matrix[0][0] = 1.0;
    m_matrix[1][1] = 1.0;
    m_matrix[2][2] = 1.0;
}

/**
 * @brief Multiplies this transform by another transform (matrix multiplication).
 *
 * Algorithm (Matrix Multiplication):
 * - Performs matrix multiplication: this = this * other
 * - Formula: result[i][j] = sum over k of (this[i][k] * other[k][j])
 * - This combines two transformations into one
 *
 * Order matters:
 * - this * other means: apply 'other' first, then 'this'
 * - For SVG: transforms are applied left-to-right, so we multiply left-to-right
 * - Example: translate(10,0) * rotate(45) means: rotate first, then translate
 *
 * Matrix multiplication formula:
 * result[i][j] = Σ(k=0 to 2) m_matrix[i][k] * other.m_matrix[k][j]
 *
 * @param other The transform to multiply with (applied first)
 */
void SVGTransform::multiply(const SVGTransform& other)
{

    SVGNumber result[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};

    // Standard matrix multiplication: result = this * other
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {

            for (int k = 0; k < 3; ++k) {
                result[i][j] += m_matrix[i][k] * other.m_matrix[k][j];
            }
        }
    }

    std::memcpy(m_matrix, result, sizeof(m_matrix));
}

/**
 * @brief Applies a translation (shift) transformation.
 *
 * Algorithm:
 * - Creates a translation matrix and multiplies it with the current transform
 * - Translation matrix:
 *   [1  0  tx]
 *   [0  1  ty]
 *   [0  0  1 ]
 * - This shifts all points by (tx, ty)
 *
 * Formula:
 * - newX = oldX + tx
 * - newY = oldY + ty
 *
 * @param tx Translation amount in X direction
 * @param ty Translation amount in Y direction
 */
void SVGTransform::translate(SVGNumber tx, SVGNumber ty)
{

    SVGTransform translateMatrix;
    translateMatrix.m_matrix[0][2] = tx; // Translation X
    translateMatrix.m_matrix[1][2] = ty; // Translation Y
    // Other elements are already identity (from constructor)

    this->multiply(translateMatrix);
}

/**
 * @brief Applies a scaling transformation.
 *
 * Algorithm:
 * - Creates a scale matrix and multiplies it with the current transform
 * - Scale matrix:
 *   [sx  0   0]
 *   [0   sy  0]
 *   [0   0   1]
 * - This scales all points by (sx, sy)
 *
 * Formula:
 * - newX = oldX * sx
 * - newY = oldY * sy
 *
 * Notes:
 * - sx > 1: enlarge horizontally, sx < 1: shrink horizontally
 * - sy > 1: enlarge vertically, sy < 1: shrink vertically
 * - sx = sy: uniform scaling (preserves aspect ratio)
 * - sx = -1, sy = 1: flip horizontally
 * - sx = 1, sy = -1: flip vertically
 *
 * @param sx Scale factor in X direction
 * @param sy Scale factor in Y direction
 */
void SVGTransform::scale(SVGNumber sx, SVGNumber sy)
{

    SVGTransform scaleMatrix;
    scaleMatrix.m_matrix[0][0] = sx; // Scale X
    scaleMatrix.m_matrix[1][1] = sy; // Scale Y
    // Other elements are already identity (from constructor)

    this->multiply(scaleMatrix);
}

/**
 * @brief Applies a rotation transformation (counterclockwise around origin).
 *
 * Algorithm:
 * - Converts angle from degrees to radians
 * - Creates a rotation matrix using cosine and sine
 * - Rotation matrix (counterclockwise):
 *   [cos(θ)  -sin(θ)  0]
 *   [sin(θ)   cos(θ)  0]
 *   [0        0       1]
 *
 * Formula:
 * - rad = angle * π / 180
 * - newX = oldX * cos(θ) - oldY * sin(θ)
 * - newY = oldX * sin(θ) + oldY * cos(θ)
 *
 * Rotation is counterclockwise around the origin (0, 0).
 * To rotate around a different point, combine with translate operations.
 *
 * @param angle Rotation angle in degrees (positive = counterclockwise)
 */
void SVGTransform::rotate(SVGNumber angle)
{

    // Convert degrees to radians
    SVGNumber rad = angle * M_PI / 180.0;
    SVGNumber c = std::cos(rad); // Cosine
    SVGNumber s = std::sin(rad); // Sine

    SVGTransform rotateMatrix;
    rotateMatrix.m_matrix[0][0] = c;  // cos(θ)
    rotateMatrix.m_matrix[0][1] = -s; // -sin(θ)
    rotateMatrix.m_matrix[1][0] = s;  // sin(θ)
    rotateMatrix.m_matrix[1][1] = c;  // cos(θ)
    // Translation elements (m[0][2], m[1][2]) remain 0

    this->multiply(rotateMatrix);
}

/**
 * @brief Transforms a point using this transformation matrix.
 *
 * Algorithm (Homogeneous Coordinate Transformation):
 * - Applies the 3x3 transformation matrix to a 2D point
 * - Uses homogeneous coordinates: (x, y) -> (x, y, 1)
 * - Matrix multiplication:
 *   [x']   [a  b  tx]   [x]
 *   [y'] = [c  d  ty] * [y]
 *   [1 ]   [0  0  1 ]   [1]
 *
 * Formula:
 * - newX = a*x + b*y + tx
 * - newY = c*x + d*y + ty
 * - (w = 1, not needed for 2D)
 *
 * Where:
 * - a, b, c, d: rotation, scaling, and skewing components
 * - tx, ty: translation components
 *
 * @param point The point to transform: {x, y}
 * @return SVGPointF The transformed point: {newX, newY}
 */
SVGPointF SVGTransform::map(const SVGPointF& point) const
{

    SVGPointF newPoint;

    SVGNumber x = point.x;
    SVGNumber y = point.y;

    // Apply transformation matrix: newPoint = matrix * point
    newPoint.x = m_matrix[0][0] * x + m_matrix[0][1] * y + m_matrix[0][2];
    newPoint.y = m_matrix[1][0] * x + m_matrix[1][1] * y + m_matrix[1][2];

    return newPoint;
}