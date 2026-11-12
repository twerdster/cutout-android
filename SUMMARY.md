# cutout-android: maintenance context

Read this file first, then SKILL.md and README.md. This context describes the repository as of 2026-11-12.

## Purpose and current source

Historical Android scribble segmentation project by Elad Richardson, supervised by Anastasia Dubrovina and Aaron Wetzler (both credited in docs/summary.pdf and docs/poster.pdf); related incomplete MATLAB/C++ prototype plus a separately written Python fast-marching exercise.

Three related prototype files retain recovered bytes. Their authorship is unverified; matching archived outputs support their connection to this project. The Android app, generic heap, example image and scribble MAT file are missing.

## Use and validation

Python 3.10+ required. From updated/: python3 scribble_demo.py --size 48 --edge-weight 24 --output build/demo.svg; python3 -m unittest discover -s tests -v. See updated/README.md and source/PROVENANCE.json.

updated/VALIDATION.json records 16 component tests and five real notebook code cells passed on 2026-11-12 with macOS 15.1.1 arm64 / Python 3.14.4. The original local Eikonal formula was compared on 1,005 finite cases, max absolute difference 7.11e-15 for the dimensionless unit-grid example. This is not whole-grid MEX equivalence, an app build or photographic segmentation accuracy.

Queue, accepted-neighbor handling, boundaries, multiple seeds, image-to-cost rule and label composition are new. All updated images/scribbles are procedural. MATLAB/MEX, Android and Linux runtime are unvalidated.

## Documentation maintenance

The 2026-11-12 documentation revision organizes the README around implementation, available source, setup, recorded checks and limits. It preserves historical visuals and links, with captions separating historical results from updated examples. No runnable code, source provenance, pinned revisions, licenses or validation results changed in this revision. Recorded earlier tests were not rerun for the prose-only edits.

Preserve original source and authorship. New ports belong under updated/ and require their own scope/validation. Do not infer a blanket license from recovery. Review this file and SKILL.md before every commit and synchronize changes to workflow, commands, validation or status. Check relative links and figure rendering; never describe inventory counts or component tests as full-application validation.

Two actual Claude editorial review rounds on 2026-11-12 covered the README and related changed documentation, including context files, component guides and applicable algorithm/build notes or schematic text. Useful qualifications were checked against source, preserved project records and saved validation logs, then applied. Editorial review adds no runtime coverage.
