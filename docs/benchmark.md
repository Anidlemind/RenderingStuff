# Timing a render

There's a small benchmark program for comparing changes or trying different
settings.

## Build without a window

```sh
cmake -S . -B build-headless -DRENDERER_BUILD_APP=OFF -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless --parallel
```

It also gives you `renderer_headless`, which can save a picture:

```sh
./build-headless/renderer_headless --width 1280 --height 720 --shadows --output scene.ppm
```

## Measure

```sh
./build-headless/renderer_benchmark --scene primitives --shading phong \
  --shadows --width 1280 --height 720 --workers 4 \
  --warmup 20 --frames 100 --report build-headless/timings.csv
```

This lets the renderer settle for a few frames, then measures another 100.
The measuring generates a csv file.
Leave out `--report` to print numbers in the terminal instead.

Try `--scene shrimp` for the model, or `--scene all` for all the small test
scenes. Use `--help` for the other options.
