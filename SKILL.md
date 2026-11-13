# Maintaining cutout-android

Read SUMMARY.md, then this file, then README.md. Inspect relevant code, original usage records and any updated/VALIDATION.json before changing technical claims.

## Repository workflow

Original Android source is preserved under source/original/android-app. Read docs/original-android.md and source/ANDROID_PROVENANCE.json before build or algorithm claims. JNI DoKernelEstimation is the entry point; the source includes independent density-estimation and level-set switches. OpenCV 2.4.9 and an external multitouch-controller library are absent. No Android installation/build procedure is verified; do not present historical configuration as a successful modern setup.

Python 3.10+ required. From updated/: python3 scribble_demo.py --size 48 --edge-weight 24 --output build/demo.svg; python3 -m unittest discover -s tests -v. See updated/README.md and source/PROVENANCE.json.

Treat these commands as scoped component or historical inspection procedures exactly as documented. Do not try running applications whose dependencies or hardware are unavailable just to check prose edits. When code changes, run the affected component checks and record actual environment, skips and limits.

## Preservation and claims

Keep original source bytes, submodule pins, provenance manifests, student/third-party credits and licenses intact. Do not edit upstream README files in submodules or historical source documentation to modernize the wrapper. Put any future implementation or dependency adaptation in a separately labeled updated/ area with its own tests.

Android original files must match source/ANDROID_PROVENANCE.json. Supplementary license references live in docs/licenses with retrieval URLs/hashes; they are separate from the received snapshot and do not assign a repository-wide license. Retain ANN, FIGTree and MegaWave2/module credits, Luke Hutchison's MIT notice, AOSP's Apache 2.0 notice and the karooolek annotation. Preserve the original About text in res/values/strings.xml. Elad Richardson's dated permission to host recovered project code is his permission only and does not speak for other credited contributors. It does not resolve the separate prototype's authorship or imply consent to a new manuscript. Keep correspondence, access identifiers and the incoming ZIP private. Inspect new archives, source and raster metadata/content before publication.

Queue, accepted-neighbor handling, boundaries, multiple seeds, image-to-cost rule and label composition are new. All updated images/scribbles are procedural. MATLAB/MEX, Android and Linux runtime are unvalidated.

Before each commit, review and synchronize SUMMARY.md and this file. Validate Markdown/relative links, retained figure paths and diff scope. Keep historical figures distinct from current output, and synthetic examples distinct from measurements. Explain what unit-test counts cover; do not convert source inspection or failed builds into compatibility, correctness or benchmark claims. Do not claim an external review unless it actually occurred.
