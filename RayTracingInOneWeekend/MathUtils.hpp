#pragma once

#include <climits>
#include <cmath>
#include <numbers>
#include <random>

constexpr auto infinity = std::numeric_limits<float>::infinity();
constexpr auto pi = std::numbers::pi_v<float>;

inline float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}

inline float random_float() {
    std::uniform_real_distribution<float> d(0.0f, 1.0f);
    static std::mt19937 g;
    return d(g);
}

inline float random_float(float min, float exclusive_max) {
    std::uniform_real_distribution<float> d(min, exclusive_max);
    static std::mt19937 g;
    return d(g);
}
