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

        viewMatrix = DirectX::XMMatrixLookAtLH({lookFrom.x(), lookFrom.y(), lookFrom.z() }, { lookAt.x(), lookAt.y(), lookAt.z() }, {vUp.x(), vUp.y(), vUp.z()});
        projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(vfovDegrees, aspectRatio, 0.0001f, 1000.0f);
        viewProjectionMatrix = DirectX::XMMatrixMultiply(viewMatrix, projectionMatrix);

        const auto GetFloat3FromVector = [](const DirectX::XMVECTOR& v)->DirectX::XMFLOAT3 {
            DirectX::XMFLOAT3 result{};
            DirectX::XMStoreFloat3(&result, v);
            return result;
        };

        const auto uFloat3 = GetFloat3FromVector(viewMatrix.r[0]);
        const auto u = Vector3{uFloat3.x, uFloat3.y, uFloat3.z};

        const auto vFloat3 = GetFloat3FromVector(viewMatrix.r[1]);
        const auto v = Vector3{vFloat3.x, vFloat3.y, vFloat3.z};

        const auto wFloat3 = GetFloat3FromVector(viewMatrix.r[2]);
        const auto w = Vector3{wFloat3.x, wFloat3.y, wFloat3.z};


        position = lookFrom;
        old_position = position;
        horizontal = focusDistance * viewport_width * u;
        vertical = focusDistance * viewport_height * v;
        lower_left_corner = position - horizontal * 0.5f - vertical * 0.5f - focusDistance * w;
        lensRadius = aperture * 0.5f;
    }

    Ray3 get_ray(float s, float t) const {

        const auto GetFloat3FromVector = [](const DirectX::XMVECTOR& v)->DirectX::XMFLOAT3 {
            DirectX::XMFLOAT3 result{};
            DirectX::XMStoreFloat3(&result, v);
            return result;
        };

        const auto uFloat3 = GetFloat3FromVector(viewMatrix.r[0]);
        const auto u = Vector3{ uFloat3.x, uFloat3.y, uFloat3.z };

        const auto vFloat3 = GetFloat3FromVector(viewMatrix.r[1]);
        const auto v = Vector3{ vFloat3.x, vFloat3.y, vFloat3.z };

        const auto rd = lensRadius * random_in_unit_disk();
        const auto offset = u * rd.x() + v * rd.y();
        return Ray3{position + offset, lower_left_corner + s * horizontal + t * vertical - position - offset};
    }

    void Update(float deltaSeconds) {
        const auto theta = degrees_to_radians(vFovDegrees);
        const auto h = std::tan(theta * 0.5f);
        const auto viewport_height = 2.0f * h;
        const auto viewport_width = aspectRatio * viewport_height;

        viewMatrix = DirectX::XMMatrixLookAtLH({ position.x(), position.y(), position.z() }, { lookAt.x(), lookAt.y(), lookAt.z() }, { up.x(), up.y(), up.z() });
        projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(vFovDegrees, aspectRatio, 0.0001f, 1000.0f);
        viewProjectionMatrix = DirectX::XMMatrixMultiply(viewMatrix, projectionMatrix);

        const auto GetFloat3FromVector = [](const DirectX::XMVECTOR& v)->DirectX::XMFLOAT3 {
            DirectX::XMFLOAT3 result{};
            DirectX::XMStoreFloat3(&result, v);
            return result;
        };

        const auto uFloat3 = GetFloat3FromVector(viewMatrix.r[0]);
        const auto u = Vector3{ uFloat3.x, uFloat3.y, uFloat3.z };

        const auto vFloat3 = GetFloat3FromVector(viewMatrix.r[1]);
        const auto v = Vector3{ vFloat3.x, vFloat3.y, vFloat3.z };

        const auto wFloat3 = GetFloat3FromVector(viewMatrix.r[2]);
        const auto w = Vector3{ wFloat3.x, wFloat3.y, wFloat3.z };

        position = (position - old_position) * speed * deltaSeconds;
        horizontal = focusDistance * viewport_width * u;
        vertical = focusDistance * viewport_height * v;
        lower_left_corner = position - horizontal * 0.5f - vertical * 0.5f - focusDistance * w;
        lensRadius = aperture * 0.5f;
    }

    void SetSpeed(float newSpeed) {
        speed = newSpeed;
    }
    
    float GetSpeed() {
        return speed;
    }

    DirectX::XMMATRIX GetViewMatrix() const {
        return viewMatrix;
    }

    DirectX::XMMATRIX GetProjectionMatrix() const {
        return projectionMatrix;
    }
    
    DirectX::XMMATRIX GetViewProjectionMatrix() const {
        return viewProjectionMatrix;
    }

protected:
private:
    Point3 position;
    Point3 old_position;
    Point3 lookAt;
    Point3 lower_left_corner;
    Vector3 horizontal;
    Vector3 vertical;
    Vector3 up;
    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;
    DirectX::XMMATRIX viewProjectionMatrix;
    float speed = 5.0f;
    float aperture;
    float lensRadius;
    float focusDistance;
    float aspectRatio;
    float vFovDegrees;
};
