#include "MathUtils.hpp"

#include "Camera.hpp"
#include "Color.hpp"
#include "HittableList.hpp"
#include "Material.hpp"
#include "Sphere3.hpp"

#include <iostream>
#include <fstream>
#include <string>

Color ray_color(const Ray3& r, const Hittable& world, int depth);
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
    const int samples_per_pixel = [argc, argv]() {
        return argc > 3 ? static_cast<int>(std::stoll(argv[3])) : 100;
    }();
    const int max_depth = [argc, argv]() {
        return argc > 4 ? static_cast<int>(std::stoll(argv[4])) : 50;
    }();


    //World
    HittableList world{};

    auto material_ground = make_lambertian(MaterialDesc{ Color{0.8f, 0.8f, 0.0f} });
    auto material_center = make_lambertian(MaterialDesc{Color{0.1f, 0.2f, 0.5f}});
    auto material_left = make_dielectric(MaterialDesc{ Color{1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, 1.5f });
    auto material_right = make_metal(MaterialDesc{ Color{0.8f, 0.6f, 0.2f}, 0.0f, 1.0f });

    world.add(std::make_shared<Sphere3>(Point3{0.0f, -100.5f, -1.0f}, 100.0f, material_ground));
    world.add(std::make_shared<Sphere3>(Point3{0.0f, 0.0f, -1.0f}, 0.5f, material_center));
    world.add(std::make_shared<Sphere3>(Point3{-1.0f, 0.0f, -1.0f}, 0.5f, material_left));
    world.add(std::make_shared<Sphere3>(Point3{-1.0f, 0.0f, -1.0f}, -0.45f, material_left));
    world.add(std::make_shared<Sphere3>(Point3{1.0f, 0.0f, -1.0f}, 0.5f, material_right));

    //Camera
    Camera camera{ Point3{-2.0f, 2.0f, 1.0f}, Point3{0.0f, 0.0f, -1.0f}, Vector3{0.0f, 1.0f, 0.0f}, 90, aspect_ratio };

    //Render
    const int max_pixel_value = 255;

    std::ofstream bin_file("image_binary.ppm", std::ios_base::binary);
    bin_file << "P6\n" << image_width << ' ' << image_height << '\n' << max_pixel_value << '\n';
    for(int y = image_height - 1; y >= 0; --y) {
        std::cerr << "\rScanlines remaining: " << y << ' ' << std::flush;
        for(int x = 0; x < image_width; ++x) {
            Color pixel_color{0.0f, 0.0f, 0.0f};
            for(int sample = 0; sample < samples_per_pixel; ++sample) {
                const auto u = (x + random_float()) / (image_width - 1);
                const auto v = (y + random_float()) / (image_height - 1);
                const auto r = camera.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }
            write_color_binary(bin_file, pixel_color, samples_per_pixel);
        }
    }
    std::cerr << "\nDone.\n";
    bin_file.close();
    return 0;
}

Color ray_color(const Ray3& r, const Hittable& world, int depth) {
    hit_record rec{};

    //If we've exceeded the ray bounce limit, no more light is gathered.
    if(depth <= 0) {
        return Color{0.0f, 0.0f, 0.0f};
    }
    if(world.hit(r, 0.001f, infinity, rec)) {
        Ray3 scattered{};
        if(rec.material.scatter(r, rec, scattered)) {
            return rec.material.color * ray_color(scattered, world, depth - 1);
        }
        return Color{0.0f, 0.0f, 0.0f};
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
