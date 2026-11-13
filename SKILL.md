# Maintaining CutOut

Read SUMMARY.md, then README.md and the guides for affected components.

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
