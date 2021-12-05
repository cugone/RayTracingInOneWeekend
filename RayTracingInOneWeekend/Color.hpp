#pragma once

#include "Vector3.hpp"

#include <algorithm>
#include <iostream>

void write_color(std::ostream& out, Color pixel_color, int samples_per_pixel) {

    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // DIvide the color by the number of samples.
    const auto scale = 1.0f / samples_per_pixel;
    r *= scale;
    g *= scale;
    b *= scale;

    //Write the translated [0l255] value of each color component.
    out << static_cast<int>(256 * std::clamp(r, 0.0f, 0.999f)) << ' '
        << static_cast<int>(256 * std::clamp(g, 0.0f, 0.999f)) << ' '
        << static_cast<int>(256 * std::clamp(b, 0.0f, 0.999f)) << '\n';

}

