# CutOut for Android

<p align="center"><img src="assets/project-preview.jpg" alt="Original CutOut project interface and segmentation figure" width="380"></p>

![Code](https://img.shields.io/badge/code-Java%20%2F%20C%2B%2B%20%2F%20MATLAB%20%2F%20Python-blue)
![Runtime](https://img.shields.io/badge/updated%20runtime-macOS%20%2F%20Android-blue)
![Original](https://img.shields.io/badge/original%20snapshot-preserved-lightgrey)

CutOut was developed to extract an object from an image using foreground and background scribbles, then refine its boundary and alpha channel for image editing. The recovered C++ segmentation operators now run in a separately labeled updated native implementation, a local drawing interface and a new Android interface. The original Android source, project documents and separate MATLAB/C++ prototype remain preserved.

Student: Elad Richardson. Supervision: Anastasia Dubrovina and Aaron Wetzler, as credited in the original summary and poster.

The recovered application's About text credits Elad Richardson, Anastasia Dubrovina and Ron Kimmel, lists Aaron Wetzler for additional technical supervision, and credits Michal Hendel for help with interface and logo design. Its original wording and third-party source notices are preserved.

*The figure above is unchanged historical project imagery from the archived website. It does not show the Python exercise or a new validation result.*

## What is available

- [Project summary](docs/summary.pdf) and [poster](docs/poster.pdf).
- [Original Android source](source/original/android-app/): Java interface, JNI/C++ segmentation and image-editing operations, resources, and Eclipse/NDK build metadata. Elad Richardson supplied this archive on 2026-11-13 and identified it as the version used in the Android application. Its 319 regular files retain their received bytes. Start with the [source and build guide](docs/original-android.md), especially `jni/jni_part.cpp` and `DoKernelEstimation`.
- [Updated native implementation](updated/native/), [local drawing interface](updated/desktop/) and [updated Android interface](updated/android/). These use the recovered Lab density, pixel-grid Dijkstra and FLST/tree-Dijkstra operators, with documented fixes and optional alpha refinement.
- [Three related prototype files](source/original/related-prototype/): `fmm.m`, `testScribbles.m` and `fmmMex.cpp`. Matching archived outputs connect them to this project, but their source authorship is unverified. They are not a verified complete student handoff.
- [Updated Python component](updated/): fast marching on a positive-cost grid, plus a procedural scribble example. It returns arrival-time grids and composes foreground/background fields into a mask; the demo writes an SVG.

## Run the updated native implementation

The desktop build needs CMake 3.22+, Ninja, a C++17 compiler and OpenCV 4 development files for `core`, `imgproc` and `imgcodecs`. The drawing interface additionally needs Python 3.10+ and uses its standard library. From the repository root:

```sh
cmake -S updated/native -B build/native -G Ninja \
  -DOpenCV_DIR=/path/to/opencv/lib/cmake/opencv4 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/native --parallel 2
ctest --test-dir build/native --output-on-failure
python3 updated/desktop/server.py --binary build/native/cutout_cli
```

Open `http://127.0.0.1:8768/`, load the coffee example or your own photograph, draw foreground/background strokes and run segmentation. Choose FIGTree or histogram density, pixel-grid or level-set-tree propagation, and hard or refined alpha. The [native guide](updated/native/README.md) includes CLI input/output formats and an isolated OpenCV build recipe; the [drawing guide](updated/desktop/README.md) explains image limits and stroke corrections.

![Actual updated desktop run on the coffee photograph](assets/updated-desktop-run.png)

*Screenshot from a local drawing-interface run: FIGTree, level-set-tree propagation and boundary-refined alpha, with a 400-pixel working limit. The photograph is by Rachel Michetti, courtesy of Pikolo Espresso Bar, distributed by scikit-image under CC0. Strokes are new demonstration inputs. There is no independent reference mask or photographic accuracy claim.*

The [Android guide](updated/android/README.md) provides the Gradle/OpenCV SDK setup, APK build and device/emulator commands. Its new interface supports image loading and scribbles around the same native core. It does not restore the original camera, drag/compositing, blur or grayscale interface. The historical Eclipse application itself remains unbuilt.

## Use the Python exercise

The component requires Python 3.10+ and uses the standard library. From a checkout:

```sh
cd updated
python3 scribble_demo.py --size 48 --edge-weight 24 --output build/demo.svg
python3 -m unittest discover -s tests -v
```

Open `build/demo.svg` in a browser. The [notebook](updated/notebooks/fmm_scribbles.ipynb) exposes the same synthetic image, seeds, arrival fields and mask. See the [component guide](updated/README.md) for its API, notebook setup, original-formula comparison and behavior changes.

## Validation and limitations

The [2026-11-13 native record](updated/validation/VALIDATION.json) reports **31 named regression checks** passing with AddressSanitizer and UndefinedBehaviorSanitizer on macOS arm64, repeated with `-funsigned-char` to match Android ARM's unsigned plain `char`; the recovered FLST table contains negative entries. This is one CTest executable containing 31 checks, including 40 small deterministic generated-image/operator runs inside one check. OpenCV and system libraries were not sanitizer-instrumented.

The same record includes **25 CLI cases**: 15 successful runs, including eight density/distance/alpha combinations on one real photograph, and 10 expected input rejections. It also records **six isolated comparisons** where hash-verified original function bodies fail the regression and the updated functions pass. The [desktop record](updated/desktop/VALIDATION.json) covers the actual browser-to-native drawing workflow; the [Android record](updated/android/VALIDATION.json) covers the new app's build, 14 emulator segmentation runs and a separate 14-run check on a physical Google Pixel 2 running Android 10/API 29 with the same debug APK. These are separate from the original application's validation.

On the Pixel 2, offline checks covered 70 generated PNGs, seed-label agreement, alpha output, six coffee-input identities and six matched hard/refined mask pairs. Touch swipes injected with `adb shell input swipe`, working-size selection, missing-seed recovery, stale-result invalidation and credits/license viewing were also checked. Phone gallery import and interrupted-save fault injection were not repeated; those recorded checks remain emulator-specific.

Fixes cover empty strokes, Lab histogram indexing/strides, degenerate alpha, crop bounds, constant-color density estimation, unsigned-character FLST lookup values and FIGTree's time-dependent sampling. Repeated inputs now give repeatable outputs on the tested builds. [Updated source provenance](updated/native/PROVENANCE.json) records copied hashes and intentional behavior changes, including seed enforcement and the new wrapper's alpha/output choices.

The coffee example has no independent ground-truth segmentation. Observed grid-propagation outputs retain some wooden background; level-set outputs still have ragged edges or small fragments. No segmentation-accuracy benchmark, cross-platform bitwise equivalence or Linux execution is claimed. Android API 26 remains a declared minimum without a runtime test. The new interfaces recompute from all strokes; they do not expose the original cached `ADD_FRONT`/`ADD_BACK` interaction or every editing tool.

### Earlier Python exercise

The [2026-11-12 record](updated/VALIDATION.json) reports 16 component unit tests and five notebook code cells executed with Python 3.14.4 on macOS 15.1.1 arm64. Tests cover the local numerical stencil, propagation invariants, invalid inputs and the procedural example. A C++ harness compared the related prototype's `fmmMex.cpp` local Eikonal update with Python on 1,005 finite cases; maximum absolute difference was `7.11e-15` for the dimensionless unit-grid example.

The 2026-11-13 recovery changes the Python module's introductory docstring only; executable statements are unchanged and those component checks were not rerun.

That comparison covers one numerical update. The Python queue, accepted-neighbor rule, boundary handling, multiple seeds and image-to-cost composition are new. It does not establish whole-grid equivalence to the MEX prototype or segmentation accuracy on photographs. All example images and scribbles are procedural.

The related MATLAB/MEX prototype cannot run as provided: its generic `BinaryHeap` dependency was excluded because attribution and licensing could not be established, and `image.jpg`/`scrib1.mat` are absent. These gaps belong to that separate prototype, not the newly recovered Android snapshot.

The original Android project references OpenCV 2.4.9 and an external multitouch-controller library that are not included. It has legacy Eclipse/NDK configuration, including `gnustl_static` and a historical absolute OpenCV path. Its Java interface has not been rebuilt; the updated Android interface uses a new build and adapter. MATLAB/MEX and Linux execution also remain unvalidated. The [original-source guide](docs/original-android.md) records the historical operator chain and build gaps.

## Original code and attribution

The Android snapshot and three related prototype files retain their original bytes. [Source provenance](source/PROVENANCE.json) and the [Android manifest](source/ANDROID_PROVENANCE.json) record paths and hashes. Elad Richardson authorized hosting recovered project code on 2026-11-13. This records his permission only; it does not speak for other credited contributors, establish authorship of the separate prototype or assign a standard license to the mixed archive.

The Python local formula derives from the related prototype; the queue, validation, renderer, tests and synthetic composition were written in 2026 by Codex for Aaron Wetzler. Existing student and third-party credits remain in place. See the [Android attribution and license notes](docs/original-android.md#attribution-and-terms) and [supplementary license texts](docs/licenses/README.md). No blanket license is assigned to recovered or derived work. Authors can open an issue to correct attribution, contribute updates or request removal.

The native wrapper and fixes, local drawing interface and new Android runner are separately labeled 2026 work by Codex for Aaron Wetzler. Elad is credited for the recovered project implementation; the historical poster identifies the underlying method papers, and third-party modules retain their own author credits.

## Links

- [Project entry on Aaron Wetzler's website](https://twerdster.github.io/#cutout-for-android)
- [Project page](https://gip.cs.technion.ac.il/last-projects-2015-1.html)
- [Author page](https://eladrich.github.io/)
