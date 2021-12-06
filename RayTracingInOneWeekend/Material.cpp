#include "Material.hpp"

#include "Ray3.hpp"
#include "Hittable.hpp"

bool Material::scatter([[maybe_unused]] const Ray3& ray_in, const hit_record& rec, Ray3& result) {
    switch(type) {
    case Type::Lambertian:
    {
        direction = [&rec, this]() {
            auto result = rec.normal + roughness * random_unit_vector();
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
        direction = metallic * reflect(unit_vector(ray_in.direction()), rec.normal);
        result = Ray3{rec.p, direction + roughness * random_in_unit_sphere()};
        return (dot(result.direction(), rec.normal) > 0);
    }
    default:
    {
        return false;
    }
    }
}

Material make_material(const MaterialDesc& desc) {
    Material m{};
    m.color = desc.color;
    m.metallic = desc.metallic;
    m.roughness = desc.roughness;
    m.attenuation = Vector3{1.0f, 0.0f, 0.0f};
    return m;
}

Material make_lambertian(const MaterialDesc& desc) {
    Material lambertian{};
    lambertian.color = desc.color;
    lambertian.attenuation = Vector3{1.0f, 0.0f, 0.0f};
    lambertian.type = Material::Type::Lambertian;
    lambertian.roughness = desc.roughness < 1.0f ? desc.roughness : 1.0f;
    lambertian.metallic = desc.metallic;
    return lambertian;
}

Material make_metal(const MaterialDesc& desc) {
    Material metal{};
    metal.color = desc.color;
    metal.attenuation = Vector3{1.0f, 0.0f, 0.0f};
    metal.roughness = desc.roughness < 1.0f ? desc.roughness : 1.0f;
    metal.metallic = desc.metallic;
    metal.type = Material::Type::Metal;
    return metal;
}
