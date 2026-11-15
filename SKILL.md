# Maintaining CutOut

Read SUMMARY.md, then README.md and the guides for affected components.

## Updated-work navigation

Aaron authorized the root README’s local `New!` badge and exact “Updated and ported for modern hardware” link on 2026-11-14. Keep both linked to `updated/README.md#updated-and-ported-for-modern-hardware`, followed by the italic project-reference qualifier. Preserve the rest of the current root body and keep new implementation descriptions under the matching updated heading.

The updated entrypoint must distinguish the recovered Android C++ operator port and current native/mobile evidence from the earlier independent Python exercise. Keep the native overview under the updated heading and nest the earlier Python guide headings beneath “Earlier Python exercise”. Keep original Eclipse build status, photographic accuracy and iOS interaction-test limits explicit. Preserve source/provenance/validation hashes; historical records stay bound to their recorded snapshots. Check root-body preservation after removing only the navigation block. Documentation-only navigation changes do not require a package rebuild or add runtime coverage.

## Historical award references

Aaron authorized the root README's award paragraph on 2026-11-15. Preserve the distinction between the [2014 Amdocs contest](https://www.cs.technion.ac.il/news/view-new.php?nwid=763), its [6 July 2015 award ceremony](https://www.cs.technion.ac.il/news/view-new.php?nwid=783), and the separate [25 November 2015 Intel Challenge](https://www.cs.technion.ac.il/news/view-new.php?nwid=813). Both awards concern Elad Richardson's CutOut project. The paragraph is an additional authorized exception to older root-body preservation checks; it adds no implementation or runtime claims.

## Build and use

Use the [native guide](updated/native/README.md) for CMake/OpenCV dependencies, CLI formats and operator changes. The [drawing guide](updated/desktop/README.md) covers the Python loopback server; [Android](updated/android/README.md) and [iOS](updated/ios/README.md) have separate builds and interfaces. Keep the Python exercise independent.

From the repository root, the native check command is `ctest --test-dir build/native --output-on-failure`. Use the actual build directory. The validation guide describes sanitizer builds, CLI checks and isolated original-function regressions. Run checks affected by code changes; documentation edits need link, rendering and factual-scope checks.

## Preserve source and behavior records

- Keep `source/original/` and its manifests byte-exact. Original Android source, the related prototype and updated implementations have separate provenance.
- Keep copied dependency notices and `docs/licenses/` unchanged. Record intentional updated native changes with its provenance generator.
- Preserve the original foreground-likelihood input for both seed-distance runs unless an algorithm change is explicitly documented. Record resizing, seed enforcement, randomness, alpha-band and output-cropping behavior.
- Screenshots must come from actual runs and retain fixture credit. Component tests and one photograph without reference labels do not establish segmentation accuracy or historical-APK equivalence.

## Interface checks

Regenerate the iOS project with `python3 updated/ios/generate_project.py` after changing its source/resource inventory or build settings. Keep shared signing and bundle settings at project level, configured externally; never commit personal team or device identifiers.

Keep processing local, bounded and isolated from concurrent solver state. Preserve input validation, ordered scribble corrections and result invalidation after input/options change. For reused output directories, remove the old completion marker before overwriting images and write a new marker last. The iOS app instead creates an exclusive unique directory for each run. Saving failure must not discard a computed result; distinguish a failed run save from an optional last-run pointer warning.

For desktop changes, check an actual browser-to-native request and narrow/wide layouts. For mobile changes, record the selected device or simulator, build identity, real segmentation and relevant image/drawing/error/export controls. Build or launch success alone is insufficient. Preserve visible implementation and third-party credits.

Device tests must preserve existing app outputs and collect only application-scoped logs. Do not clear shared device log buffers. Capture only the intended foreground application. Keep emulator/simulator and physical-device evidence separate, including failed or partial checks.

## Documentation and publication

Keep root README usage-oriented and short. Put detailed commands in platform guides, test environments/results in validation records, and attribution in [docs/ATTRIBUTION.md](docs/ATTRIBUTION.md). Avoid machine-specific paths and setup history in general documentation.

Before committing, review and synchronize SUMMARY.md and this file, check staged scope, Markdown, relative links and figures, and verify preservation manifests. When Claude review is requested, use an actual review and check factual suggestions against code and recorded evidence. Never claim a review or run that did not complete.

Keep credentials, signing keys, SDKs, caches and unreviewed user images/logs out of commits. Publish through the existing guarded workflow without rewriting historical commits. If website resources change, synchronize its HTML, JSON, resource index and context; website checks do not validate the segmentation implementation.

## Browser maintenance

Browser work stays under `updated/browser/`; preserve the shared native operators and recovered originals. Build with Emscripten 4.0.20 and verified OpenCV 4.12.0 sources using `bash updated/browser/build.sh OPENCV_SOURCE BUILD_DIRECTORY`. Serve the generated `BUILD_DIRECTORY/site` over HTTP(S), not `file:`. The build uses scalar OpenCV, a single worker and bounded growable Wasm memory; no cross-origin-isolation headers or pthreads are required.

Run the native/Wasm comparison and actual browser checks described in `updated/browser/README.md` after bridge, worker or drawing changes. Retain actual per-case mask/alpha differences and the native OpenCV version; the CI reference distribution differs from the Wasm library. Check cancellation after a real run has started, recovery into a fresh worker, ordered label corrections, single-pointer ownership, input/result invalidation, downloads and phone-width layout. Browser rasterization and decode are not covered by the raw-buffer core comparison. Do not infer physical-phone or cross-browser execution from responsive desktop screenshots.

`package.py` replaces only a generated output directory with its marker; choose a fresh empty directory otherwise. It includes corresponding native/browser source, generic tar metadata, public guide links and actual dependency notices. Keep browser-specific credits independent of mobile UI text. Do not add user images, local toolchains, generated Wasm, signing values or personal paths to source commits. Rebuild the package after documentation/source changes so its guide, notices, archive and hashes match. Keep the root README short; link this implementation from its browser platform row and the updated overview.
