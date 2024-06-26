// This is an incomplete port of http://glmatrix.net/
//
// Copyright (c) 2013 Brandon Jones, Colin MacKenzie IV
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the
// use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not claim
//    that you wrote the original software. If you use this software in a
//    product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <mbgl/util/mat3.hpp>

#include <cmath>

namespace mbgl {
namespace matrix {

void identity(mat3& out) noexcept {
    out[0] = 1.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    out[3] = 0.0f;
    out[4] = 1.0f;
    out[5] = 0.0f;
    out[6] = 0.0f;
    out[7] = 0.0f;
    out[8] = 1.0f;
}

void translate(mat3& out, const mat3& a, double x, double y) noexcept {
    const double a00 = a[0];
    const double a01 = a[1];
    const double a02 = a[2];
    const double a10 = a[3];
    const double a11 = a[4];
    const double a12 = a[5];
    const double a20 = a[6];
    const double a21 = a[7];
    const double a22 = a[8];

    out[0] = a00;
    out[1] = a01;
    out[2] = a02;

    out[3] = a10;
    out[4] = a11;
    out[5] = a12;

    out[6] = x * a00 + y * a10 + a20;
    out[7] = x * a01 + y * a11 + a21;
    out[8] = x * a02 + y * a12 + a22;
}

void rotate(mat3& out, const mat3& a, double rad) noexcept {
    const double s = std::sin(rad);
    const double c = std::cos(rad);
    const double a00 = a[0];
    const double a01 = a[1];
    const double a02 = a[2];
    const double a10 = a[3];
    const double a11 = a[4];
    const double a12 = a[5];
    const double a20 = a[6];
    const double a21 = a[7];
    const double a22 = a[8];

    out[0] = c * a00 + s * a10;
    out[1] = c * a01 + s * a11;
    out[2] = c * a02 + s * a12;

    out[3] = c * a10 - s * a00;
    out[4] = c * a11 - s * a01;
    out[5] = c * a12 - s * a02;

    out[6] = a20;
    out[7] = a21;
    out[8] = a22;
}

void scale(mat3& out, const mat3& a, double x, double y) noexcept {
    out[0] = x * a[0];
    out[1] = x * a[1];
    out[2] = x * a[2];
    out[3] = y * a[3];
    out[4] = y * a[4];
    out[5] = y * a[5];
    out[6] = a[6];
    out[7] = a[7];
    out[8] = a[8];
}

void transformMat3f(vec3f& out, const vec3f& a, const mat3& m) noexcept {
    out[0] = static_cast<float>(m[0]) * a[0] + static_cast<float>(m[3]) * a[1] + static_cast<float>(m[6]) * a[2];
    out[1] = static_cast<float>(m[1]) * a[0] + static_cast<float>(m[4]) * a[1] + static_cast<float>(m[7]) * a[2];
    out[2] = static_cast<float>(m[2]) * a[0] + static_cast<float>(m[5]) * a[1] + static_cast<float>(m[8]) * a[2];
}

} // namespace matrix
} // namespace mbgl
