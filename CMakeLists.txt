cmake_minimum_required(VERSION 3.16)
project(softrenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(RENDERER_BUILD_APP "Build the SDL viewer" ON)
option(RENDERER_ENABLE_IPO "Optimize across translation units in release builds" ON)
if(RENDERER_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT RENDERER_IPO_SUPPORTED LANGUAGES CXX)
    if(RENDERER_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    endif()
endif()
find_program(RENDERER_CLANG_FORMAT NAMES clang-format-18 clang-format)
if(RENDERER_CLANG_FORMAT)
    file(GLOB_RECURSE RENDERER_STYLE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    add_custom_target(format
        COMMAND ${RENDERER_CLANG_FORMAT} -i ${RENDERER_STYLE_FILES}
        COMMENT "Apply Google-based formatting to project sources (not third_party)"
        VERBATIM)
    add_custom_target(check-format
        COMMAND ${RENDERER_CLANG_FORMAT} --dry-run --Werror ${RENDERER_STYLE_FILES}
        COMMENT "Check Google-based formatting" VERBATIM)
endif()
include(CTest)
find_package(Threads REQUIRED)

function(renderer_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(softrenderer STATIC
    src/math/math.cpp
    src/render/rasterizer.cpp
    src/render/prepared_triangle.cpp
    src/render/shading.cpp
    src/render/shadow_map.cpp
    src/render/frame_buffer.cpp
    src/render/projection.cpp
    src/render/clipping.cpp
    src/render/software_renderer.cpp
    src/ui/text.cpp
    src/assets/obj_loader.cpp
    src/assets/mtl_loader.cpp
    src/assets/texture.cpp
    src/scene/camera.cpp
    src/scene/camera_controller.cpp
    src/scene/scene.cpp
    src/scene/mesh.cpp
)

target_include_directories(softrenderer PUBLIC src)
target_include_directories(softrenderer SYSTEM PRIVATE third_party)
target_link_libraries(softrenderer PUBLIC Threads::Threads)
renderer_warnings(softrenderer)

add_library(renderer_cli STATIC src/app/options.cpp src/app/headless.cpp)
target_link_libraries(renderer_cli PUBLIC softrenderer)
target_compile_definitions(renderer_cli PRIVATE RENDERER_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
renderer_warnings(renderer_cli)
add_executable(renderer_headless src/main.cpp)
target_link_libraries(renderer_headless PRIVATE renderer_cli)
renderer_warnings(renderer_headless)

add_library(renderer_fixtures STATIC src/scene/benchmark_scenes.cpp)
target_link_libraries(renderer_fixtures PUBLIC softrenderer)
target_compile_definitions(renderer_fixtures PRIVATE RENDERER_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
renderer_warnings(renderer_fixtures)
add_executable(renderer_benchmark src/benchmark_main.cpp)
target_link_libraries(renderer_benchmark PRIVATE renderer_fixtures)
target_compile_definitions(renderer_benchmark PRIVATE RENDERER_BUILD_CONFIG="$<CONFIG>")
renderer_warnings(renderer_benchmark)

if(RENDERER_BUILD_APP)
    find_package(SDL2 REQUIRED)
    add_executable(renderer src/main.cpp src/app/app.cpp src/sdl_platform/window.cpp)
    target_link_libraries(renderer PRIVATE renderer_cli SDL2::SDL2)
    target_compile_definitions(renderer PRIVATE RENDERER_WITH_SDL=1)
    renderer_warnings(renderer)
endif()

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
    add_executable(renderer_tests tests/renderer_tests.cpp tests/pipeline_tests.cpp tests/benchmark_tests.cpp tests/optimization_tests.cpp tests/shading_tests.cpp tests/scene_tests.cpp)
    target_link_libraries(renderer_tests PRIVATE renderer_cli renderer_fixtures GTest::gtest_main)
    target_compile_definitions(renderer_tests PRIVATE RENDERER_TEST_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    renderer_warnings(renderer_tests)
    gtest_discover_tests(renderer_tests DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)
    add_test(NAME headless_smoke COMMAND renderer_headless --width 96 --height 64 --workers 2 --frames 2 --output headless-smoke.ppm)
    set_tests_properties(headless_smoke PROPERTIES TIMEOUT 30)
    add_test(NAME benchmark_smoke COMMAND renderer_benchmark --scene all --width 64 --height 64 --workers 2 --warmup 1 --frames 2 --report benchmark-smoke.csv)
    set_tests_properties(benchmark_smoke PROPERTIES TIMEOUT 30)
    add_test(NAME primitives_headless_smoke COMMAND renderer_headless --scene primitives --shadows --aa ssaa4 --width 96 --height 64 --workers 2 --frames 2 --output primitives-smoke.ppm)
    set_tests_properties(primitives_headless_smoke PROPERTIES TIMEOUT 30)
    add_test(NAME shrimp_headless_smoke COMMAND renderer_headless --scene shrimp --width 96 --height 64 --workers 2 --frames 2 --output shrimp-smoke.ppm)
    set_tests_properties(shrimp_headless_smoke PROPERTIES TIMEOUT 30)
endif()
