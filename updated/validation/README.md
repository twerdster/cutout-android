# Native validation tools (updated 2026)

These checks exercise the updated native port of the recovered Android operators. They do not build the historical APK, verify an Android interface or establish photographic segmentation accuracy. Originals under `source/original/android-app/` are read without changes.

[VALIDATION.json](VALIDATION.json) records the 2026-11-13 macOS run: 31 native checks passed with address/undefined-behavior sanitizers, the same 31 passed in a separate `-funsigned-char` build, 25 CLI cases passed, and six isolated original-method checks failed on the recovered bodies and passed on the updated implementation. The sanitizer switch covered the port and bundled ANN/FIGTree/MegaWave sources; the separate OpenCV build was not instrumented. Linux execution is not recorded here. Android build and runtime results belong to the [Android guide](../android/README.md).

## Native regressions

The native CMake project builds `cutout_regressions` from [native_regressions.cpp](native_regressions.cpp). It reports 31 named checks. Several checks contain multiple inputs: the operator checks cover both density estimators, both propagation methods and both alpha modes; one stress check runs ten small deterministic random images through four density/propagation combinations with refined alpha. A passing check count is not a number of benchmark photographs.

The checks cover empty scribble paths, line filling, inclusive alpha crops, Lab histogram bounds and row strides, analytic and independently relaxed pixel-grid distances, disconnected masks, finite alpha at zero/unreachable distances, constant level-set trees, API input rejection, reduction collisions, full-size restoration, thin images, strided image/mask inputs and repeatability. Ring and nested-square level-set inputs have known shape counts, parent areas, gray levels and edge-distance sums; they exercise the negative topology-table entries whose meaning must survive unsigned plain `char` builds. Explicit foreground/background labels must survive reduction and mask smoothing.

After building the native project:

```sh
ctest --test-dir build/native --output-on-failure
build/native/cutout_regressions crop_single_pixel
```

Use the actual build directory if it differs. Run an address/undefined-behavior sanitizer build as described in the native guide before making memory-safety claims.

## Original-failure comparisons

[verify_original_failures.py](verify_original_failures.py) verifies four original source/header hashes against `source/ANDROID_PROVENANCE.json`, extracts selected original function bodies unchanged and compiles an isolated driver with address/undefined-behavior sanitizers. Each unsafe case runs in a separate process. It compares the original result with the corresponding updated regression for empty paths, empty/single/full crops, Lab clamping and degenerate alpha distances.

```sh
python3 updated/validation/verify_original_failures.py \
  --updated-regressions build/native/cutout_regressions \
  --report build/original-failure-comparison.json
```

This requires a C++17 compiler and OpenCV development headers/libraries. By default, flags come from `pkg-config opencv4`. Use `--opencv-prefix /path/to/opencv/install` for a dedicated OpenCV installation. These are isolated function comparisons, not execution or binary equivalence of the original Android app.

## CLI and photograph checks

[check_cli.py](check_cli.py) needs Python and Pillow. It generates small procedural inputs in its output directory and uses the unchanged CC0 [coffee photograph](fixtures/README.md). For example, from an environment with Pillow installed:

```sh
python3 updated/validation/check_cli.py \
  --binary build/native/cutout_cli \
  --output-dir build/native-validation
```

It runs all four density/propagation combinations with hard and refined alpha on one photograph, repeats one run to check mask determinism, exercises constant images and reduced/unreduced procedural inputs, and checks rejection of malformed or contradictory inputs. PNG formats, dimensions, binary masks, alpha channels, source scribble coordinates and recorded pixel counts must agree. Execution timing in `result.json` is elapsed wall time in seconds for that call; it is not a comparative performance benchmark.

The photograph has no independent reference mask. The scribbles identify the requested foreground; they are neither a complete object annotation nor a substitute for one. No photo IoU or accuracy value is computed. Visual inspection against a checkerboard showed retained wood above the cup in the pixel-grid examples, plus ragged edges and small background fragments around some saucer boundaries. The level-set examples removed more of that particular background region with these strokes; one selected photograph does not establish a general quality comparison. See [fixture provenance](fixtures/PROVENANCE.json) for the photographer, CC0 source, pinned URL and SHA-256.
