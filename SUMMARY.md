# CutOut project context

Read this file, then SKILL.md and README.md.

## Updated-work navigation (2026-11-14)

Aaron authorized a top-of-page local `New!` badge and exact “Updated and ported for modern hardware” link in the root README. Both lead to the matching heading in `updated/README.md`, which now briefly introduces the current recovered-Android native port, documented fixes, desktop/mobile interfaces and limits before the unchanged earlier Python exercise. An italic qualifier marks the following root content as the project reference.

This navigation-only change preserves the complete current root body after removing only the new navigation block. The updated guide is titled “CutOut updates”; earlier Python headings are nested beneath “Earlier Python exercise”, with its prose and code unchanged. All original and updated source, dependency pins, media, provenance manifests and validation records remain unchanged. No earlier missing-Android-source claim applies to the current recovered Android snapshot; the historical Eclipse interface and new native/mobile executions remain distinct. These documentation edits add no runtime evidence or package changes.

## Components

CutOut performs object segmentation from foreground/background image scribbles. The recovered implementation is by Elad Richardson; [attribution and terms](docs/ATTRIBUTION.md) distinguish project, interface and dependency credits.

- `source/original/android-app/`: unchanged historical Android source; its Eclipse interface remains unbuilt. `source/ANDROID_PROVENANCE.json` records the received files and hashes.
- `updated/native/`: C++17/OpenCV 4 core and CLI using recovered Lab density, pixel/tree Dijkstra and optional alpha refinement. The provenance manifest maps copied files and intentional changes.
- `updated/desktop/`: Python standard-library drawing server around the native CLI; images and computation stay local.
- `updated/android/`: new Java/JNI interface using the native core.
- `updated/ios/`: new UIKit/Objective-C++ interface using the shared core, with ordered seed drawing, independent operator controls, unique saved runs and PNG sharing. Its [guide](updated/ios/README.md) and [validation record](updated/ios/VALIDATION.json) distinguish device segmentation from interface-test coverage.
- `.github/workflows/native-linux.yml`: Ubuntu native sanitizer builds, CLI checks and isolated original-function comparisons, with named-case results and source hashes.
- `updated/README.md`: concise native-update overview and links, followed by the separate Python fast-marching exercise derived from a related prototype of unverified authorship.

## Entry points

The root README contains the desktop quickstart and platform-guide links. Build the native CLI with CMake, then run `python3 updated/desktop/server.py --binary build/native/cutout_cli`. Mobile builds use their platform guides and dependency settings.

[Validation and limitations](docs/VALIDATION.md) links native, drawing-interface and mobile records. Each record names its implementation and execution environment; source recovery, successful execution and segmentation accuracy are separate claims. The coffee photograph has no independent reference mask. Do not infer support for an untested device, operating system or API level.

## Preservation and maintenance

Keep original source, source manifests, figures and license texts unchanged. Fixes and new interfaces belong under `updated/`, with their own provenance and validation. The older three-file prototype retains unverified authorship; Android credits do not resolve it.

Keep the README concise. Put setup details in platform guides, test results in validation records, and historical attribution in the linked documents. Before committing, synchronize this file and SKILL.md, verify changed links and relevant execution claims, and preserve existing copyright and license notices.
