#include "Color.hpp"
#include "Vector3.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {

    const int image_width = [argc, argv]() -> const int {
        return argc > 1 ? static_cast<int>(std::stoll(argv[1])) : 256u;
    }();
    const int image_height = [argc, argv]() -> const int {
        return argc > 2 ? static_cast<int>(std::stoll(argv[2])) : 256u;
    }();

    const int max_pixel_value = 255;

    std::cout << "P3\n" << image_width << ' ' << image_height << '\n' << max_pixel_value << '\n';

    for(int y = image_height - 1; y >= 0; --y) {
        std::cerr << "\rScanlines remaining: " << y << ' ' << std::flush;
        for(int x = 0; x < image_width; ++x) {
            Color pixel_color(static_cast<float>(x) / (image_width - 1), static_cast<float>(y) / (image_height - 1), 0.25f);
            write_color(std::cout, pixel_color);
        }
    }
    std::cerr << "\nDone.\n";
    return 0;
}
