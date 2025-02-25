cmake_minimum_required(VERSION 3.28)
project(
    raytracer
    VERSION 0.0.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_CXX_STANDARD 23)

find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)



add_subdirectory(app)