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

HittableList random_scene();

int main(int argc, char** argv) {

    //Image
    float aspect_ratio = 3.0f / 2.0f;
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
    HittableList world = random_scene();

    //Camera
    const auto lookFrom = Point3{13.0f, 2.0f, 3.0f};
    const auto lookAt = Point3{0.0f, 0.0f, 0.0f};
    const auto vUp = Vector3{0.0f, 1.0f, 0.0f};
    const auto distance_to_focus = 10.0f;
    const auto aperture = 0.1f;

    Camera camera{lookFrom, lookAt, vUp, 20, aspect_ratio, aperture, distance_to_focus};

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

HittableList random_scene() {
    HittableList world{};

    const auto ground_material = make_lambertian(MaterialDesc{ Color{0.5f, 0.5f, 0.5f} });
    world.add(std::make_shared<Sphere3>(Point3{0.0f, -1000.0f, 0.0f}, 1000.0f, ground_material));

    for(int a = -11; a < 11; ++a) {
        for(int b = -11; b < 11; ++b) {
            const auto choose_mat = random_float();
            const auto center = Point3{a + 0.9f * random_float(), 0.2f, b + 0.9f * random_float()};

            if((center - Point3{4.0f, 0.2f, 0.0f}).length() > 0.9f) {
                Material material{};
                MaterialDesc desc{};
                if(choose_mat < 0.8f) {
                    desc.color = Color::random() * Color::random();
                    material = make_lambertian(desc);
                } else if(choose_mat < 0.95f) {
                    desc.color = Color::random(0.5f, 1.0f);
                    desc.roughness = random_float(0.0f, 0.5f);
                    desc.metallic = 1.0f;
                    material = make_metal(desc);
                } else {
                    desc.refractionIndex = 1.5f;
                    desc.color = Color{1.0f, 1.0f, 1.0f};
                    desc.roughness = 0.0f;
                    material = make_dielectric(desc);
                }
                world.add(std::make_shared<Sphere3>(center, 0.2f, material));
            }
        }
    }

    const auto glass = make_dielectric(MaterialDesc{ Color{1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, 1.5f });
    const auto lambertian = make_lambertian(MaterialDesc{ Color{0.4f, 0.2f, 0.1f}});
    const auto metal = make_metal(MaterialDesc{ Color{0.7f, 0.6f, 0.5f}, 0.0f, 1.0f});

    world.add(std::make_shared<Sphere3>(Point3{0.0f, 1.0f, 0.0f}, 1.0f, glass));
    world.add(std::make_shared<Sphere3>(Point3{-4.0f, 1.0f, 0.0f}, 1.0f, lambertian));
    world.add(std::make_shared<Sphere3>(Point3{4.0f, 1.0f, 0.0f}, 1.0f, metal));

    return world;
}