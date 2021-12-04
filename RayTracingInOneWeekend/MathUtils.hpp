#pragma once

#include <cmath>
#include <climits>
#include <numbers>


constexpr auto infinity = std::numeric_limits<float>::infinity();
constexpr auto pi = std::numbers::pi_v<float>;

inline float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}

#include "Ray3.hpp"
#include "Vector3.hpp"
