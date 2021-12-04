#include "MathUtils.hpp"

#include "Color.hpp"
#include "HittableList.hpp"
#include "Sphere3.hpp"

#include <iostream>
#include <string>

Color ray_color(const Ray3& r, const Hittable& world);
float hit_sphere(const Point3& center, float radius, const Ray3& r);

int main(int argc, char** argv) {

    //Image
    float aspect_ratio = 16.0f / 9.0f;
    const int image_width = [argc, argv]() -> const int {
        return argc > 1 ? static_cast<int>(std::stoll(argv[1])) : 400;
    }();
    const int image_height = [argc, argv, image_width, &aspect_ratio]() -> const int {
        if(argc > 2) {
            const int h = static_cast<int>(std::stoll(argv[2]));
            aspect_ratio = image_width / static_cast<float>(h);
            return h;
        } else {
            return static_cast<int>(image_width / aspect_ratio);
        }
    }();

    //World
    HittableList world{};
    world.add(std::make_shared<Sphere3>(Point3(0.0f, 0.0f, -1.0f), 0.5f));
    world.add(std::make_shared<Sphere3>(Point3(0.0f, -100.5, -1.0f), 100.0f));

    //Camera
    const auto viewport_height = 2.0f;
    const auto viewport_width = aspect_ratio * viewport_height;
    const auto focal_length = 1.0f;

    const auto origin = Point3{0.0f, 0.0f, 0.0f};
    const auto horizontal = Vector3{viewport_width, 0, 0};
    const auto vertical = Vector3{0, viewport_height, 0};
    const auto lower_left_corner = origin - horizontal / 2 - vertical / 2 - Vector3{0, 0, focal_length};


    //Render
    const int max_pixel_value = 255;

    std::cout << "P3\n" << image_width << ' ' << image_height << '\n' << max_pixel_value << '\n';

    for(int y = image_height - 1; y >= 0; --y) {
        std::cerr << "\rScanlines remaining: " << y << ' ' << std::flush;
        for(int x = 0; x < image_width; ++x) {
            const auto u = static_cast<float>(x) / (image_width - 1);
            const auto v = static_cast<float>(y) / (image_height - 1);
            Ray3 r{origin, lower_left_corner + u * horizontal + v * vertical - origin};
            Color pixel_color = ray_color(r, world);
            write_color(std::cout, pixel_color);
        }
    }
    std::cerr << "\nDone.\n";
    return 0;
}

Color ray_color(const Ray3& r, const Hittable& world) {
    hit_record rec{};
    if(world.hit(r, 0, infinity, rec)) {
        return 0.5f * (rec.normal + Color(1, 1, 1));
    }
    Vector3 direction = unit_vector(r.direction());
    auto t = 0.5f * (direction.y() + 1.0f);
    return (1.0f - t) * Color(1.0f, 1.0f, 1.0f) + t * Color(0.5f, 0.7f, 1.0f);
}

float hit_sphere(const Point3& center, float radius, const Ray3& r) {
    Vector3 oc = r.origin() - center;
    const auto a = r.direction().length_squared();
    const auto half_b = dot(oc, r.direction());
    const auto c = oc.length_squared() - radius * radius;
    const auto discriminant = half_b * half_b - a * c;
    if(discriminant < 0.0f) {
        return -1.0f;
    } else {
        return (-half_b - std::sqrt(discriminant)) / a;
    }
}
