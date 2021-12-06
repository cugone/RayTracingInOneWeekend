#pragma once

#include "Ray3.hpp"
#include "Vector3.hpp"

class Camera {
public:
    Camera(Point3 lookFrom, Point3 lookAt, Vector3 vUp, float vfovDegrees, float aspectRatio) {
        const auto theta = degrees_to_radians(vfovDegrees);
        const auto h = std::tan(theta * 0.5f);
        const auto viewport_height = 2.0f * h;
        const auto viewport_width = aspectRatio * viewport_height;

        const auto w = unit_vector(lookFrom - lookAt);
        const auto u = unit_vector(cross(vUp, w));
        const auto v = cross(w, u);

        origin = lookFrom;
        horizontal = viewport_width * u;
        vertical = viewport_height * v;
        lower_left_corner = origin - horizontal * 0.5f - vertical * 0.5f - w;
    }

    Ray3 get_ray(float s, float t) const {
        return Ray3{origin, lower_left_corner + s * horizontal + t * vertical - origin};
    }
protected:
private:
    Point3 origin;
    Point3 lower_left_corner;
    Vector3 horizontal;
    Vector3 vertical;
};
