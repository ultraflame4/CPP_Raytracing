add_executable(main main.cpp)

target_link_libraries(main PRIVATE spdlog::spdlog)
target_link_libraries(main PRIVATE fmt::fmt)