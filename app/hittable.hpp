#pragma once

#include <cassert>
#include <memory>
#include <mutex>
#include <vector>

#include "interval.hpp"
#include "math.hpp"
#include "ray.hpp"

using std::make_shared;
using std::shared_ptr;

class Material;

class HitRecord {
    public:
    Point3 p;
    Vec3 normal;
    double t;
    bool front_face;
    shared_ptr<Material> mat;

    /**
     * @brief Set the face normal object using the given ray and outward  normal (the normal
     * calculated).
     */
    void set_face_normal(const Ray &r, const Vec3 &outward_normal) {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Hittable {
    public:
    virtual ~Hittable() {}
    virtual bool hit(const Ray &r, Interval ray_t, HitRecord &rec) const = 0;
};

class HittableList : public Hittable {
    private:
    std::mutex m;
    bool is_locked = false;
    std::vector<shared_ptr<Hittable>> objects;
    public:

    HittableList() {}
    HittableList(shared_ptr<Hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<Hittable> object) {
        lock();
        objects.push_back(object); 
        unlock();
    }

    void lock() {
        m.lock();
        is_locked = true;
    }

    void unlock() {
        m.unlock();
        is_locked = false;
    }

    bool hit(const Ray &r, Interval ray_t, HitRecord &rec) const override {
        
        if (!is_locked) {
            throw std::runtime_error("HittableList is not locked!. Please lock the list before calling hit. This is to ensure thread safety.");
        }

        HitRecord temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (int i = 0; i < objects.size(); i++) {
            auto object = objects[i].get();
            if (object->hit(r, Interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        // Old code for the loop above. Below is slower because it involves copying the pointer which will do stuff like mutex locking,
        //  obliterating performance in multithreaded environments.
        // ---
        // for (const auto &object : objects) {
        //     if (object->hit(r, Interval(ray_t.min, closest_so_far), temp_rec)) {
        //         hit_anything = true;
        //         closest_so_far = temp_rec.t;
        //         rec = temp_rec;
        //     }
        // }

        return hit_anything;
    }
};