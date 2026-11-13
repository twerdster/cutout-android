# CutOut for Android

<p align="center"><img src="assets/project-preview.jpg" alt="Original CutOut project interface and segmentation figure" width="380"></p>

![Code](https://img.shields.io/badge/code-Java%20%2F%20C%2B%2B%20%2F%20MATLAB%20%2F%20Python-blue)
![Platform](https://img.shields.io/badge/platform-Android%20source-lightgrey)
![Build](https://img.shields.io/badge/Android%20build-unverified-orange)

CutOut was developed to extract an object from an image using foreground and background scribbles, then refine its boundary and alpha channel for image editing. This repository contains the recovered Android Java/C++ implementation, project documents, a separate incomplete MATLAB/C++ prototype, and a separately written Python exercise. The Android source has been recovered; its build and execution have not been validated.

Student: Elad Richardson. Supervision: Anastasia Dubrovina and Aaron Wetzler, as credited in the original summary and poster.

The recovered application's About text credits Elad Richardson, Anastasia Dubrovina and Ron Kimmel, lists Aaron Wetzler for additional technical supervision, and credits Michal Hendel for help with interface and logo design. Its original wording and third-party source notices are preserved.

*The figure above is unchanged historical project imagery from the archived website. It does not show the Python exercise or a new validation result.*

## What is available

- [Project summary](docs/summary.pdf) and [poster](docs/poster.pdf).
- [Original Android source](source/original/android-app/): Java interface, JNI/C++ segmentation and image-editing operations, resources, and Eclipse/NDK build metadata. Elad Richardson supplied this archive on 2026-11-13 and identified it as the version used in the Android application. Its 319 regular files retain their received bytes. Start with the [source and build guide](docs/original-android.md), especially `jni/jni_part.cpp` and `DoKernelEstimation`.
- [Three related prototype files](source/original/related-prototype/): `fmm.m`, `testScribbles.m` and `fmmMex.cpp`. Matching archived outputs connect them to this project, but their source authorship is unverified. They are not a verified complete student handoff.
- [Updated Python component](updated/): fast marching on a positive-cost grid, plus a procedural scribble example. It returns arrival-time grids and composes foreground/background fields into a mask; the demo writes an SVG.

## Use the Python exercise

The component requires Python 3.10+ and uses the standard library. From a checkout:

```sh
cd updated
python3 scribble_demo.py --size 48 --edge-weight 24 --output build/demo.svg
python3 -m unittest discover -s tests -v
```

Open `build/demo.svg` in a browser. The [notebook](updated/notebooks/fmm_scribbles.ipynb) exposes the same synthetic image, seeds, arrival fields and mask. See the [component guide](updated/README.md) for its API, notebook setup, original-formula comparison and behavior changes.

## Validation and limitations

The [2026-11-12 record](updated/VALIDATION.json) reports 16 component unit tests and five notebook code cells executed with Python 3.14.4 on macOS 15.1.1 arm64. Tests cover the local numerical stencil, propagation invariants, invalid inputs and the procedural example. A C++ harness compared the related prototype's `fmmMex.cpp` local Eikonal update with Python on 1,005 finite cases; maximum absolute difference was `7.11e-15` for the dimensionless unit-grid example.

The 2026-11-13 recovery changes the Python module's introductory docstring only; executable statements are unchanged and those component checks were not rerun.

That comparison covers one numerical update. The Python queue, accepted-neighbor rule, boundary handling, multiple seeds and image-to-cost composition are new. It does not establish whole-grid equivalence to the MEX prototype or segmentation accuracy on photographs. All example images and scribbles are procedural.

The related MATLAB/MEX prototype cannot run as provided: its generic `BinaryHeap` dependency was excluded because attribution and licensing could not be established, and `image.jpg`/`scrib1.mat` are absent. These gaps belong to that separate prototype, not the newly recovered Android snapshot.

The Android project references OpenCV 2.4.9 and an external multitouch-controller library that are not included. It has legacy Eclipse/NDK configuration, including `gnustl_static` and a historical absolute OpenCV path. No APK, Android build, device/emulator run or photographic segmentation benchmark was produced during recovery. MATLAB/MEX and Linux execution also remain unvalidated. The [Android guide](docs/original-android.md) records the inspected operator chain, missing dependencies and unresolved build configuration.

## Original code and attribution

The Android snapshot and three related prototype files retain their original bytes. [Source provenance](source/PROVENANCE.json) and the [Android manifest](source/ANDROID_PROVENANCE.json) record paths and hashes. Elad Richardson authorized hosting recovered project code on 2026-11-13. This records his permission only; it does not speak for other credited contributors, establish authorship of the separate prototype or assign a standard license to the mixed archive.

The Python local formula derives from the related prototype; the queue, validation, renderer, tests and synthetic composition were written in 2026 by Codex for Aaron Wetzler. Existing student and third-party credits remain in place. See the [Android attribution and license notes](docs/original-android.md#attribution-and-terms) and [supplementary license texts](docs/licenses/README.md). No blanket license is assigned to recovered or derived work. Authors can open an issue to correct attribution, contribute updates or request removal.

## Links

- [Project entry on Aaron Wetzler's website](https://twerdster.github.io/#cutout-for-android)
- [Project page](https://gip.cs.technion.ac.il/last-projects-2015-1.html)
- [Author page](https://eladrich.github.io/)
