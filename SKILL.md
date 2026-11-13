# Maintaining CutOut

Read SUMMARY.md, then this file, README.md and the platform/validation guides before edits.

## Components and commands

Keep received originals under source/original unchanged, verified against source/ANDROID_PROVENANCE.json and source/PROVENANCE.json. The old MATLAB/C++ prototype has unverified authorship. The historical Java application remains unbuilt.

The restored implementation is updated/native (C++17/OpenCV 4), shared by updated/desktop (Python 3.10+ standard-library localhost drawing server) and updated/android (new Java/JNI shell). The earlier Python FMM notebook/exercise is independent. Follow each guide and validation record; do not combine their test counts or scopes.

Native workflow: cmake -S updated/native -B build/native -G Ninja -DOpenCV_DIR=/path/to/opencv/lib/cmake/opencv4 -DCMAKE_BUILD_TYPE=RelWithDebInfo; cmake --build build/native --parallel 2; ctest --test-dir build/native --output-on-failure. Add CUTOUT_SANITIZE=ON for affected regression validation. Repeat with -funsigned-char when changing MegaWave/platform code. Run updated/validation/check_cli.py and verify_original_failures.py as documented for affected operator/regression changes. Expected original failures are evidence, not successful original app execution.

Drawing workflow: python3 updated/desktop/server.py --binary build/native/cutout_cli. Bind to loopback; retain input-size limits, one-process-at-a-time isolation, origin checking, disjoint ordered stroke masks and working-size/error controls. Validate 390/1440-pixel layouts and an actual browser-to-native request for UI/server changes.

Android workflow: configure task-scoped SDK/NDK/OpenCV dependency paths, build via updated/android/gradlew with at most two workers, run lint and updated/android/tools/smoke.py against an explicitly selected device or emulator. Check real image loading, drawing, errors, working-size choices and result handling. A successful build/launch is not proof that segmentation ran. Do not add release signing/store distribution unless requested.

## Attribution and scientific scope

Preserve Elad Richardson/project credits, ANN/FIGTree/MegaWave module notices and all supplemental license texts. New wrappers/fixes are updated 2026 work for Aaron Wetzler, with original sections retained. No blanket repository license. Elad's permission to host recovered project code is his permission alone; it does not cover other contributors or a new manuscript.

Retain original foreground-likelihood input for both distance runs unless an explicitly explained algorithm change is intended. Record changes to seed enforcement, determinism, alpha band, resizing and output cropping. Do not claim numerical identity with the old APK or general correctness from limited regressions. No ground-truth accuracy claim for coffee photo; use recorded units and distinguish single-call timings from benchmarks.

If copied legacy/vendor code changes, regenerate updated/native/PROVENANCE.json using its supplied script and verify hashes. Keep the current validation source-hash record synchronized after rerunning affected tests. Screenshots must come from actual runs and retain fixture credit/provenance. Original-source hashes must remain byte-exact.

## Environments and publication

Use native filesystem semantics for repositories, SDKs, CMake/Gradle builds and virtual environments. Prefer verified mounted AgentSSD for large archives; record actual local paths in the private handoff. Existing Homebrew OpenCV is not the verified dependency: use the isolated build recipe if necessary. Never repair global packages or disturb unrelated services merely to run this project.

Keep raw correspondence, private addresses/keys, SDK archives, build outputs/caches, debug signing material and run logs out of commits. Build artifacts for Aaron may be delivered separately; do not describe a debug APK as a store release.

Before every commit reread and synchronize SUMMARY.md and this file. Check authored Markdown, links/images, staged paths, preservation manifests, current recorded test results and runtime scope. Preserve original/copied license bytes even when the default whitespace check flags them; validate authored edits strictly. Actual Claude reviews are required by the current requested workflow; verify feedback against source, apply useful corrections and review substantial revisions again. Commit as Aaron Wetzler <409920+twerdster@users.noreply.github.com>, push normally with current remote guard and verify public files. If website metadata changes, keep its HTML/JSON/llms/provenance/context synchronized and distinguish tested updated runtimes from the unbuilt historical interface.

For saved Android run artifacts, result.json is the completion marker: invalidate an old marker before overwriting PNGs and publish a new marker last. Display computed output independently from saving, and invalidate previews after input/options changes. Preserve visible implementation and third-party credits in each interface.

The 2026-11-13 restoration received actual Claude code, documentation and focused patch reviews. Check recorded evidence before repeating review or execution claims; every result applies to the named updated implementation, not the historical APK.

For a personal physical phone, verify its API/ABI and USB-debug authorization, preserve existing app-generated last-run outputs before testing, and collect only CutOut process logs. Do not clear device log buffers. Capture screenshots only while CutOut is foreground and keep phone captures private until reviewed. Record physical-device evidence separately from emulator measurements; the current tested phone is one Google Pixel 2 on Android 10/API 29, not all supported Android versions. The declared API 26 minimum and Linux remain untested. No APK rebuild is needed for host-only smoke-helper or documentation changes.

The smoke helper retains its launch PID and a device-time log boundary before waiting for results, and writes scoped logs even if that PID exits. The recorded fault injection covers an intentional Java crash. Native fatal lines from the app PID may be logged, but separate crash-dump tombstones and crashes before a PID is observed are outside the retained-log guarantee. Preserve the original 14-case device record separately from any partial diagnostic rechecks.
