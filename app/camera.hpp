#pragma once

#include "hittable.hpp"
#include "material.hpp"
#include "math.hpp"
#include "ray.hpp"
#include <algorithm>
#include <execution>
#include <fstream>
#include <numeric>
#include <vector>

inline double linear_to_gamma(double linear_component) {
    if (linear_component > 0)
        return std::sqrt(linear_component);

    return 0;
}

inline void write_color(std::ostream &out, const Color &pixel_color) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Apply a linear to gamma transform for gamma 2
    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    // Translate the [0,1] component values to the byte range [0,255].
    static const Interval intensity(0.000, 0.999);
    int rbyte = int(256 * intensity.clamp(r));
    int gbyte = int(256 * intensity.clamp(g));
    int bbyte = int(256 * intensity.clamp(b));

    // Write out the pixel color components.
    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}


struct CameraRay {
    Color *output_pixel;
    int x;
    int y;
};

struct CameraRayScatter{
    Color color;
    bool should_continue;
    Ray next;
};

class Camera {
    public:
    /* Public Camera Parameters Here */

    double aspect_ratio = 1.0;              // Ratio of image width over height
    int image_width = 100;                  // Rendered image width in pixel count
    int samples_per_pixel = 10;             // Count of random samples for each pixel
    int max_depth = 5;                      // Maximum ray recursion depth
    
    std::string output_file = "output.ppm"; // Output file name

    void render(const Hittable &world) {
        initialize();

        // Calculate all the ray instances we need to cast.

        std::vector<Color> pixels(image_height * image_width, Color(0, 0, 0));
        std::vector<CameraRay> rays;
        rays.reserve(image_height * image_width);

        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                auto out_pixel = &pixels[j * image_width + i];
                rays.push_back(CameraRay(out_pixel, i, j));
            }
        }

        std::mutex m;

        int rays_remaining = rays.size();
        // Simulate the rays
        std::for_each(std::execution::par_unseq, rays.begin(), rays.end(), [&](CameraRay &r) {
            Color pixel_color(0, 0, 0);

            for (int sample = 0; sample < samples_per_pixel; sample++) {
                // -- SHOOT RAY --
                Ray ray = get_ray(r.x, r.y, sample);
                auto next = shoot_ray(ray, world);
                auto r_color = next.color; // Track color for the current ray
                for (int d = 1; d < max_depth; d++) {
                    
                    if (!next.should_continue) {
                        break;
                    }

                    next = shoot_ray(next.next, world);
                    r_color = r_color * next.color; // Accumulate color
                }
                // -- END SHOOT RAY --
                
                pixel_color += r_color; // Sum color of all samples
            }

            *r.output_pixel = pixel_color * pixel_samples_scale;


            rays_remaining--;
            if (rays_remaining % 100 == 0) {
                m.lock();
                std::clog << "\rScanlines remaining: " << rays_remaining << std::flush;
                m.unlock();
            }

        });

        // std::vector<int> rowIndices(image_height);

        // std::iota(rowIndices.begin(), rowIndices.end(), 0);

        // std::vector<std::vector<Color>> imageBuffer(image_height,
        //                                             std::vector<Color>(image_width));

        // int rowsRemaining = image_height;
        // std::for_each(
        //     std::execution::seq, rowIndices.begin(), rowIndices.end(), [&](int &rowIndex) {
        //         auto row = imageBuffer[rowIndex];

        //         for (int i = 0; i < (int)row.size(); i++) {
        //             Color pixel_color(0, 0, 0);
        //             for (int sample = 0; sample < samples_per_pixel; sample++) {
        //                 Ray r = get_ray(i, rowIndex, sample);
        //                 pixel_color += ray_color(r, world);
        //             }
        //             imageBuffer[rowIndex][i] = pixel_samples_scale * pixel_color;
        //         }
        //         rowsRemaining--;

        //         m.lock();
        //         std::clog << "\rScanlines remaining: " << rowsRemaining << ' ' << std::flush;
        //         m.unlock();
        //     });
        // for (int j = 0; j < image_height; j++) {
        //     std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' <<
        //     std::flush;

        // }

        std::ofstream output_stream(output_file);

        output_stream << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        // Write the pixel color values to the output file.
        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; ++i) {
                write_color(output_stream, pixels[j * image_width + i]);
            }
        }

        output_stream.close();
        std::clog << "\rDone.                 \n";
    }

    private:
    int image_height;           // Rendered image height
    double pixel_samples_scale; // Color scale factor for a sum of pixel samples
    Point3 center;              // Camera center
    Point3 pixel00_loc;         // Location of pixel 0, 0
    Vec3 pixel_delta_u;         // Offset to pixel to the right
    Vec3 pixel_delta_v;         // Offset to pixel below

    void initialize() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;

        center = Point3(0, 0, 0);

        // Determine viewport dimensions.
        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = Vec3(viewport_width, 0, 0);
        auto viewport_v = Vec3(0, -viewport_height, 0);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left =
            center - Vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    Ray get_ray(int i, int j, int sample_index) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i, j.

        auto offset = sample_square();
        if (sample_index == 0) {
            offset = Vec3(0, 0, 0);
        }

        auto pixel_sample = pixel00_loc + ((i + offset.x()) * pixel_delta_u) +
                            ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = center;
        auto ray_direction = pixel_sample - ray_origin;

        return Ray(ray_origin, ray_direction);
    }

    Vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return Vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    CameraRayScatter shoot_ray(const Ray &r, const Hittable &world) const {
        HitRecord rec;

        if (world.hit(r, Interval(0.0001, infinity), rec)) {

            // Old code -> Randomly Scatter Rays
            // Vec3 direction = rec.normal + random_unit_vector();
            // return 0.5 * ray_color(Ray(rec.p, direction), world, current_depth + 1);

            // New code -> Scatter Rays based on Material
            Ray scattered;
            Color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered)) {
                return CameraRayScatter(attenuation, true, scattered);
            }

            // If the ray is not scattered, return black. (Fully absorbed)
            return CameraRayScatter(Color(0, 0, 0), false);
        }

        Vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0);
        
        auto skyColor = (1.0 - a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
        return CameraRayScatter(skyColor, false);
    }
};