# CPP Ray Tracer

A simple raytracer I followed using [_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html).

This version of the raytracer is multi-threaded using `std::for_each(std::execution::par_unseq...`, as such some parts of it will differ greatly from the tutorial,
especiall the camera `render` and `ray_color` part.

