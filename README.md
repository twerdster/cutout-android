# CutOut for Android

<p align="center"><img src="assets/project-preview.jpg" alt="Original CutOut project interface and segmentation figure" width="380"></p>

![Code](https://img.shields.io/badge/code-MATLAB%20%2F%20C%2B%2B%20%2F%20Python-blue)
![Platform](https://img.shields.io/badge/platform-Android%20app%20unrecovered-lightgrey)
![Source](https://img.shields.io/badge/source-related%20prototype-orange)

CutOut was developed to segment an object in an Android image from foreground and background scribbles. This repository contains project documents, a related incomplete MATLAB/C++ prototype, and a separately written Python exercise. The Android application has not been recovered.

Student: Elad Richardson. Supervision: Anastasia Dubrovina and Aaron Wetzler, as credited in the original summary and poster.

*The figure above is unchanged historical project imagery from the archived website. It does not show the Python exercise or a new validation result.*

## What is available

- [Project summary](docs/summary.pdf) and [poster](docs/poster.pdf).
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

The [2026-11-12 record](updated/VALIDATION.json) reports 16 component unit tests and five notebook code cells executed with Python 3.14.4 on macOS 15.1.1 arm64. Tests cover the local numerical stencil, propagation invariants, invalid inputs and the procedural example. A C++ harness compared the recovered local Eikonal update with Python on 1,005 finite cases; maximum absolute difference was `7.11e-15` for the dimensionless unit-grid example.

That comparison covers one numerical update. The Python queue, accepted-neighbor rule, boundary handling, multiple seeds and image-to-cost composition are new. It does not establish whole-grid equivalence to the MEX prototype or segmentation accuracy on photographs. All example images and scribbles are procedural.

The related prototype cannot run as provided: its generic `BinaryHeap` dependency was excluded because attribution and licensing could not be established, and `image.jpg`/`scrib1.mat` are absent. MATLAB/MEX execution, the Android build and Linux execution have not been validated. The [project summary](docs/summary.pdf) and [poster](docs/poster.pdf) describe an OpenCV/C++ algorithm ported with the Android NDK and wrapped with an Android SDK interface. The complete application and its build requirements are unavailable.

## Original code and attribution

The three recovered files retain their original bytes. [Source provenance](source/PROVENANCE.json) records the published paths and hashes. The Python local formula derives from this related prototype; the queue, validation, renderer, tests and synthetic composition were written in 2026 by Codex for Aaron Wetzler. Existing student and third-party credits remain in place. No blanket license is assigned to recovered or derived work. Authors can open an issue to correct attribution, contribute updates or request removal.

## Links

- [Project entry on Aaron Wetzler's website](https://twerdster.github.io/#cutout-for-android)
- [Project page](https://gip.cs.technion.ac.il/last-projects-2015-1.html)
- [Author page](https://eladrich.github.io/)
