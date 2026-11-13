# Updated native CutOut runner

This 2026 port compiles the Android application's recovered C++ segmentation operators with OpenCV 4. It adapts Elad Richardson's recovered CutOut implementation of Lab color-density estimation and pixel-grid Dijkstra or MegaWave2 Fast Level Sets Transform (FLST) followed by Dijkstra on its tree. The FIGTree estimator, its ANN dependency and the histogram estimator are included. Both distance runs use the foreground likelihood, as in the recovered implementation.

The CLI accepts an image and foreground/background scribbles, then writes a binary mask, transparent PNG, contour overlay, likelihood visualization and JSON run metadata. Optional boundary refinement calls the recovered `generateAlpha` operator with corrections for degenerate distances. This is separately labeled updated work. The [original Android snapshot](../../source/original/android-app/) remains unchanged.

## Build and run

Requirements: CMake 3.22+, Ninja, a C++17 compiler, and OpenCV 4 development headers/libraries for `core`, `imgproc` and `imgcodecs`. ANN and FIGTree retain C++14 compilation for their historical `register` and `random_shuffle` interfaces. The core library uses C++17. Android can link the same `cutout_core` CMake target with CLI and tests disabled; see the [updated Android guide](../android/README.md).

```sh
cmake -S updated/native -B build/native -G Ninja \
  -DOpenCV_DIR=/path/to/opencv/lib/cmake/opencv4 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/native --parallel 2
ctest --test-dir build/native --output-on-failure
build/native/cutout_cli --help
build/native/cutout_cli --image photograph.png --scribbles scribbles.json \
  --output-dir build/cutout --density figtree --distance pixel --alpha refined
```

The same commands support Clang/GCC sanitizers with `-DCUTOUT_SANITIZE=ON`. This instruments the port and bundled ANN/FIGTree/MegaWave sources. An ordinary external OpenCV installation is not instrumented by that switch.

`scribbles.json` contains strokes in full-resolution integer `[x,y]` image coordinates, with an optional brush radius in pixels:

```json
{
  "foreground": [[[120, 80], [140, 95]]],
  "background": [[[10, 10], [10, 150]]],
  "brush_radius": 2
}
```

Alternatively supply same-size grayscale PNG masks using `--foreground foreground.png --background background.png`; each nonzero pixel is a seed. Both labels must be present and disjoint. Empty strokes are ignored; isolated points are supported. Out-of-image coordinates, overlapping classes and conflicts caused by image reduction are rejected.

Each successful run writes:

- `mask.png`: full-resolution binary object mask, values 0 or 255.
- `cutout.png`: full-resolution color PNG with the requested alpha channel.
- `outline.png`: full-resolution original BGR image with green mask contours.
- `alpha.png`: the alpha channel encoded as grayscale bytes.
- `foreground_likelihood.png`: likelihood scaled to bytes, at segmentation working resolution.
- `result.json`: selected operators, input/working dimensions, foreground-pixel count, runtime OpenCV version (`opencv_version`), compile-time header version (`opencv_header_version`) and elapsed seconds for this invocation.

Default density is FIGTree and default distance is pixel-grid Dijkstra. `--distance levelset` selects the recovered FLST/tree path. `--density histogram` selects the original JNI's 16-bin, blur-mode-8 histogram settings; use `--histogram-bins` and `--histogram-blur` to change these. FIGTree retains bandwidth 10 Lab units and approximation tolerance `1e-1` from the recovered call. Its clustering and method-selection samples now use a fixed-seed, thread-local random generator reset for each density evaluation. This replaces the recovered time-seeded global random calls so repeated inputs on the tested build produce the same result. It does not promise bitwise equivalence across OpenCV versions, architectures or compiler settings. A completely constant Lab domain uses its exact Gaussian sum because the original automatic parameter selection rejects zero color range.

Images with a longest side above 400 pixels are reduced by area interpolation in floating-point BGR before segmentation. The mask is enlarged with cubic interpolation, thresholded, box-smoothed with the recovered `5/scale` kernel rule and thresholded again. `--max-dimension 0` disables reduction; `--no-smooth` disables only the enlargement's box smoothing. Explicit scribble labels are enforced on the final full-resolution mask and alpha, which changes historical behavior on ties and after smoothing. Foreground and background seeds that merge during reduction are rejected rather than silently assigned to the same working pixel.

The default `--alpha hard` copies the binary mask into alpha. `--alpha refined` runs recovered contour-band refinement at full resolution using `--alpha-band 3` pixels by default, intensity 1.5 and no foreground-color replacement. This smaller fixed band is an updated choice; the recovered JNI requested a band of `10/scale`. Refinement can leave contours unchanged when there are no usable paired band contours. Output remains full-size rather than applying the historical crop.

## Isolated OpenCV installation

The first Mac build found an existing Homebrew OpenCV 4.12 installation with an unavailable OpenEXR 3.3 dylib and a TBB crash during process shutdown. No global package was changed. A separate OpenCV 4.12 installation was built with only `core`, `imgproc` and `imgcodecs`, bundled PNG/JPEG/zlib, and no TBB, OpenEXR or AVIF dependency.

Download [OpenCV's 4.12.0 source archive](https://github.com/opencv/opencv/archive/refs/tags/4.12.0.tar.gz), verify SHA-256 `44c106d5bb47efec04e531fd93008b3fcd1d27138985c5baf4eafac0e1ec9e9d`, and extract it. Keep build/install directories on a filesystem supporting normal compiler and library semantics. The archive can live on an external data drive.

```sh
bash updated/native/scripts/build_opencv.sh \
  /path/to/opencv-4.12.0 /path/to/opencv-build /path/to/opencv-install
```

Then point `OpenCV_DIR` at `/path/to/opencv-install/lib/cmake/opencv4`. The script is the recorded minimal-build recipe; it does not download files or alter system packages. OpenCV's [configuration reference](https://docs.opencv.org/4.x/db/d05/tutorial_config_reference.html) describes the options.

## Changes and limits

[PROVENANCE.json](PROVENANCE.json) maps every copied algorithm/dependency file to its original path and hash, records updated hashes and lists modifications. Port changes include portable logging, OpenCV include updates, a const-correct queue comparator, empty-stroke handling, bounded Lab bins, correct row-stride handling in the histogram lookup, symmetric histogram-kernel offsets, finite alpha at zero/unreachable distances, inclusive/empty crop bounds, a corrected optional background-sample index, RAII density buffers and error propagation, constant-color density handling, repeatable FIGTree choices, explicit signed MegaWave hole counts on Android, and iterative tree-edge extraction to avoid recursive traversal depth. The new wrapper adds validation, stroke rasterization, deterministic seed-label enforcement and serialization around legacy global state.

The [validation guide](../validation/README.md) records measured checks. These tests cover specified native operators and generated fixtures. They do not establish photographic segmentation accuracy, reproduce published benchmark results or validate the historical Java editing application. The new Android shell and desktop UI use this core, with their own build/run records. The original `ADD_FRONT`/`ADD_BACK` cached-PDF interaction and blur, grayscale, drag/compositing tools are not exposed in this CLI; each invocation recomputes a segmentation from all supplied seeds.

Input images are limited to 40 million pixels; the longest working side is configurable up to 4096 pixels when reduction is enabled. With reduction disabled, level-set dimensions must still fit the recovered signed 16-bit coordinate representation (at most 32767 pixels per side). Full-resolution refinement may cost substantially more than reduced segmentation. Legacy allocation failures and very large images have not been exhaustively tested.

## Attribution and terms

The recovered CutOut application implementation is by Elad Richardson; the wrapper and fixes are updated 2026 work by Codex for Aaron Wetzler. The historical poster cites Bai and Sapiro's geodesic segmentation method and Dubrovina, Hershkovitz and Kimmel's level-set-tree image-editing method. Those underlying methods and the third-party libraries have their own authors; this port does not establish exact reproduction of either paper. Original project supervision and interface credits remain in the repository. FIGTree credits Vlad Morariu, Vikas Raykar and Changjiang Yang; ANN credits Sunil Arya, David Mount and the University of Maryland. FLST credits Pascal Monasse, and MegaWave2 system files credit their original authors and CMLA, Ecole Normale Superieure de Cachan, 61 avenue du President Wilson, 94235 Cachan cedex, France.

Original notices are retained in the copied files. See the [source and license guide](../../docs/original-android.md#attribution-and-terms) and [supplementary terms](../../docs/licenses/README.md). The archive contains mixed terms; this port does not assign a blanket repository license. Elad's permission to host recovered code does not replace third-party notices.
