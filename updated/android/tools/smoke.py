#!/usr/bin/env python3
"""Run the updated APK on one explicitly selected device/emulator.

Exercises fixture segmentation through the actual Activity/JNI/native chain.
Outputs are execution evidence for two fixtures, not an accuracy benchmark.
"""
import argparse
import itertools
import json
from pathlib import Path
import shutil
import subprocess
import time

PACKAGE = "org.awetzler.cutout.updated"
ACTIVITY = PACKAGE + "/.MainActivity"
REMOTE = "/sdcard/Android/data/" + PACKAGE + "/files/last-run"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adb", default="adb")
    parser.add_argument("--serial", required=True, help="Exact adb device serial")
    parser.add_argument("--output", type=Path, default=Path("build/android-smoke"))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    adb = [args.adb, "-s", args.serial]

    def call(*words, check=True):
        return subprocess.run([*adb, *words], capture_output=True, check=check, timeout=30)

    def text(*words, **kwargs):
        return call(*words, **kwargs).stdout.decode(errors="replace")

    text("shell", "pm", "path", PACKAGE)
    cases = [
        *(("ellipse", *flags, 400) for flags in itertools.product([False, True], repeat=3)),
        *(("coffee", True, level, alpha, 400) for level in [False, True] for alpha in [False, True]),
        *(("coffee", True, level, True, 0) for level in [False, True]),
    ]
    records = []
    for example, figtree, level_set, refine_alpha, max_dimension in cases:
        name = f"{example}-figtree-{int(figtree)}-levelset-{int(level_set)}-alpha-{int(refine_alpha)}"
        if max_dimension == 0:
            name += "-full"
        directory = args.output / name
        directory.mkdir(exist_ok=True)
        text("shell", "am", "force-stop", PACKAGE)
        # Remove only this runner's generated outputs; source and imported images are untouched.
        text("shell", "rm", "-rf", REMOTE)
        text("logcat", "-c")
        start = time.monotonic()
        text("shell", "am", "start", "-S", "-n", ACTIVITY,
             "--ez", "run_fixture", "true", "--es", "example", example,
             "--ez", "figtree", str(figtree).lower(),
             "--ez", "level_set", str(level_set).lower(),
             "--ez", "refine_alpha", str(refine_alpha).lower(),
             "--ei", "max_dimension", str(max_dimension))
        result = None
        while time.monotonic() - start < 90:
            try:
                result = json.loads(text("shell", "cat", REMOTE + "/result.json", check=False))
                break
            except json.JSONDecodeError:
                time.sleep(0.25)
        logs = text("logcat", "-d", "-s", "CutOutUpdated:I", "AndroidRuntime:E", "*:S")
        (directory / "logcat.txt").write_text(logs, encoding="utf-8")
        if result is None:
            raise RuntimeError(f"{name}: no completed result within 90 seconds; inspect {directory}/logcat.txt")
        expected = {"figtree": figtree, "level_set": level_set, "refine_alpha": refine_alpha, "max_dimension": max_dimension}
        if any(result.get(key) != value for key, value in expected.items()):
            raise RuntimeError(f"{name}: returned operator settings differ from requested settings")
        expected_size = (600, 400) if example == "coffee" else (128, 96)
        if (result.get("width"), result.get("height")) != expected_size:
            raise RuntimeError(f"{name}: unexpected image dimensions")
        if not 0 < result.get("selected_pixels", 0) < expected_size[0] * expected_size[1]:
            raise RuntimeError(f"{name}: mask is empty or selects the entire image")
        if "CUTOUT_ERROR" in logs or "FATAL EXCEPTION" in logs:
            raise RuntimeError(f"{name}: app reported a runtime error")
        files = directory / "files"
        if files.exists():
            shutil.rmtree(files)  # Prevent adb pull nesting stale results on a repeated check.
        call("pull", REMOTE, str(files))
        (directory / "screenshot.png").write_bytes(call("exec-out", "screencap", "-p").stdout)
        record = {"scenario": name, "success": True, **result, "wall_seconds": time.monotonic() - start}
        records.append(record)
        (args.output / "runs.json").write_text(json.dumps(records, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(record), flush=True)

    for example in ["ellipse", "coffee"]:
        for figtree, level_set in itertools.product([False, True], repeat=2):
            if example == "coffee" and not figtree:
                continue
            prefix = f"{example}-figtree-{int(figtree)}-levelset-{int(level_set)}-alpha-"
            hard = args.output / (prefix + "0") / "files/mask.png"
            refined = args.output / (prefix + "1") / "files/mask.png"
            if hard.read_bytes() != refined.read_bytes():
                raise RuntimeError(f"{prefix}: binary masks differ when alpha refinement changes")
    print("All 14 Activity/JNI segmentation runs passed; six hard/refined mask pairs agree.")


if __name__ == "__main__":
    main()
