# Scenes and models

The default scene is a cube, a sphere, and a pyramid on a plane. Use Tab to
pick an object, then move it with the arrow keys. The full controls are in
[the README](../README.md).

You can choose a scene when starting the viewer:

```sh
./run.sh --scene primitives
./run.sh --scene shrimp
```

## Bring your own model

```sh
./run.sh --model path/to/model.obj
```

The viewer centers the model and adjusts its size so it fits the scene.
Keep any accompanying MTL files and textures alongside it in the folders the
model expects. Simple opaque models work best; transparency isn't supported.

Use either `--model` or `--scene` in a command. A model's materials normally
supply its textures, but you can replace them with `--texture image.png` or
hide them with `--no-texture`.

## See the scene differently

Press F1 to cycle through the extra views, or pick one at launch:

```sh
./run.sh --debug wireframe
```

| View | What you see |
| --- | --- |
| `none` | The usual picture |
| `wireframe` | The triangles making up the objects |
| `normals` | Colors showing which way each surface faces |
| `depth` | Nearer surfaces are brighter |
| `overdraw` | Colors showing where triangles overlap |

## Save a picture

To draw a frame without opening a window:

```sh
./run.sh --headless --width 1280 --height 720 --shadows --output scene.ppm
```

The picture is saved as a PPM file, without the FPS counter. It uses the same
starting camera as the viewer. There's also a [build without SDL](benchmark.md)
if you only want to render pictures or measure speed.
