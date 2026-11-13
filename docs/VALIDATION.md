# Validation and limitations

Validation applies to the named updated implementation and recorded environment. The unchanged historical Eclipse application has not been rebuilt. Test counts describe cases, not benchmark photographs or general correctness.

## Records and reproduction

| Component | Instructions | Recorded results |
| --- | --- | --- |
| Native operators and CLI | [Validation tools](../updated/validation/README.md) | [macOS](../updated/validation/VALIDATION.json) · [Linux](../updated/validation/LINUX_VALIDATION.json) |
| Local drawing interface | [Desktop guide](../updated/desktop/README.md) | [Browser record](../updated/desktop/VALIDATION.json) |
| Android interface | [Android guide](../updated/android/README.md) | [Android record](../updated/android/VALIDATION.json) |
| iOS interface | [iOS guide](../updated/ios/README.md) | [iOS record](../updated/ios/VALIDATION.json) |
| Python exercise | [Exercise guide](../updated/README.md) | [Earlier record](../updated/VALIDATION.json) |

Platform guides describe generic setup. JSON records retain compiler, library, operating-system, device and build identities needed to interpret or reproduce a particular run. Results from another platform do not validate iOS or an untested Android device/API combination.

## Native and drawing-interface checks

The native record reports 31 named checks under AddressSanitizer and UndefinedBehaviorSanitizer, repeated with `-funsigned-char`. One CTest executable contains those checks; a single stress check includes 40 generated-image/operator runs. Sanitizers cover the port and bundled ANN/FIGTree/MegaWave sources, not separate OpenCV or system libraries.

The 25 CLI cases comprise 15 successful runs, including eight density/distance/alpha combinations on one photograph, and 10 expected input rejections. Six isolated comparisons compile hash-verified original function bodies: those bodies fail the selected regression while their updated counterparts pass. These comparisons do not execute the original APK.

The drawing-interface record covers eight actual browser-to-native photographic operator combinations, uploads, single-point/crossing strokes, undo/clear, PNG download, stale-result invalidation and input/origin errors. Its layout checks cover 390- and 1440-pixel widths.

## Android checks

The Android record separates 14 emulator segmentation runs from 14 runs on one physical Pixel 2 with Android 10/API 29 using the same debug APK. Each run set contains eight procedural ellipse cases, four coffee cases at a 400-pixel working limit and two coffee cases at full input resolution. Seed labels, saved PNGs, alpha output and six hard/refined binary-mask pairs were checked. These are repeated execution cases on two environments, not 28 different algorithm cases.

Interface checks and their limits belong to the Android record. In particular, physical-device gallery import and interrupted-save fault injection were not repeated. The log-helper record separately describes retained-process Java-crash diagnostics and a partial foreground-guard check; neither is another complete segmentation run set. Debug APK execution does not establish a store release or support for every declared API level.

## iOS checks

The iOS record covers a development-signed Debug build on one physical iPhone 12 with iOS 18.0.1, built with Xcode 16.1 and OpenCV 4.12.0. Fourteen saved segmentation cases use the same case structure as Android: eight ellipse combinations, four coffee cases at a 400-pixel working limit and two full-input coffee cases. Independent checks cover all 70 PNGs, dimensions, seed labels, alpha, six hard/refined mask pairs and RGB consistency. All six coffee inputs match the pinned photograph. These are integration and output-consistency checks, not an accuracy benchmark.

The UI-test target builds, but the recorded attempt stopped while enabling iOS automation before any test case started. Touch drawing, picker import, option/error interactions and share-sheet behavior therefore have no completed iOS UI-test result. A screenshot of the actual running application was reviewed for layout; that does not validate those interactions. Simulator execution, other iPhone/iOS combinations and the declared iOS 16 minimum remain untested.

## Python exercise

The earlier Python record reports 16 unit tests, five notebook cells and 1,005 local-stencil comparisons against a hash-verified function from the related prototype. Maximum absolute difference was `7.11e-15` for a dimensionless unit-grid example. That tests one numerical update; it does not establish whole-grid MEX equivalence, MATLAB execution or Android behavior.

Its procedural disk example produced IoU `1.000` with the selected edge cost and `0.901` without it. These are measurements on one deliberately simple generated input. The new queue, boundaries, seeds and image-to-cost composition differ from the incomplete prototype. Notebook execution and Colab setup are documented separately in the exercise guide.

## Output and compatibility limits

The coffee photograph has no independent reference mask, and no photographic accuracy benchmark is reported. Pixel-grid examples retain some wooden background; level-set examples retain ragged boundaries or small fragments. Do not generalize a visual comparison on this one input.

Updated interfaces recompute segmentation from all supplied strokes. They do not recreate the historical cached `ADD_FRONT`/`ADD_BACK` interaction, camera capture, composition, blur or grayscale tools. The original Eclipse project and related MATLAB/MEX prototype have missing dependencies described in the [source guide](original-android.md) and exercise guide.

Use each platform record for completed execution evidence. A generic build recipe or declared minimum version is not a runtime test. Cross-platform bitwise equivalence and segmentation accuracy remain unestablished.
