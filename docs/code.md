# Code

|  | |
| --- | --- |
| `src/scene` | Shapes, objects, and the camera |
| `src/render` | Drawing triangles, lighting, and shadows |
| `src/assets` | Loading models and textures |
| `src/app` | Startup options and the main loop |
| `src/sdl_platform` | The window, keyboard, and mouse |
| `tests` | Checks for the drawing code and file loaders |

To change the starting arrangement of shapes, look at `MakePrimitivesScene()`
in [scene.cpp](../src/scene/scene.cpp). The shapes themselves are in
[mesh.cpp](../src/scene/mesh.cpp).

## Run the tests

```sh
cmake -S . -B build-tests -DRENDERER_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

The tests catch things like broken model loading, missing triangles, and
unexpected changes to the picture.

For debugging the viewer itself, use:

```sh
BUILD_TYPE=Debug BUILD_DIR=build-debug ./run.sh
```
