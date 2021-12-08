#include "Material.hpp"

#include "Ray3.hpp"
#include "Hittable.hpp"

bool Material::scatter([[maybe_unused]] const Ray3& ray_in, const hit_record& rec, Ray3& result) {
    switch(type) {
    case Type::Lambertian:
    {
        const auto direction = [&rec, this]() {
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
        const auto direction = metallic * reflect(unit_vector(ray_in.direction()), rec.normal);
        result = Ray3{rec.p, direction + roughness * random_in_unit_sphere()};
        return (dot(result.direction(), rec.normal) > 0);
    }
    case Type::Glass:
    {
        static const auto reflectance = [](float cosine, float ref_idx) {
            // Use Schlick's approximation for reflectance.
            auto r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
            r0 = r0 * r0;
            return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5.0f);
        };
        const auto direction = [&rec, &ray_in, this]() {
            const auto refraction_ratio = rec.front_face ? (1.0f / refractionIndex) : refractionIndex;
            const auto unit_direction = unit_vector(ray_in.direction());
            const auto cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0f);
            const auto sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
            const auto cannot_refract = refraction_ratio * sin_theta > 1.0f;
            if(cannot_refract || reflectance(cos_theta, refraction_ratio) > random_float()) {
                return reflect(unit_direction, rec.normal);
            } else {
                return refract(unit_direction, rec.normal, refraction_ratio);
            }
        }();
        result = Ray3{ rec.p, direction };
        return true;
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
    lambertian.refractionIndex = desc.refractionIndex;
    return lambertian;
}

Material make_metal(const MaterialDesc& desc) {
    Material metal{};
    metal.color = desc.color;
    metal.attenuation = Vector3{1.0f, 0.0f, 0.0f};
    metal.roughness = desc.roughness < 1.0f ? desc.roughness : 1.0f;
    metal.metallic = desc.metallic;
    metal.refractionIndex = desc.refractionIndex;
    metal.type = Material::Type::Metal;
    return metal;
}

Material make_dielectric(const MaterialDesc& desc) {
    Material glass{};
    glass.color = desc.color;
    glass.attenuation = Vector3{1.0f, 1.0f, 1.0f};
    glass.roughness = desc.roughness < 1.0f ? desc.roughness : 1.0f;
    glass.metallic = 0.0f;
    glass.refractionIndex = desc.refractionIndex;
    glass.type = Material::Type::Glass;
    return glass;
}
