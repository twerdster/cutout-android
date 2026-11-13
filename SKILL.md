# Linux CI workflow

Read SUMMARY.md, .github/workflows/native-linux.yml and the native/validation guides. The workflow installs Ubuntu OpenCV4 development files and Clang18, builds normal and unsigned-char sanitizer configurations, runs31nativechecks/25CLIcases/sixoriginal-bodycomparisons, verifies preserved source hashes and uploads scoped results. A workflow definition is not execution evidence. Keep original source and licenses unchanged; keep numerical accuracy and platform compatibility claims scoped to actual runs.

Before committing, review/synchronize these context files, inspect staged paths and run authored whitespace/syntax checks. Use Aaron Wetzler <409920+twerdster@users.noreply.github.com>, normal pushes and verify remote CI results. Do not publish credentials or local machine paths.
