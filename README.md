# CutOut for Android

<p align="center"><img src="assets/project-preview.jpg" alt="Original CutOut project interface and segmentation figure" width="380"></p>

![Code](https://img.shields.io/badge/code-MATLAB%20%2F%20C%2B%2B%20%2F%20Python-blue)
![Platform](https://img.shields.io/badge/platform-Android%20app%20unrecovered-lightgrey)
![Source](https://img.shields.io/badge/source-related%20prototype-orange)

An Android application for object segmentation from user-provided scribbles.

Students: Elad Richardson.

Supervision: Aaron Wetzler.

## Status

The original [project summary](docs/summary.pdf) and [CutOut poster](docs/poster.pdf) are available. A **strongly related, partial MATLAB/C++ prototype** is now preserved in [`source/original/related-prototype/`](source/original/related-prototype/). Corresponding archived result files match the Richardson project results, but the source itself has no verified author headers. **The final Android application remains unrecovered.**

## Running

The preserved prototype contains the `fmm.m` MATLAB wrapper, a short `testScribbles.m` experiment and `fmmMex.cpp`. The generic `BinaryHeap` dependency was excluded because its attribution/license could not be established; example `image.jpg` and `scrib1.mat` are also absent. It is therefore intentionally incomplete and has not been compiled or executed. The final Android project used OpenCV/C++, NDK/JNI and an Android SDK interface; exact versions and the Android build are still unavailable.

## Original code

These three project-specific prototype files retain their original bytes. They must not be attributed as a verified complete Elad Richardson source handoff or relabeled as the Android app. A generic heap implementation, result imagery and unreviewed assets were excluded. Authors and additional source provenance are welcome.

The [source provenance manifest](source/PROVENANCE.json) records the published paths, hashes and any redactions. Original student/third-party notices remain in place. No blanket license is assigned to recovered work. Future ports belong in `updated/` and must be described and tested as updated implementations.

## Updated code (2026)

The separately maintained [`updated/`](updated/) directory adds a runnable Python fast-marching and scribble exercise. The original source and student attribution remain preserved above. [Open the notebook](updated/notebooks/fmm_scribbles.ipynb) to explore the synthetic example; it is distinct from the historical application and original results.

On macOS 15.1.1 with Python 3.14.4, **16 component tests pass**, and all 5 notebook code cells execute in an isolated real Jupyter kernel. The original hash-verified local Eikonal formula matches 1,005 finite comparison cases (maximum absolute difference 7.11e-15). Queue, boundaries, multi-source seeds and procedural label composition are new; no whole-grid equivalence to the incomplete MEX prototype or Android app is claimed. Linux and the complete historical app have not been validated. See the [updated setup and limitations](updated/README.md).

## Links

- [Project entry on Aaron Wetzler's website](https://twerdster.github.io/#cutout-for-android)

- [Project page](https://gip.cs.technion.ac.il/last-projects-2015-1.html)
- [Author page](https://eladrich.github.io/)

## Attribution and Updates

Original student and third-party attribution will be retained. No new license for original source code is implied by this README. Original authors can open an issue to contribute updates or request corrections or removal of their material.


Figure provenance: the original project figure from Aaron Wetzler’s archived website, copied without changing its bytes. It is historical project imagery, not a new validation result.
