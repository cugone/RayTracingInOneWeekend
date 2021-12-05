#include "Vector3.hpp"

Vector3 random_in_unit_sphere() {
    for(;;) {
        const auto p = Vector3::random(-1.0f, 1.0f);
        if(p.length_squared() > 1.0f) continue;
        return p;
    }
}

Vector3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}
