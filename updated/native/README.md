# Native CutOut runner for macOS and Linux

This C++ port runs Elad Richardson's recovered Android segmentation implementation with OpenCV 4. Given an image and foreground/background scribbles, it estimates Lab color densities, propagates distances on a pixel grid or a MegaWave2 level-set tree, and writes a mask and transparent PNG. Boundary-band alpha refinement is optional. The [original Android source](../../source/original/android-app/) remains unchanged; fixes and interfaces in this directory are updated 2026 work.

## Install and build

Run these commands from the repository root. The desktop runner requires CMake 3.22+, Ninja, a C++17 compiler and OpenCV **4** development libraries for `core`, `imgproc` and `imgcodecs`. Python 3.10+ runs the browser interface. The bundled ANN/FIGTree sources are built as C++14; the core and CLI use C++17.

On macOS, install Apple's command-line developer tools and [Homebrew](https://docs.brew.sh/Installation), then configure against the [OpenCV 4 formula](https://formulae.brew.sh/formula/opencv@4):

```sh
brew install cmake ninja opencv@4 python
cmake -S updated/native -B build/native -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DOpenCV_DIR="$(brew --prefix opencv@4)/lib/cmake/opencv4"
```

The recorded macOS validation used the [pinned OpenCV source build](BUILDING_OPENCV.md); the Homebrew recipe is a dependency setup option.

On Ubuntu 24.04, install the compiler, build tools and [OpenCV development package](https://packages.ubuntu.com/noble/libopencv-dev), then configure:

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends \
  build-essential cmake ninja-build libopencv-dev python3
cmake -S updated/native -B build/native -G Ninja -DCMAKE_BUILD_TYPE=Release
```

Build and run the same commands on either platform:

```sh
cmake --build build/native --parallel 2
ctest --test-dir build/native --output-on-failure
build/native/cutout_cli \
  --image updated/validation/fixtures/coffee.png \
  --scribbles updated/validation/fixtures/coffee-scribbles.json \
  --output-dir build/cutout \
  --density figtree --distance levelset --alpha refined
```

Open `build/cutout/cutout.png` to inspect the output. The example photograph is CC0; [fixture provenance](../validation/fixtures/README.md) identifies its photographer and source. It has no independent reference mask.

For interactive drawing and transparent-PNG download, start the [browser interface](../desktop/README.md):

```sh
python3 updated/desktop/server.py --binary build/native/cutout_cli
```

Open `http://127.0.0.1:8768`. Segmentation runs locally through the compiled CLI. Native mobile interfaces use the same core; see the [Android](../android/README.md) and [iOS](../ios/README.md) guides.

If a package manager does not provide a suitable OpenCV 4 installation, use the [optional pinned OpenCV source build](BUILDING_OPENCV.md). An existing installation can be selected with `-DOpenCV_DIR=/path/to/opencv/lib/cmake/opencv4`. OpenCV 5 is not the documented dependency.

## Inputs and outputs

`--image` accepts an image supported by the selected OpenCV build. `--scribbles` reads full-resolution integer `[x, y]` coordinates and a brush radius in pixels:

```json
{
  "foreground": [[[120, 80], [140, 95]]],
  "background": [[[10, 10], [10, 150]]],
  "brush_radius": 2
}
```

Alternatively, use `--foreground foreground.png --background background.png` with same-size grayscale masks; nonzero pixels are seeds. Both classes must be present and disjoint. Empty strokes are ignored and isolated points are supported. Coordinates outside the image, overlapping classes and opposing seeds that merge during reduction are rejected.

The output directory contains:

- `mask.png`: full-resolution binary mask, with values 0 or 255.
- `cutout.png`: full-resolution color PNG with alpha.
- `outline.png`: input image with green mask contours.
- `alpha.png`: alpha encoded as grayscale bytes.
- `foreground_likelihood.png`: likelihood scaled to bytes at the working resolution.
- `result.json`: selected operators, input/working dimensions, foreground-pixel count, runtime and header OpenCV versions, and elapsed seconds for this call.

Run `build/native/cutout_cli --help` for all options.

## Operator choices and changed behavior

`--density figtree` is the default; `--density histogram` uses the recovered JNI's 16-bin, blur-mode-8 settings. `--histogram-bins` and `--histogram-blur` override the latter. FIGTree retains bandwidth 10 Lab units and tolerance `1e-1`. Its random choices now use a fixed-seed local generator reset for each density evaluation. Completely constant Lab domains use the exact Gaussian sum. These changes make the tested build repeatable; they do not promise identical results across library versions or architectures.

`--distance pixel` is the default; `--distance levelset` uses the recovered FLST/tree path. Both foreground and background distance runs use foreground likelihood, as in the recovered implementation.

Images whose longest side exceeds 400 pixels are reduced using area interpolation in floating-point BGR. The mask is enlarged using cubic interpolation, thresholded, box-smoothed with the recovered `5/scale` rule, and thresholded again. `--max-dimension 0` disables reduction; `--no-smooth` disables the enlargement's box smoothing. Explicit labels are enforced on the final mask and alpha, changing historical behavior on ties and after smoothing.

`--alpha hard` copies the binary mask to alpha. `--alpha refined` runs recovered contour-band refinement at full resolution with intensity 1.5 and a default band of 3 pixels, adjustable with `--alpha-band`. The historical JNI used `10/scale`. Refinement can leave a contour unchanged when usable paired band contours are absent. The CLI returns a full-size image instead of applying the historical crop.

## Validation and limits

The Ubuntu installation recipe uses the distribution compiler; the recorded Linux checks used Clang 18, and GCC execution is not recorded. The [validation guide](../validation/README.md) describes the named checks and their scope. The [Ubuntu 24.04 CI run](https://github.com/twerdster/cutout-android/actions/runs/43465063049) built on x86-64 with Clang 18.1.3 and OpenCV 4.6.0. It passed 31 native checks, the same 31 with unsigned plain `char`, 25 CLI cases and six isolated original-method comparisons. The [Linux workflow](../../.github/workflows/native-linux.yml) retains the JSON results, source hashes and logs for subsequent runs.

Add `-DCUTOUT_SANITIZE=ON` to a Clang/GCC configure command to instrument the port and bundled ANN/FIGTree/MegaWave sources. The external OpenCV installation is not instrumented by that switch. Checks cover specified operators and fixtures; they do not establish photographic accuracy, reproduce published benchmarks or validate the historical Java application.

Each call recomputes both PDFs. The recovered cached `ADD_FRONT`/`ADD_BACK` interaction and blur, grayscale and drag/compositing tools are not exposed. Input images are limited to 40 million pixels. With reduction enabled, the configurable longest working side is at most 4096 pixels. Unreduced level-set dimensions must fit the recovered signed 16-bit coordinates, at most 32767 pixels per side. Full-resolution refinement can cost substantially more than reduced segmentation; very large images and allocation failures have not been exhaustively tested.

## Attribution and terms

[PROVENANCE.json](PROVENANCE.json) maps the 48 copied files to their originals, records hashes and identifies changes. The recovered application implementation is by Elad Richardson; the wrappers and fixes are updated 2026 work by Codex for Aaron Wetzler. The historical poster cites Bai and Sapiro's geodesic segmentation method and Dubrovina, Hershkovitz and Kimmel's level-set-tree image-editing method. This port does not establish exact reproduction of either paper.

Original project credits remain. FIGTree credits Vlad Morariu, Vikas Raykar and Changjiang Yang; ANN credits Sunil Arya, David Mount and the University of Maryland. FLST credits Pascal Monasse, and MegaWave2 system files credit their original authors and CMLA, Ecole Normale Superieure de Cachan, 61 avenue du President Wilson, 94235 Cachan cedex, France.

Copied source notices and [supplementary license references](../../docs/licenses/README.md) remain in place. See the [original implementation guide](../../docs/original-android.md#attribution-and-terms). The archive has mixed terms; no blanket repository license is assigned. Elad's hosting permission does not replace third-party notices.
