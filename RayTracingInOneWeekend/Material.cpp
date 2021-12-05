#include "Material.hpp"

#include "Ray3.hpp"
#include "Hittable.hpp"

bool Material::scatter([[maybe_unused]] const Ray3& ray_in, const hit_record& rec, Ray3& result) {
    switch(type) {
    case Type::Lambertian:
    {
        direction = [&rec]() {
            auto result = rec.normal + random_unit_vector();
            if(result.near_zero()) {
                result = rec.normal;
            }
            return result;
        }();
        result = Ray3{ rec.p, direction };
        return true;
    }
    case Type::Metal:
    {
        direction = reflect(unit_vector(ray_in.direction()), rec.normal);
        result = Ray3{rec.p, direction};
    }
    default:
    {
        return false;
    }
    }
}

Material make_lambertian(const Color& color) {
    Material lambertian{};
    lambertian.color = color;
    lambertian.attenuation = Vector3{1.0f, 0.0f, 0.0f};
    lambertian.type = Material::Type::Lambertian;
    return lambertian;
}

Material make_metal(const Color& color) {
    Material metal{};
    metal.color = color;
    metal.attenuation = Vector3{1.0f, 0.0f, 0.0f};
    metal.type = Material::Type::Metal;
    return metal;
}
