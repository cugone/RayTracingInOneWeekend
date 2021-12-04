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
        for(int x = 0; x < image_width; ++x) {
            const auto r = static_cast<double>(x) / (image_width - 1);
            const auto g = static_cast<double>(y) / (image_height - 1);
            const auto b = 0.25;

            const auto ir = static_cast<int>((max_pixel_value + 1) * 0.999999 * r);
            const auto ig = static_cast<int>((max_pixel_value + 1) * 0.999999 * g);
            const auto ib = static_cast<int>((max_pixel_value + 1) * 0.999999 * b);

            std::cout << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }
    return 0;
}
