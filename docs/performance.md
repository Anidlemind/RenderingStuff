# Performance

The picture is drawn on the CPU, so window size and extra effects make a big
difference.

```sh
./run.sh --width 1280 --height 720 --shadows
```

That gives you 720p with shadows and no extra edge smoothing.

## A few things to try

- Turn off smoothing with F3 if you enabled it. It draws four times as many
  samples for each frame.
- Toggle shadows with F2 to see how much they cost on your machine.
- Use the normal `./run.sh` build for playing around. Debug builds are slower.
- The viewer picks a thread count automatically. You can try `--workers 4`
  if you want to compare it with a fixed number.

## How fast is it now?

A local run reached about 72 FPS with the shapes at 720p, shadows on, and
smoothing off. That was a Release build on a Ryzen 7 5800H host with four
available logical CPUs. It included drawing and SDL presentation.
