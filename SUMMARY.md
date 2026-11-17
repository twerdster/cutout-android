# CutOut: current maintenance context

Read this file, then SKILL.md, README.md and linked implementation/validation guides.

## Original project and preservation

Object segmentation from foreground/background scribbles by Elad Richardson, supervised by Anastasia Dubrovina and Aaron Wetzler. Original Android source and notices are preserved under source/original/android-app/. The separate C++17/OpenCV core has documented density, distance, alpha, bounds and ownership repairs.

## Current work — 2026-11-17

The existing deployed WebAssembly app was rechecked on 2026-11-17: real coffee segmentation and cutout PNG download worked. Browser/runtime code, dependency pins and package inputs remain unchanged by this concise README update. Detailed former overview/Python exercise is retained as updated/DETAILS.md. Existing native/mobile results remain separately dated.

Aaron requested concise natural README prose and a runnable demo in each Updated section. A real tools-disabled Claude review and follow-up inform the wording; factual suggestions are checked against source and records. The previous full README is preserved as PROJECT_RECORD.md. Existing updated-guide anchors remain available; lengthy implementation/recovery records are linked rather than repeated in the main README.

## Checks and maintenance

Preserve source manifests, copied dependency licenses and native operators. New interfaces remain under updated/. Read source manifests, docs/ATTRIBUTION.md, docs/VALIDATION.md and affected platform guide before code changes. Keep ordered scribbles, single-pointer ownership, input/result invalidation, bounded local worker and cancellation/recovery. Browser inputs composite on white and resize to at most 1,000 pixels. Keep compiler prefix maps and artifact path checks enabled. Rebuild browser package only if bundled browser/native source or package documentation changes; root/updated entrypoint prose is not a package input.

New static demonstration checks are in updated/demo/VALIDATION.json when present. Actual Chrome desktop/390-pixel interactions were checked; a narrow viewport is not a physical device. No hardware, live microphone/camera recognition or cross-browser coverage is added. Review and synchronize SUMMARY.md and SKILL.md before every commit. Keep private evidence, account details and machine paths outside the repository.
