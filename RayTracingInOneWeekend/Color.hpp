#pragma once

#include "Vector3.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

void write_color(std::ostream& out, Color pixel_color, int samples_per_pixel) {

    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Divide the color by the number of samples and gamma-correct for gamma = 2.0
    const auto scale = 1.0f / samples_per_pixel;
    r = std::sqrt(scale * r);
    g = std::sqrt(scale * g);
    b = std::sqrt(scale * b);

    //Write the translated [0, 255] value of each color component.
    out << static_cast<int>(255 * std::clamp(r, 0.0f, 1.0f)) << ' '
        << static_cast<int>(255 * std::clamp(g, 0.0f, 1.0f)) << ' '
        << static_cast<int>(255 * std::clamp(b, 0.0f, 1.0f)) << '\n';

}

