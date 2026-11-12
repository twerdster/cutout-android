# Historical student-project archive

## Current source state — 2026-11-12

The original [project summary](docs/summary.pdf) and [CutOut poster](docs/poster.pdf) are available. A **strongly related, partial MATLAB/C++ prototype** is now preserved in [`source/original/related-prototype/`](source/original/related-prototype/). Corresponding archived result files match the Richardson project results, but the source itself has no verified author headers. **The final Android application remains unrecovered.**

These three project-specific prototype files retain their original bytes. They must not be attributed as a verified complete Elad Richardson source handoff or relabeled as the Android app. A generic heap implementation, result imagery and unreviewed assets were excluded. Authors and additional source provenance are welcome.

## Setup and validation

The preserved prototype contains the `fmm.m` MATLAB wrapper, a short `testScribbles.m` experiment and `fmmMex.cpp`. The generic `BinaryHeap` dependency was excluded because its attribution/license could not be established; example `image.jpg` and `scrib1.mat` are also absent. It is therefore intentionally incomplete and has not been compiled or executed. The final Android project used OpenCV/C++, NDK/JNI and an Android SDK interface; exact versions and the Android build are still unavailable.

Publication validation covers the source allowlist, SHA-256 provenance, text/XML or project metadata where applicable, and credential/private-material scanning. It is not an end-to-end runtime result. Read README.md and source/PROVENANCE.json before further edits; preserve source/original and existing documents. Any future updated implementation belongs in updated/ with its own requirements, tests, attribution and a clear description of its differences. Do not introduce a blanket license. Review this context file before committing; keep actual validation and source counts accurate.

## Interactive possibilities

A separately labeled, newly implemented fast-marching/scribble-segmentation exercise using synthetic images is a plausible notebook or Space. The recovered files do not currently provide the complete segmentation application.
