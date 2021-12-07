#pragma once

#include "MathUtils.hpp"

#include <cmath>
#include <ostream>

class Vector3 {
public:
    constexpr Vector3() = default;
    constexpr Vector3(const Vector3& other) = default;
    constexpr Vector3(Vector3&& other) = default;
    constexpr Vector3& operator=(const Vector3& other) = default;
    constexpr Vector3& operator=(Vector3&& other) = default;
    constexpr Vector3(float e0, float e1, float e2);
    constexpr ~Vector3() = default;

    constexpr float x() const;
    constexpr float y() const;
    constexpr float z() const;

    constexpr Vector3 operator-() const;

    Vector3& operator+=(const Vector3& rhs);

    Vector3& operator*=(const float t);

    Vector3& operator/=(const float t);

    constexpr float length_squared() const;

    float length() const;

    static Vector3 random();

    static Vector3 random(float min, float max);

    bool near_zero() const;

protected:
private:
    float m_x{ 0.0f };
    float m_y{ 0.0f };
    float m_z{ 0.0f };
};

using Point3 = Vector3;
using Color = Vector3;

std::ostream& operator<<(std::ostream& out, const Vector3& v);

constexpr Vector3 operator+(const Vector3& u, const Vector3& v);

Vector3 operator-(const Vector3& u, const Vector3& v);

Vector3 operator*(const Vector3& u, const Vector3& v);

constexpr Vector3 operator*(float t, const Vector3& v);

Vector3 operator*(const Vector3& v, float t);

Vector3 operator/(const Vector3& v, float t);

constexpr float dot(const Vector3& u, const Vector3& v);

Vector3 cross(const Vector3& u, const Vector3& v);

Vector3 unit_vector(Vector3 v);

Vector3 reflect(const Vector3& v, const Vector3& n);

Vector3 refract(const Vector3& uv, const Vector3& n, float eta_over_etaprime);

Vector3 random_in_unit_sphere();

Vector3 random_unit_vector();

Vector3 random_in_unit_disk();