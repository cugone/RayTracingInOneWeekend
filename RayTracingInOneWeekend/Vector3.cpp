#include "Vector3.hpp"

Vector3 random_in_unit_sphere() {
    for(;;) {
        const auto p = Vector3::random(-1.0f, 1.0f);
        if(p.length_squared() > 1.0f) continue;
        return p;
    }
}

std::ostream& operator<<(std::ostream& out, const Vector3& v) {
    return out << v.x() << ' ' << v.y() << ' ' << v.z();
}

Vector3 operator+(const Vector3& u, const Vector3& v) {
    return Vector3{ u.x() + v.x(), u.y() + v.y(), u.z() + v.z() };
}

Vector3 operator-(const Vector3& u, const Vector3& v) {
    return Vector3{ u.x() - v.x(), u.y() - v.y(), u.z() - v.z() };
}

Vector3 operator*(const Vector3& u, const Vector3& v) {
    return Vector3{ u.x() * v.x(), u.y() * v.y(), u.z() * v.z() };
}

Vector3 operator*(float t, const Vector3& v) {
    return Vector3{ t * v.x(), t * v.y(), t * v.z() };
}

Vector3 operator*(const Vector3& v, float t) {
    return t * v;
}

Vector3 operator/(const Vector3& v, float t) {
    return (1 / t) * v;
}

float dot(const Vector3& u, const Vector3& v) {
    return u.x() * v.x() + u.y() * v.y() + u.z() * v.z();
}

Vector3 cross(const Vector3& u, const Vector3& v) {
    return Vector3{ u.y() * v.z() - u.z() * v.y(),
                   u.z() * v.x() - u.x() * v.z(),
                   u.x() * v.y() - u.y() * v.x() };
}

Vector3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}

Vector3 unit_vector(Vector3 v) {
    return v / v.length();
}

Vector3 reflect(const Vector3& v, const Vector3& n) {
    return v - 2.0f * dot(v, n) * n;
}

Vector3 refract(const Vector3& uv, const Vector3& n, float eta_over_etaprime) {
    const auto cos_theta = std::fmin(dot(-uv, n), 1.0f);
    const auto perpendicular = eta_over_etaprime * (uv + cos_theta * n);
    const auto parallel = -std::sqrt(std::fabs(1.0f - perpendicular.length_squared())) * n;
    return perpendicular + parallel;
}

Vector3& Vector3::operator/=(const float t) {
    return *this *= 1.0f / t;
}

Vector3& Vector3::operator*=(const float t) {
    m_e[0] *= t;
    m_e[1] *= t;
    m_e[2] *= t;
    return *this;
}

Vector3& Vector3::operator+=(const Vector3& rhs) {
    m_e[0] += rhs.m_e[0];
    m_e[1] += rhs.m_e[1];
    m_e[2] += rhs.m_e[2];
    return *this;
}

Vector3::Vector3(float e0, float e1, float e2) : m_e{ e0, e1, e2 } {

}

float Vector3::x() const {
    return m_e[0];
}

float Vector3::y() const {
    return m_e[1];
}

float Vector3::z() const {
    return m_e[2];
}

Vector3 Vector3::operator-() const {
    return Vector3{ -m_e[0], -m_e[1], -m_e[2] };
}

float& Vector3::operator[](int i) {
    return m_e[i];
}

float Vector3::operator[](int i) const {
    return m_e[i];
}

float Vector3::length_squared() const {
    return m_e[0] * m_e[0] + m_e[1] * m_e[1] + m_e[2] * m_e[2];
}

float Vector3::length() const {
    return std::sqrt(length_squared());
}

Vector3 Vector3::random(float min, float max) {
    return Vector3{ random_float(min, max), random_float(min, max), random_float(min, max) };
}

Vector3 Vector3::random() {
    return Vector3{ random_float(), random_float(), random_float() };
}

bool Vector3::near_zero() const {
    const auto epsilon = 1e-8;
    return (std::abs(m_e[0]) < epsilon) && (std::abs(m_e[1]) < epsilon) && (std::abs(m_e[2]) < epsilon);
}
