#pragma once

#include "Vector3.hpp"

class Ray3 {
public:
    Ray3() = default;
    constexpr Ray3(const Ray3& other) = default;
    constexpr Ray3(Ray3&& other) = default;
    constexpr Ray3& operator=(const Ray3& other) = default;
    constexpr Ray3& operator=(Ray3&& other) = default;
    constexpr ~Ray3() = default;

    constexpr Ray3(const Point3& origin, const Vector3& direction) : o{ origin }, d{ direction } {}

    constexpr Point3 origin() const { return o; }
    constexpr Vector3 direction() const { return d; }

    constexpr Point3 at(float t) const {
        return o + t * d;
    }

    Point3 o;
    Vector3 d;
protected:
private:
};
