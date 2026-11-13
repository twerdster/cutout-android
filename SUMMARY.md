# CutOut maintenance context

Read this file first, then SKILL.md and README.md. Current state: 2026-11-13.

## Software and preservation

CutOut is Elad Richardson's historical Android scribble-segmentation project. The original summary/poster credit Anastasia Dubrovina and Aaron Wetzler as supervisors. The recovered About text credits Elad, Anastasia and Ron Kimmel, Aaron for additional technical supervision, and Michal Hendel's help with interface/logo design.

Elad supplied the original Android archive and permission to host recovered project code on 2026-11-13. All 319 received files remain unchanged under source/original/android-app, checked against source/ANDROID_PROVENANCE.json. The separate three-file MATLAB/C++ prototype retains unverified authorship and original hashes; its heap/image/MAT inputs remain missing. Do not transfer Android attribution to it.

## Updated runnable paths

updated/native is a 2026 adaptation of recovered KernelEstimation/Dijkstra/ANN/FIGTree/MegaWave FLST operators. It uses OpenCV 4 and exposes a CMake cutout_core library and CLI. Both seed-set distance runs retain the original foreground-likelihood input. Density (FIGTree/histogram), propagation (pixel/tree) and alpha (hard/refined) are independent options.

updated/desktop is a new Python standard library localhost drawing interface calling the actual native CLI. Default URL 127.0.0.1:8768. It supports uploads, ordered scribble corrections, undo/clear, working-size selection and transparent PNG download. It makes no external segmentation request.

updated/android is a new Java/JNI shell using the same core, with gallery loading, touch scribbles, operator/working-size controls, ellipse/coffee examples and result export. The build uses Gradle 8.7, AGP 8.3.0, OpenCV 4.12, SDK 34, NDK 27.2.12479018 and CMake 3.22.1. ARM64-v8a only, minimum API 26. Historical Eclipse Java UI remains unbuilt; camera/compositing/blur/grayscale tools are not recreated.

From repo root:

```sh
cmake -S updated/native -B build/native -G Ninja -DOpenCV_DIR=/path/to/opencv/lib/cmake/opencv4 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/native --parallel 2
ctest --test-dir build/native --output-on-failure
python3 updated/desktop/server.py --binary build/native/cutout_cli
```

See platform guides for the isolated OpenCV recipe, Android commands and dependencies.

## Fixes and changed behavior

Updated fixes cover empty strokes, histogram clamps/strides and Gaussian offset, finite alpha, inclusive/empty crop, optional background color lookup, RAII/error handling, constant Lab-domain FIGTree density, fixed-seed local FIGTree sampling, signed hole-count lookup on Android, iterative tree-edge extraction and MegaWave signed-short coordinate limits. The wrapper validates seed collisions, serializes legacy state, enforces supplied labels, recomputes PDFs per run and returns full-size output. Hard alpha is CLI default; refined band 3 differs from historical 10/scale. Native PROVENANCE.json maps 48 copied files and exact hashes/changes. Never silently claim numerical identity to historical results.

## Recorded verification

updated/validation/VALIDATION.json records 31 named native checks with ASan/UBSan, repeated with -funsigned-char; one CTest executable contains 31 checks. One stress check includes 40 small deterministic image/operator runs. External OpenCV/system libraries were not sanitizer-instrumented; no leak-sanitizer claim.

Also 25 CLI cases (15 successful and 10 expected errors), eight density/distance/alpha photo combinations, repeatability and six isolated original-body fail/updated-pass comparisons. These are scoped regression/execution checks, not photographic accuracy or the historical APK.

updated/desktop/VALIDATION.json records Chrome 157 at 390/1440 pixels, eight actual browser-to-native photo combinations, uploads/single-point strokes, crossing corrections at full resolution, undo/clear, input/origin errors and no overflow, JavaScript errors or external requests. assets/updated-desktop-run.png is an actual FIGTree/tree/refined-alpha screenshot. The 600 × 400 coffee photograph is by Rachel Michetti, courtesy of Pikolo Espresso Bar, distributed under CC0 through pinned scikit-image; no independent ground truth. Grid outputs retain wood; tree outputs still have ragged edges/fragments.

updated/android/VALIDATION.json records a successful debug APK build/lint, 14 actual emulator segmentation runs, six hard/refined mask-pair comparisons, gallery/touch/error/working-size checks and preservation of successful output when saving fails. Credits/license viewing, stale-result invalidation, separate stroke segments after leaving the image, and interrupted-save recovery were also exercised. Android lint reported zero errors and 32 warnings. A later physical Google Pixel 2 / Android 10 (API 29, arm64-v8a) run used the same APK: 14 Activity/JNI segmentation cases, 70 saved PNG checks, six coffee input identities and six hard/refined mask-pair comparisons passed. Android input events injected with adb swipes/taps, working-size selection, missing-seed error/recovery, stale-preview prevention and credits/license viewing also passed. Phone gallery import and save-failure injection were not repeated on the device. API 26, Linux, general segmentation accuracy and cross-platform bitwise equivalence remain untested. Existing Python FMM exercise’s 16 tests, 1,005 local-stencil comparisons and five notebook cells remain dated 2026-11-12 and separate; its executable code was not changed for this restoration.

The initial existing Homebrew OpenCV failed on a missing OpenEXR ABI and TBB shutdown. A task-scoped minimal OpenCV 4.12 build (core/imgproc/imgcodecs, bundled PNG/JPEG/zlib, no TBB/OpenEXR/AVIF) was used; no global package repair. Keep archive data on verified AgentSSD when available, and SDKs/builds on a native filesystem; actual local paths and logs are recorded in the private restoration handoff.

## Maintenance boundaries

Original and third-party credits/terms remain. Elad's hosting permission is his alone, not other contributors' consent or a blanket license or manuscript approval. Preserve source/original bytes and source manifests; new work stays updated/. License references are supplemental, not relicensing. Raw mail, access identifiers, SDK archives, signing keys, build caches and private logs stay out of Git. Review/update this file and SKILL.md before every commit; current workflow requires actual Claude review and relevant source/link/image/run checks.

Two Claude review rounds covered the restored implementation, documentation and website records. Useful corrections were checked against source and executed tests; a third focused patch review checked the subsequent UI/result-persistence and credit corrections. Its remaining Spinner initialization guard was applied and checked during the final Android run. Reader Markdown, links and figures were checked at 390/1440 pixels. The current desktop record also verifies PNG download and stale-result invalidation. Reviews add no photographic accuracy or historical-application validation.

The physical-device follow-up retains the emulator record separately. The host smoke helper now reads only CutOut process logs, leaves device log buffers intact and checks foreground focus before taking a screenshot. The physical record identifies the executed 14-case helper revision. Later crash-log handling retained an intentionally induced Java fatal trace after the process exited. A subsequent normal pass completed three cases before Camera took focus and the screenshot guard stopped it; this partial pass is not another 14-run result. Native-signal injection and full cross-process tombstones remain outside this check. It still replaces this app’s generated last-run outputs; preserve them before testing a personal device. The APK and recovered source were unchanged.

Two real Claude reviews covered this physical-device follow-up. Useful corrections clarified injected touch events, installed APK hash evidence, within-pair image identity and timing scope, and fixed loss of retained-PID crash logs. The APK and native implementation were not rebuilt or changed.
