#pragma once

#include <memory>
#include <vector>

#include "interval.hpp"
#include "math.hpp"
#include "ray.hpp"

using std::make_shared;
using std::shared_ptr;

class Material;

class HitRecord{
    public:
    Point3 p;
    Vec3 normal;
    double t;
    bool front_face;
    shared_ptr<Material> mat;

    /**
     * @brief Set the face normal object using the given ray and outward  normal (the normal calculated).
     */
    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Hittable{
    public:
    virtual ~Hittable() {}
    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const = 0;
};


class HittableList: public Hittable{
    public:
    std::vector<shared_ptr<Hittable>> objects;

    HittableList() {}
    HittableList(shared_ptr<Hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<Hittable> object) {
        objects.push_back(object);
    }

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {
        HitRecord temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto& object : objects) {
            if (object->hit(r, Interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }
};