# Updated Android runner

This is a new Android interface and JNI adapter for the restored CutOut native code in [`../native`](../native/). It loads an image through Android's document picker, accepts foreground and background scribbles, and runs the recovered color-density and distance-propagation operators. FIGTree or histogram density estimation, pixel-grid or level-set-tree propagation, and optional boundary-band alpha refinement can be selected independently. The working-image limit can be set to 400 pixels, 800 pixels or the full loaded image.

The original Eclipse application, its interface, resources and build metadata remain unchanged under [`../../source/original/android-app`](../../source/original/android-app/). This runner does not reproduce its camera capture, tutorial, multitouch composition, blur or grayscale editing interface. Its procedural fixture, supplied scribbles and interface were written in 2026 by Codex for Aaron Wetzler; the underlying recovered segmentation implementation is by Elad Richardson, with the original project and third-party credits retained.

<p><img src="assets/android-coffee-run.png" alt="Actual updated Android emulator run showing the coffee photograph cutout, operator choices and elapsed time" width="320"></p>

*Actual Android 14 emulator screenshot using FIGTree, level-set propagation, refined alpha and the 400-pixel working limit. Coffee photograph: Rachel Michetti / Pikolo Espresso Bar, CC0 via scikit-image. This one input has no independent reference mask.*

## Build

The restoration uses Android Gradle Plugin 8.3.0, Gradle 8.7, OpenCV Android SDK 4.12.0, Android SDK platform/build tools 34, NDK 27.2.12479018 and CMake 3.22.1. The checked-in wrapper was executed successfully with the documented command. It verifies the Gradle distribution SHA-256. Its JAR matches the [official Gradle 8.7.0 tag](https://raw.githubusercontent.com/gradle/gradle/v8.7.0/gradle/wrapper/gradle-wrapper.jar) and [published wrapper checksum](https://services.gradle.org/distributions/gradle-8.7-wrapper.jar.sha256), `cb0da6751c2b753a16ac168bb354870ebb1e162e9083f116729cec9c781156b8`; exact dependency and script hashes are in the [validation record](VALIDATION.json). Gradle’s [license](gradle/LICENSE) and [notice](gradle/NOTICE) are copied unchanged from that verified distribution; they do not assign terms to the recovered CutOut code. Java 17 or 21 is suitable; the recorded build uses OpenJDK 21. The APK currently includes `arm64-v8a` only and requires Android 8.0/API 26 or later.

Install Google's command-line tools or Android Studio, then install these packages with `sdkmanager`:

```sh
sdkmanager 'platform-tools' 'platforms;android-34' 'build-tools;34.0.0' \
  'ndk;27.2.12479018' 'cmake;3.22.1'
```

Download `opencv-4.12.0-android-sdk.zip` from the [official OpenCV release](https://github.com/opencv/opencv/releases/tag/4.12.0), verify SHA-256 `fd7f2332331b4eb8b67e55137281cfb16823c9399d90deb9cfa3476783b99e35`, and extract it. Keep SDKs, dependencies and build directories on a filesystem supporting normal Unix semantics. Set `ANDROID_HOME` to the installed Android SDK and `OPENCV_ANDROID_SDK` to the extracted `OpenCV-android-sdk` directory:

```sh
cd updated/android
export ANDROID_HOME=/path/to/android-sdk
export OPENCV_ANDROID_SDK=/path/to/OpenCV-android-sdk
./gradlew --no-daemon --max-workers=2 assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n org.awetzler.cutout.updated/.MainActivity
```

The APK uses Android's normal local debug signing configuration. This build does not create a Play Store release, configure a global SDK, or download images at runtime. Google SDK licenses must be accepted for the selected packages.

## Use

Open an image, draw green foreground and red background scribbles, then select **Segment**. Drawing one class erases any overlapped pixels from the other class. **Image / result** switches between the input with scribbles and the cutout against a checkerboard. **Examples** loads the procedural ellipse or the bundled coffee photograph with supplied scribbles; **Credits and terms** opens the implementation/method/dependency credits and a viewer for packaged license texts. **Clear** removes both seed classes. Empty or invalid inputs produce a visible error rather than entering the native solver. Changing the image, scribbles or operator settings clears the current result; a failed run cannot re-display an earlier cutout. Operator settings are disabled while a run is in progress. A brush leaving and re-entering the displayed image starts a separate stroke segment.

Imported images are decoded with bounded sampling, oriented using Exif metadata and reduced to at most 800 pixels along their longest side. The native pipeline uses its separately recorded 400-pixel working limit by default. Choose **Working: full input** or 800 pixels when downsampling would merge nearby opposing scribbles; conflicting labels are rejected rather than silently reassigned. This can change fine boundaries and does not establish equivalence to every historical device/result. Processing runs on a single background worker, and all segmentation is local; the application requests no Internet, camera or broad storage permission.

A successful computation remains visible if saving fails; the status reports the persistence warning separately. If app-specific external storage is unavailable, the runner uses its internal files directory. The latest successfully saved run writes `input.png`, `foreground.png`, `background.png`, `mask.png`, `cutout.png` and `result.json` under Android's app-specific external `files/last-run` directory. The mask is binary; alpha refinement changes the alpha channel in `cutout.png`. `result.json` is the completion marker for a consistent saved run. Saving invalidates the previous marker before changing any PNG, then writes a temporary marker and renames it to `result.json` only after all outputs succeed. If saving fails, partial PNGs may remain and must not be accepted as a complete run without that marker. The JSON records image dimensions, selected mask pixels, switches and elapsed segmentation/JNI time in milliseconds. These are per-run measurements, not an image-quality benchmark.

## Recorded device runs

The [validation record](VALIDATION.json) keeps the Android 14/API 34 emulator results separate from a physical **Google Pixel 2 running Android 10/API 29**. The same debug APK passed 14 fixture segmentation runs on each: eight procedural ellipse operator combinations, four coffee runs at the 400-pixel working limit, and two coffee runs at full 600 × 400 resolution with refined alpha. The physical runs passed 70 generated-PNG checks, supplied seed-label agreement and alpha checks, six coffee-input RGB identity checks, and six matched hard/refined mask-pair comparisons.

Physical UI checks used `adb shell input swipe` to inject foreground/background strokes on the bundled coffee photograph, working-size selection, missing-seed recovery, invalidation of old previews after Clear or option changes, and the credits/license viewer. Phone gallery import and interrupted-save fault injection were not repeated; the existing evidence for those paths is from the emulator. The screenshot above remains an emulator screenshot.

On this Pixel 2, the recorded coffee calls took about 5.8–7.0 seconds with the 400-pixel working limit and 10.6–17.4 seconds at full resolution. These are individual app-recorded segmentation/JNI intervals from fresh app starts, not a general phone benchmark. API 26 remains a declared minimum without a runtime test; Linux execution and photographic accuracy remain unverified.

## Device or emulator smoke run

Use a connected, authorized ARM64 Android device running API 26 or later, or install an ARM64 emulator image and create a task-scoped AVD with `avdmanager`. Use an explicit device serial when more than one device is connected. Keep the selected device or emulator awake and unlocked for the screenshots. With it running:

```sh
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -S -n org.awetzler.cutout.updated/.MainActivity \
  --ez run_fixture true --ez figtree true --ez level_set false --ez refine_alpha false
adb logcat -d -s CutOutUpdated:I '*:S'
adb pull /sdcard/Android/data/org.awetzler.cutout.updated/files/last-run ./last-run
```

`run_fixture` loads a newly generated two-color ellipse with fixed scribbles and segments it. Add `--es example coffee` to run the bundled 600 × 400 photograph. The unchanged photo is by Rachel Michetti, courtesy of Pikolo Espresso Bar via scikit-image, under CC0; its [provenance and example scribbles](../validation/fixtures/README.md) distinguish the image from new inputs. Repeat with `figtree false`, `level_set true` or `refine_alpha true` to exercise the separate operator choices. A successful launch alone is not a segmentation test. The [Android validation record](VALIDATION.json) records actual runs and their scope. The optional Python 3 integration runner exercises eight ellipse operator combinations, four coffee runs at the default limit and two coffee runs at full input resolution:

```sh
python3 tools/smoke.py --serial emulator-5580 --output build/android-smoke
```

Replace `emulator-5580` with the serial reported by `adb devices` for your selected phone or emulator. Back up any generated `files/last-run` outputs you want to retain before running the helper. This command restarts this updated app, replaces those last-run outputs and stores fresh PNG/JSON/log/screenshot evidence locally. The helper reads process-filtered CutOut logs and checks foreground focus before a capture; its tested revision is recorded in the validation JSON. Keep personal-phone screenshots private unless they have been inspected and cleared for sharing. It checks completion, dimensions, nontrivial masks and agreement of binary masks with/without alpha refinement. It does not measure photographic accuracy. The later crash-diagnostic change retained the app’s fatal trace after an intentionally induced Java crash. A normal recheck completed three cases before the phone switched to Camera and the screenshot guard stopped it; this is separate from the earlier complete 14-case run. Native fatal signal lines are included in the app-PID filter, but a native fault was not injected and full tombstones from separate crash-dump processes are not collected.

## Attribution and dependencies

Read the [original implementation guide](../../docs/original-android.md) and [supplementary license references](../../docs/licenses/README.md). This runner links the adapted recovered core, including ANN, FIGTree and MegaWave2 code. Building it does not assign a blanket license to those sources or to the recovered project. The unchanged launcher icon comes from the recovered Android resources. OpenCV 4.12.0, its dependency license texts from the selected SDK, and supplementary recovered dependency notices are copied into APK assets during the build; the original SDK license files remain in the installed dependency. Source changes, their provenance and original attribution belong to the native restoration guide.
