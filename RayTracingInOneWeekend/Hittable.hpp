#pragma once

#include "Ray3.hpp"
#include "Vector3.hpp"

struct hit_record {
    Point3 p{};
    Vector3 normal{};
    float t;
    bool hit{false};
    bool front_face{ false };

    inline void set_face_normal(const Ray3& r, const Vector3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Hittable {
public:
    virtual bool hit(const Ray3& r, float t_min, float t_max, hit_record& rec) const = 0;
    virtual ~Hittable() noexcept = default;
protected:
private:

};
