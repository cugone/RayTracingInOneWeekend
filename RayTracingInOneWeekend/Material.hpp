#pragma once

#include "Vector3.hpp"

class Ray3;
struct hit_record;

class Material {
public:
    enum class Type {
        None
        ,Lambertian
        ,Metal
    };

    bool scatter(const Ray3& ray_in, const hit_record& rec, Ray3& result);

    Vector3 attenuation{};
    Vector3 direction{};
    Color color{};
    float roughness = 1.0f;
    float metallic = 0.0f;
    Material::Type type{};
protected:
private:
};

struct MaterialDesc {
    Color color{1.0f, 1.0f, 1.0f};
    float roughness{1.0f};
    float metallic{0.0f};
};

Material make_material(const MaterialDesc& desc);
Material make_lambertian(const MaterialDesc& desc);
Material make_metal(const MaterialDesc& desc);
