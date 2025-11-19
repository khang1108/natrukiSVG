#include "SVGTransform.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SVGTransform::SVGTransform()
{

    std::memset(m_matrix, 0, sizeof(m_matrix));

    m_matrix[0][0] = 1.0;
    m_matrix[1][1] = 1.0;
    m_matrix[2][2] = 1.0;
}

void SVGTransform::multiply(const SVGTransform& other)
{

    SVGNumber result[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {

            for (int k = 0; k < 3; ++k) {
                result[i][j] += m_matrix[i][k] * other.m_matrix[k][j];
            }
        }
    }

    std::memcpy(m_matrix, result, sizeof(m_matrix));
}

void SVGTransform::translate(SVGNumber tx, SVGNumber ty)
{

    SVGTransform translateMatrix;
    translateMatrix.m_matrix[0][2] = tx;
    translateMatrix.m_matrix[1][2] = ty;

    this->multiply(translateMatrix);
}

void SVGTransform::scale(SVGNumber sx, SVGNumber sy)
{

    SVGTransform scaleMatrix;
    scaleMatrix.m_matrix[0][0] = sx;
    scaleMatrix.m_matrix[1][1] = sy;

    this->multiply(scaleMatrix);
}

void SVGTransform::rotate(SVGNumber angle)
{

    SVGNumber rad = angle * M_PI / 180.0;
    SVGNumber c = std::cos(rad);
    SVGNumber s = std::sin(rad);

    SVGTransform rotateMatrix;
    rotateMatrix.m_matrix[0][0] = c;
    rotateMatrix.m_matrix[0][1] = -s;
    rotateMatrix.m_matrix[1][0] = s;
    rotateMatrix.m_matrix[1][1] = c;

    this->multiply(rotateMatrix);
}

SVGPointF SVGTransform::map(const SVGPointF& point) const
{

    SVGPointF newPoint;

    SVGNumber x = point.x;
    SVGNumber y = point.y;

    newPoint.x = m_matrix[0][0] * x + m_matrix[0][1] * y + m_matrix[0][2];
    newPoint.y = m_matrix[1][0] * x + m_matrix[1][1] * y + m_matrix[1][2];

    return newPoint;
}