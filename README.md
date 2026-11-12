# CutOut for Android

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

## Links

- [Project entry on Aaron Wetzler's website](https://twerdster.github.io/#cutout-for-android)

- [Project page](https://gip.cs.technion.ac.il/last-projects-2015-1.html)
- [Author page](https://eladrich.github.io/)

## Attribution and Updates

Original student and third-party attribution will be retained. No new license for original source code is implied by this README. Original authors can open an issue to contribute updates or request corrections or removal of their material.

