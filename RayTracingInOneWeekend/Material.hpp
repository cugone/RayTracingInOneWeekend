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
    Material::Type type{};
protected:
private:
};

Material make_lambertian(const Color& color);
Material make_metal(const Color& color);
