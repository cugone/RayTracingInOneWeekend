#pragma once

#include "Ray3.hpp"
#include "Vector3.hpp"

#include <DirectXMath.h>

class Camera {
public:
    Camera(Point3 lookFrom, Point3 lookAt, Vector3 vUp, float vfovDegrees, float aspectRatio, float aperture, float focusDistance) {
        const auto theta = degrees_to_radians(vfovDegrees);
        const auto h = std::tan(theta * 0.5f);
        const auto viewport_height = 2.0f * h;
        const auto viewport_width = aspectRatio * viewport_height;

        m_viewMatrix = DirectX::XMMatrixLookAtLH({lookFrom.x(), lookFrom.y(), lookFrom.z() }, { lookAt.x(), lookAt.y(), lookAt.z() }, {vUp.x(), vUp.y(), vUp.z()});
        m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(vfovDegrees, aspectRatio, 0.0001f, 1000.0f);
        m_viewProjectionMatrix = DirectX::XMMatrixMultiply(m_viewMatrix, m_projectionMatrix);

        const auto GetFloat3FromVector = [](const DirectX::XMVECTOR& v)->DirectX::XMFLOAT3 {
            DirectX::XMFLOAT3 result{};
            DirectX::XMStoreFloat3(&result, v);
            return result;
        };

        const auto uFloat3 = GetFloat3FromVector(m_viewMatrix.r[0]);
        const auto u = Vector3{uFloat3.x, uFloat3.y, uFloat3.z};

        const auto vFloat3 = GetFloat3FromVector(m_viewMatrix.r[1]);
        const auto v = Vector3{vFloat3.x, vFloat3.y, vFloat3.z};

        const auto wFloat3 = GetFloat3FromVector(m_viewMatrix.r[2]);
        const auto w = Vector3{wFloat3.x, wFloat3.y, wFloat3.z};


        m_position = lookFrom;
        m_horizontal = focusDistance * viewport_width * u;
        m_vertical = focusDistance * viewport_height * v;
        m_lower_left_corner = m_position - m_horizontal * 0.5f - m_vertical * 0.5f - focusDistance * w;
        m_lensRadius = aperture * 0.5f;
    }

    Ray3 get_ray(float s, float t) const {

        const auto GetFloat3FromVector = [](const DirectX::XMVECTOR& v)->DirectX::XMFLOAT3 {
            DirectX::XMFLOAT3 result{};
            DirectX::XMStoreFloat3(&result, v);
            return result;
        };

        const auto uFloat3 = GetFloat3FromVector(m_viewMatrix.r[0]);
        const auto u = Vector3{ uFloat3.x, uFloat3.y, uFloat3.z };

        const auto vFloat3 = GetFloat3FromVector(m_viewMatrix.r[1]);
        const auto v = Vector3{ vFloat3.x, vFloat3.y, vFloat3.z };

        const auto rd = m_lensRadius * random_in_unit_disk();
        const auto offset = u * rd.x() + v * rd.y();
        return Ray3{m_position + offset, m_lower_left_corner + s * m_horizontal + t * m_vertical - m_position - offset};
    }

    void Update(float deltaSeconds) {
        const auto theta = m_vFovRadians;
        const auto h = std::tan(theta * 0.5f);
        const auto viewport_height = 2.0f * h;
        const auto viewport_width = m_aspectRatio * viewport_height;

        m_viewMatrix = DirectX::XMMatrixLookAtLH({ m_position.x(), m_position.y(), m_position.z() }, { m_lookAt.x(), m_lookAt.y(), m_lookAt.z() }, { m_up.x(), m_up.y(), m_up.z() });
        m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(m_vFovRadians, m_aspectRatio, 0.0001f, 1000.0f);
        m_viewProjectionMatrix = DirectX::XMMatrixMultiply(m_viewMatrix, m_projectionMatrix);

        const auto GetFloat3FromVector = [](const DirectX::XMVECTOR& v)->DirectX::XMFLOAT3 {
            DirectX::XMFLOAT3 result{};
            DirectX::XMStoreFloat3(&result, v);
            return result;
        };

        const auto uFloat3 = GetFloat3FromVector(m_viewMatrix.r[0]);
        const auto u = Vector3{ uFloat3.x, uFloat3.y, uFloat3.z };

        const auto vFloat3 = GetFloat3FromVector(m_viewMatrix.r[1]);
        const auto v = Vector3{ vFloat3.x, vFloat3.y, vFloat3.z };

        const auto wFloat3 = GetFloat3FromVector(m_viewMatrix.r[2]);
        const auto w = Vector3{ wFloat3.x, wFloat3.y, wFloat3.z };

        const auto oldVelocity = m_velocity;
        const auto oldPosition = m_position;

        m_velocity = oldVelocity * deltaSeconds;
        m_position = oldPosition + m_velocity;
        m_horizontal = m_focusDistance * viewport_width * u;
        m_vertical = m_focusDistance * viewport_height * v;
        m_lower_left_corner = m_position - m_horizontal * 0.5f - m_vertical * 0.5f - m_focusDistance * w;
        m_lensRadius = m_aperture * 0.5f;
    }

    void SetSpeed(float newSpeed) {
        m_speed = newSpeed;
    }
    
    float GetSpeed() {
        return m_speed;
    }

    DirectX::XMMATRIX GetViewMatrix() const {
        return m_viewMatrix;
    }

    DirectX::XMMATRIX GetProjectionMatrix() const {
        return m_projectionMatrix;
    }
    
    DirectX::XMMATRIX GetViewProjectionMatrix() const {
        return m_viewProjectionMatrix;
    }

    float GetFovRadians() const {
        return m_vFovRadians;
    }
    
    float GetFovDegrees() const {
        return radians_to_degrees(m_vFovRadians);
    }

protected:
private:
    Point3 m_position;
    Vector3 m_velocity{0.0f, 0.0f, 0.0f};
    Point3 m_lookAt;
    Point3 m_lower_left_corner;
    Vector3 m_horizontal;
    Vector3 m_vertical;
    Vector3 m_up;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;
    DirectX::XMMATRIX m_viewProjectionMatrix;
    float m_speed = 5.0f;
    float m_aperture;
    float m_lensRadius;
    float m_focusDistance;
    float m_aspectRatio;
    float m_vFovRadians;
};
