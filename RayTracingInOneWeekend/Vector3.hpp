#pragma once

class Vector3 {
public:
    Vector3() = default;
    Vector3(const Vector3& other) = default;
    Vector3(Vector3&& other) = default;
    Vector3& operator=(const Vector3& other) = default;
    Vector3& operator=(Vector3&& other) = default;
    Vector3(float e0, float e1, float e2) : m_e{ e0, e1, e2 } {};
    ~Vector3() = default;

    float x() const { return m_e[0]; };
    float y() const { return m_e[1]; };
    float z() const { return m_e[2]; };

    Vector3 operator-() const { return Vector3{-m_e[0], -m_e[1], -m_e[2]}; };
    float operator[](int i) const { return m_e[i]; }
    float& operator[](int i) { return m_e[i]; }

    Vector3& operator+=(const Vector3& rhs) {
        m_e[0] += rhs.m_e[0];
        m_e[1] += rhs.m_e[1];
        m_e[2] += rhs.m_e[2];
    }

    Vector3& operator*=(const float t) {
        m_e[0] *= t;
        m_e[1] *= t;
        m_e[2] *= t;
        return *this;
    }

    Vector3& operator/=(const float t) {
        return *this *= 1.0f / t;
    }

    float length() const {
        return std::sqrt(length_squared());
    }

    float length_squared() const {
        return m_e[0] * m_e[0] + m_e[1] * m_e[1] + m_e[2] * m_e[2];
    }

protected:
private:
    float m_e[3];
};

using Point3 = Vector3;
using Color = Vector3;
