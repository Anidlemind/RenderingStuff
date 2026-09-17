# Lights, textures, and smoother edges

The default lighting gives objects a bright side, a dark side, and a small
highlight. Textures are softened a little so they look less noisy from a distance.

## Try a different look

```sh
./run.sh --scene shrimp --shading unlit
```

There are three lighting choices:

| Option | Look |
| --- | --- |
| `--shading phong` | The default lighting, with highlights |
| `--shading unlit` | Just colors and textures, without lighting |
| `--shading legacy` | The older, simpler lighting |

For deliberately pixelated textures, try `--filter nearest`. The usual setting
is `trilinear`; `bilinear` is another option to compare. Texture differences
are easiest to see on the shrimp. The older lighting always uses `nearest`.

## Shadows

Add `--shadows` at launch, or press F2 while looking around. Shadows work with
the default lighting. They can still look rough in places, especially when you
move close to an object.

`--shadow-size 2048` gives them more detail than the default 1024, but takes
more time to draw.

## Smoother edges

Press F3, or start with `--aa ssaa4`. This draws a larger picture and shrinks
it down, making edges look smoother. It does more work for every frame, so
it's a noticeable trade-off on a CPU renderer.

```sh
./run.sh --width 1280 --height 720 --shadows --aa ssaa4
```

If movement starts feeling sluggish, press F3 again or use `--aa none`.
[More about speed](performance.md).
