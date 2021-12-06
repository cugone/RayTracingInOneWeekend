#pragma once

#include "Ray3.hpp"
#include "Vector3.hpp"

class Camera {
public:
    Camera(Point3 lookFrom, Point3 lookAt, Vector3 vUp, float vfovDegrees, float aspectRatio, float aperture, float focusDistance) {
        const auto theta = degrees_to_radians(vfovDegrees);
        const auto h = std::tan(theta * 0.5f);
        const auto viewport_height = 2.0f * h;
        const auto viewport_width = aspectRatio * viewport_height;

        w = unit_vector(lookFrom - lookAt);
        u = unit_vector(cross(vUp, w));
        v = cross(w, u);

        origin = lookFrom;
        horizontal = focusDistance * viewport_width * u;
        vertical = focusDistance * viewport_height * v;
        lower_left_corner = origin - horizontal * 0.5f - vertical * 0.5f - focusDistance * w;
        lens_radius = aperture * 0.5f;
    }

    Ray3 get_ray(float s, float t) const {
        const auto rd = lens_radius * random_in_unit_disk();
        const auto offset = u * rd.x() + v * rd.y();
        return Ray3{origin + offset, lower_left_corner + s * horizontal + t * vertical - origin - offset};
    }
protected:
private:
    Point3 origin;
    Point3 lower_left_corner;
    Vector3 horizontal;
    Vector3 vertical;
    Vector3 u;
    Vector3 v;
    Vector3 w;
    float lens_radius;
};
