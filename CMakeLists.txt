cmake_minimum_required(VERSION 3.16)
project(softrenderer CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(softrenderer STATIC
    src/math/math.cpp
    src/render/rasterer.cpp
)

target_include_directories(softrenderer PUBLIC src)

add_executable(renderer
    src/scene/main.cpp
)
target_link_libraries(renderer PRIVATE softrenderer)

set(WARN_FLAGS -Wall -Wextra -Wpedantic)

target_compile_options(softrenderer PRIVATE ${WARN_FLAGS})
target_compile_options(renderer     PRIVATE ${WARN_FLAGS})