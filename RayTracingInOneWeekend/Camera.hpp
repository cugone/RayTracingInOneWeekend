#pragma once

#include "Ray3.hpp"
#include "Vector3.hpp"

class Camera {
public:
    Camera() : Camera(90, 16.0f / 9.0f) {};
    Camera(float vfovDegrees, float aspect_ratio) {
        const auto theta = degrees_to_radians(vfovDegrees);
        const auto h = std::tan(theta * 0.5f);
        const auto viewport_height = 2.0f * h;
        const auto viewport_width = aspect_ratio * viewport_height;
        const auto focal_length = 1.0f;

        origin = Point3{ 0.0f, 0.0f, 0.0f };
        horizontal = Vector3{ viewport_width, 0.0f, 0.0f };
        vertical = Vector3{ 0.0f, viewport_height, 0.0f };
        lower_left_corner = origin - horizontal / 2.0f - vertical / 2.0f - Vector3{0.0f, 0.0f, focal_length};
    }

    Ray3 get_ray(float u, float v) const {
        return Ray3{origin, lower_left_corner + u * horizontal + v * vertical - origin};
    }
protected:
private:
    Point3 origin;
    Point3 lower_left_corner;
    Vector3 horizontal;
    Vector3 vertical;
};
