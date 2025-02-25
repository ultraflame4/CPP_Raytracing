
#include "./logging.hpp"

#include "camera.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "objects.hpp"


int main() {

    // World
    HittableList world;

    shared_ptr<Material> mat_a = make_shared<Lambertian>(Color(0.8, 0.8, 0.8));
    shared_ptr<Material> mat_b = make_shared<Metal>(Color(0.1, 0.2, 0.5));
    shared_ptr<Material> mat_c = make_shared<Metal>(Color(0.1, 0.2, 0.5), 0.5);

    world.add(make_shared<Sphere>(Point3(1, 0, -1), 0.5, mat_c));
    world.add(make_shared<Sphere>(Point3(-1, 0, -1), 0.5, mat_b));
    world.add(make_shared<Sphere>(Point3(0, -100.5, -1), 100, mat_a));

    Camera cam;
    cam.samples_per_pixel = 1;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;

    cam.samples_per_pixel = 50;
    cam.output_file = "output_quality_debug.ppm";
    cam.render(world);

    // cam.samples_per_pixel = 400;
    // cam.max_depth = 50;
    // cam.output_file = "output_quality_high.ppm";
    // cam.render(world);
}