# cutout-android: maintenance context

Read this file first, then SKILL.md and README.md. This context describes the repository as of 2026-11-13.

## Purpose and current source

Historical Android scribble segmentation project by Elad Richardson, supervised by Anastasia Dubrovina and Aaron Wetzler (both credited in docs/summary.pdf and docs/poster.pdf). The recovered app also credits Ron Kimmel and Michal Hendel's help with interface/logo design. Source consists of the author-supplied Android Java/C++ snapshot, a separate incomplete MATLAB/C++ prototype and a separately written Python fast-marching exercise.

Elad supplied the Android archive on 2026-11-13 and authorized hosting recovered project code. All 319 regular files are preserved unchanged in source/original/android-app, including third-party notices and historical build metadata. source/ANDROID_PROVENANCE.json records the archive and per-file hashes. docs/original-android.md maps the JNI operator chain and build limits. The OpenCV 2.4.9 SDK and multitouch-controller dependency are absent; no APK/build/device run has been validated.

Three separate related prototype files retain recovered bytes. Their authorship is unverified; matching archived outputs support their connection to this project. Their generic heap, example image and scribble MAT file remain missing. The Android recovery does not resolve those prototype gaps or authorship.

## Use and validation

Python 3.10+ required. From updated/: python3 scribble_demo.py --size 48 --edge-weight 24 --output build/demo.svg; python3 -m unittest discover -s tests -v. See updated/README.md and source/PROVENANCE.json.

updated/VALIDATION.json records 16 component tests and five notebook code cells passed on 2026-11-12 with macOS 15.1.1 arm64 / Python 3.14.4. The related prototype fmmMex.cpp local Eikonal formula was compared on 1,005 finite cases, max absolute difference 7.11e-15 for the dimensionless unit-grid example. This is not whole-grid MEX equivalence, an app build or photographic segmentation accuracy.

Queue, accepted-neighbor handling, boundaries, multiple seeds, image-to-cost rule and label composition are new. All updated images/scribbles are procedural. MATLAB/MEX, Android and Linux runtime are unvalidated.

## Documentation maintenance

The 2026-11-12 documentation revision organizes the README around implementation, available source, setup, recorded checks and limits. It preserves historical visuals and links, with captions separating historical results from updated examples. No runnable code, source provenance, pinned revisions, licenses or validation results changed in this revision. Recorded earlier tests were not rerun for the prose-only edits.

Preserve original source and authorship. New ports belong under updated/ and require their own scope/validation. Do not infer a blanket license from recovery. Review this file and SKILL.md before every commit and synchronize changes to workflow, commands, validation or status. Check relative links and figure rendering; never describe inventory counts or component tests as full-application validation.

Two Claude editorial review rounds on 2026-11-12 covered the README and related changed documentation, including context files, component guides and applicable algorithm/build notes or schematic text. Useful qualifications were checked against source, preserved project records and saved validation logs, then applied. Editorial review adds no runtime coverage.

The 2026-11-13 recovery adds the unchanged Android snapshot, a separate hash manifest, source/build/attribution guide and supplementary upstream license/copyright references. The README and Python component documentation distinguish the recovered implementation from the earlier exercise. updated/fmm.py changes only its module docstring; executable statements and the dated validation record are unchanged. Elad Richardson’s permission to host recovered project code is his permission only, does not speak for other credited contributors, and is not a blanket software license, proof of third-party authorship/permissions or consent to a new manuscript. No Android numerical/runtime coverage was added.

Recovery verification checked all 319 received file hashes, the three existing prototype hashes, six supplemental license-reference hashes and 48 XML/Eclipse files. Text and image metadata were scanned; all 202 raster files decoded, with a contact-sheet review and four individual tutorial screenshots. Reader documentation rendered without broken local resources or layout overflow at 390/1440 pixels. The default staged whitespace check reports preserved original/license whitespace; newly authored documentation passes the strict check. No source was reformatted to remove those historical differences. Two Claude editorial review rounds covered the recovery documentation and site changes. Credit, permission-scope, notebook-label and execution-scope qualifications were checked against the source and applied. Review and static checks add no app execution evidence.
