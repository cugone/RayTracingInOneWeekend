#pragma once

#include "MathUtils.hpp"

#include <cmath>
#include <ostream>

class Vector3 {
public:
    Vector3() = default;
    Vector3(const Vector3& other) = default;
    Vector3(Vector3&& other) = default;
    Vector3& operator=(const Vector3& other) = default;
    Vector3& operator=(Vector3&& other) = default;
    Vector3(float e0, float e1, float e2);
    ~Vector3() = default;

    float x() const;
    float y() const;
    float z() const;

    Vector3 operator-() const;
    float operator[](int i) const;
    float& operator[](int i);

    Vector3& operator+=(const Vector3& rhs);

    Vector3& operator*=(const float t);

    Vector3& operator/=(const float t);

    float length_squared() const;

    float length() const;

    static Vector3 random();

    static Vector3 random(float min, float max);

    bool near_zero() const;

protected:
private:
    float m_e[3];
};

using Point3 = Vector3;
using Color = Vector3;

std::ostream& operator<<(std::ostream& out, const Vector3& v);

Vector3 operator+(const Vector3& u, const Vector3& v);

Vector3 operator-(const Vector3& u, const Vector3& v);

Vector3 operator*(const Vector3& u, const Vector3& v);

Vector3 operator*(float t, const Vector3& v);

Vector3 operator*(const Vector3& v, float t);

Vector3 operator/(const Vector3& v, float t);

float dot(const Vector3& u, const Vector3& v);

Vector3 cross(const Vector3& u, const Vector3& v);

Vector3 unit_vector(Vector3 v);

Vector3 reflect(const Vector3& v, const Vector3& n);

Vector3 random_in_unit_sphere();

Vector3 random_unit_vector();
