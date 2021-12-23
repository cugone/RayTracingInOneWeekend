#pragma once

#include "Hittable.hpp"
#include "Vector3.hpp"
#include "Material.hpp"

#include <cmath>

class Sphere3 : public Hittable {
public:
    Sphere3() = default;
    Sphere3(const Sphere3& other) = default;
    Sphere3(Sphere3&& other) = default;
    Sphere3& operator=(const Sphere3& other) = default;
    Sphere3& operator=(Sphere3&& other) = default;
    virtual ~Sphere3() = default;

    Sphere3(const Point3& c, float r, Material m) : center{ c }, radius{ r }, material{m} {};

    bool hit(const Ray3& r, float t_min, float t_max, hit_record& rec) const override;

    Point3 center{};
    float radius{1.0f};
    Material material{};
protected:
private:
    
};


bool Sphere3::hit(const Ray3& r, float t_min, float t_max, hit_record& rec) const {
    const auto oc = r.origin() - center;
    const auto a = r.direction().length_squared();
    const auto half_b = dot(oc, r.direction());
    const auto c = oc.length_squared() - radius * radius;

    const auto discriminant = half_b * half_b - a * c;
    if(discriminant < 0.0f) return false;
    const auto sqrtd = std::sqrt(discriminant);

    //Find the nearest root that lies in the acceptable range.
    auto root = (-half_b - sqrtd) / a;
    if(root < t_min || t_max < root) {
        root = (-half_b + sqrtd) / a;
        if(root < t_min || t_max < root) {
            rec.hit = false;
            return false;
        }
    }
    
    rec.hit = true;
    rec.t = root;
    rec.p = r.at(rec.t);
    Vector3 outward_normal = (rec.p - center) / radius;
    rec.set_face_normal(r, outward_normal);
    rec.material = material;

    return true;
}
