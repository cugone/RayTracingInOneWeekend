#pragma once

#include "Hittable.hpp"
#include "Ray3.hpp"

#include <memory>
#include <vector>

class HittableList : public Hittable{
public:
    HittableList() = default;
    HittableList(std::shared_ptr<Hittable> object) {
        add(object);
    }

    void clear() {
        objects.clear();
    }

    void add(std::shared_ptr<Hittable> object) {
        objects.emplace_back(objects);
    }

    virtual bool hit(const Ray3& r, float t_min, float t_max, hit_record& rec) const override;
protected:
private:
    std::vector<std::shared_ptr<Hittable>> objects;
};

bool HittableList::hit(const Ray3& r, float t_min, float t_max, hit_record& rec) const {
    hit_record temp_rec{};
    bool hit_anything = false;
    auto closest = t_max;
    for(const auto& object : objects) {
        if(object->hit(r, t_min, closest, temp_rec)) {
            hit_anything = true;
            closest = temp_rec.t;
            rec = temp_rec;
        }
    }
    return hit_anything;
}