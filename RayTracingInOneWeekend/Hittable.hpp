#pragma once

#include "Ray3.hpp"
#include "Vector3.hpp"

struct hit_record {
    bool hit{false};
    Point3 p{};
    Vector3 normal{};
    float t;
};

class Hittable {
public:
    virtual bool hit(const Ray3& r, float t_min, float t_max, hit_record& rec) const = 0;
    virtual ~Hittable() noexcept = default;
protected:
private:

};
