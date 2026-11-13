# Local drawing interface (updated 2026)

This interface accepts an image and foreground/background strokes, runs the updated recovered CutOut C++ implementation, and displays its mask and transparent PNG. It offers FIGTree or Lab-histogram color density estimation, pixel-grid or level-set-tree distance propagation, and hard or boundary-refined alpha.

The drawing interface selects boundary-refined alpha initially; the CLI defaults to hard alpha. Editing strokes or changing settings hides the previous result until another run finishes.

It is a new interface around the recovered native operators, separate from the historical Android interface and the earlier Python fast-marching exercise. See the [native guide](../native/README.md) for dependencies, fixes and build commands. Original code and third-party notices remain preserved in the repository.

## Run

Build the native CLI first. From the repository root, with Python 3.10 or later:

```sh
python3 updated/desktop/server.py --binary build/native/cutout_cli --port 8768
```

Open `http://127.0.0.1:8768/`. The server binds to the local loopback interface. Images and outputs stay on this computer; it makes no requests to an external segmentation service. Stop it with Ctrl-C.

1. Choose an image, or load the supplied coffee photograph with demonstration strokes.
2. Mark the desired object with green foreground strokes and excluded areas with red background strokes. A later stroke replaces the other class where they cross. Undo or clear strokes as needed.
3. Choose density, distance and alpha options, then run segmentation.
4. Inspect the mask and contour, add more strokes if needed, and download the transparent PNG.

Uploads are resized in the browser to at most 1,600 pixels on their longest side. The native core normally segments at a 400-pixel working size and restores the output dimensions; alpha refinement uses the full uploaded image. Stroke coordinates refer to that uploaded image. The interface runs one native process at a time and ends a request after 90 seconds.

If nearby opposing strokes merge at the reduced working size, separate them or select a larger working size, including **Full input**. Larger working images require more processing time and memory. The native API rejects ambiguous labels rather than silently removing them.

The coffee photograph and stroke provenance are in [the fixture guide](../validation/fixtures/README.md). This is one photograph without an independent reference segmentation; the example is not an accuracy benchmark. Results depend on the scribbles and selected algorithm.
