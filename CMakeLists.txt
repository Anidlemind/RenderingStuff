cmake_minimum_required(VERSION 3.16)
project(softrenderer CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(softrenderer STATIC
    src/math/math.cpp
    src/render/rasterer.cpp
    src/render/frame_buffer.cpp
    src/render/projection.cpp
    src/render/text.cpp
    src/render/obj_loader.cpp
    src/render/texture.cpp
    src/sdl_platform/window.cpp
    src/scene/camera.cpp
    src/scene/camera_controller.cpp
    src/scene/scene.cpp
    src/app/app.cpp
)

find_package(SDL2 REQUIRED)

target_link_libraries(softrenderer PUBLIC SDL2::SDL2)

target_include_directories(softrenderer PUBLIC src third_party)
target_include_directories(softrenderer PUBLIC src)

add_executable(renderer
    src/main.cpp
)
target_link_libraries(renderer PRIVATE softrenderer)

set(WARN_FLAGS -Wall -Wextra -Wpedantic)

target_compile_options(softrenderer PRIVATE ${WARN_FLAGS})
target_compile_options(renderer     PRIVATE ${WARN_FLAGS})