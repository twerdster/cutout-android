#!/usr/bin/env python3
"""Record completed native/CLI checks and their source hashes.

This reads existing check outputs. It never substitutes a workflow definition or
source inspection for execution, and does not modify historical source records.
"""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import platform
from pathlib import Path
import re
import subprocess
import sys

REPO = Path(__file__).resolve().parents[3]
CLI_SUCCESS_CASES = (
    *(f"coffee-{density}-{distance}-{alpha}" for density in ("figtree", "histogram")
      for distance in ("pixel", "levelset") for alpha in ("hard", "refined")),
    "coffee-figtree-pixel-hard-repeat",
    *(f"constant-{density}-{distance}" for density in ("figtree", "histogram")
      for distance in ("pixel", "levelset")),
    "reduced-large", "unreduced",
)
CLI_ERROR_CASES = (
    "reject-empty-foreground", "reject-overlapping-labels", "reject-coordinate-outside",
    "reject-fractional-coordinate", "reject-negative-brush", "reject-malformed-json",
    "reject-unknown-density", "reject-huge-histogram", "reject-seed-collision", "reject-missing-image",
)
ORIGINAL_CASES = ("empty_path", "crop_empty", "crop_single_pixel", "crop_full",
                  "lab_bins_clamp", "alpha_degenerate_distances")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def checks(path):
    log = path.read_text()
    names = re.findall(r"^(?:\d+:\s*)?PASS (.+)$", log, re.MULTILINE)
    require(len(names) == 31 and len(set(names)) == 31,
            f"{path.name}: expected 31 distinct native checks")
    require("31 passed, 0 failed" in log and "100% tests passed" in log,
            f"{path.name}: completed successful CTest output missing")
    return names


def first_line(command):
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    return result.stdout.splitlines()[0]


def cache_values(path):
    return {match.group(1): match.group(2) for line in path.read_text().splitlines()
            if (match := re.match(r"([^#/:][^:]*):[^=]+=(.*)", line))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-log", type=Path, required=True)
    parser.add_argument("--unsigned-log", type=Path, required=True)
    parser.add_argument("--native-cache", type=Path, required=True)
    parser.add_argument("--unsigned-cache", type=Path, required=True)
    parser.add_argument("--cli-report", type=Path, required=True)
    parser.add_argument("--original-report", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    # A previous successful marker must not survive a failed refresh.
    args.output.unlink(missing_ok=True)
    normal_cache = cache_values(args.native_cache)
    unsigned_cache = cache_values(args.unsigned_cache)
    require(normal_cache.get("CUTOUT_SANITIZE") == unsigned_cache.get("CUTOUT_SANITIZE") == "ON",
            "both builds must enable CUTOUT_SANITIZE")
    require("-funsigned-char" in unsigned_cache.get("CMAKE_CXX_FLAGS", "").split(),
            "unsigned-char compiler flag is absent")
    compiler = Path(normal_cache["CMAKE_CXX_COMPILER"])
    require(compiler.resolve() == Path(unsigned_cache["CMAKE_CXX_COMPILER"]).resolve(),
            "normal and unsigned-char builds use different compilers")
    normal = checks(args.native_log)
    unsigned = checks(args.unsigned_log)
    require(normal == unsigned, "normal and unsigned-char runs contain different checks")
    cli = json.loads(args.cli_report.read_text())
    original = json.loads(args.original_report.read_text())
    require(cli.get("passed") is True and len(cli.get("cases", [])) == 25,
            "expected 25 completed successful CLI checks")
    require(all(case.get("passed") is True for case in cli["cases"]), "a CLI case failed")
    names = [case["name"] for case in cli["cases"]]
    require(len(set(names)) == 25 and set(names) == set(CLI_SUCCESS_CASES + CLI_ERROR_CASES),
            "CLI case identities differ from the expected 25")
    require(sum(case.get("expected_error") is False for case in cli["cases"]) == 15 and
            sum(case.get("expected_error") is True for case in cli["cases"]) == 10,
            "CLI cases must contain exactly 15 successful inputs and 10 expected errors")
    for case in cli["cases"]:
        expected_error = case["name"] in CLI_ERROR_CASES
        require(case["expected_error"] is expected_error and case["exit_code"] == (2 if expected_error else 0),
                f"{case['name']}: wrong expected-error class or process exit")
    require(original.get("passed") is True and len(original.get("cases", [])) == 6,
            "expected six completed original/updated comparisons")
    require(all(row.get("updated_passed") and row.get("original_rejected_by_check_or_sanitizer")
                for row in original["cases"]), "an original/updated comparison failed")
    require(tuple(row["case"] for row in original["cases"]) == ORIGINAL_CASES,
            "original/updated case identities differ")
    require(original.get("unknown_updated_case_rejected") is True,
            "updated regression selector was not checked with an unknown case")
    preserved = {}
    for manifest_name, expected_count in (("source/ANDROID_PROVENANCE.json", 319),
                                           ("source/PROVENANCE.json", 3)):
        manifest = json.loads((REPO / manifest_name).read_text())
        require(len(manifest["files"]) == expected_count, f"{manifest_name}: unexpected original count")
        for row in manifest["files"]:
            require(digest(REPO / row["path"]) == row["published_sha256"],
                    f"original changed: {row['path']}")
        preserved[manifest_name] = len(manifest["files"])
    successful = [case["result"] for case in cli["cases"] if not case["expected_error"]]
    versions = sorted({result["opencv_version"] for result in successful})
    header_versions = sorted({result["opencv_header_version"] for result in successful})
    source_hashes = {}
    native = REPO / "updated/native"
    native_paths = [native / "CMakeLists.txt"]
    for directory in ("include", "src", "legacy", "vendor", "scripts"):
        native_paths.extend((native / directory).rglob("*"))
    for path in sorted(native_paths):
        if path.is_file() and "__pycache__" not in path.parts and path.suffix != ".pyc":
            source_hashes[path.relative_to(REPO).as_posix()] = digest(path)
    for name in ("native_regressions.cpp", "check_cli.py", "verify_original_failures.py"):
        path = REPO / "updated/validation" / name
        source_hashes[path.relative_to(REPO).as_posix()] = digest(path)
    workflow = REPO / ".github/workflows/native-linux.yml"
    source_hashes[workflow.relative_to(REPO).as_posix()] = digest(workflow)
    fixtures = REPO / "updated/validation/fixtures"
    for path in sorted(fixtures.rglob("*")):
        if path.is_file():
            source_hashes[path.relative_to(REPO).as_posix()] = digest(path)
    fixture_provenance = json.loads((fixtures / "PROVENANCE.json").read_text())
    photograph = next(row for row in fixture_provenance["files"] if row["path"] == "coffee.png")
    require(digest(fixtures / "coffee.png") == photograph["sha256"] == cli["fixture_sha256"],
            "recorded photograph hashes disagree")
    require(cli["fixture_license"] == photograph["license"], "recorded photograph licenses disagree")
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip()
    report = {
        "schema_version": 1,
        "checked_utc": datetime.now(timezone.utc).isoformat(),
        "commit": commit,
        "passed": True,
        "environment": {"system": platform.system(), "release": platform.release(),
                        "architecture": platform.machine(), "python": sys.version.split()[0],
                        "compiler": first_line([str(compiler), "--version"]),
                        "compiler_from_cmake_cache": str(compiler),
                        "cmake": first_line(["cmake", "--version"]),
                        "opencv_runtime_versions": versions, "opencv_header_versions": header_versions},
        "native_checks": {"normal": 31, "unsigned_char": 31, "names": normal,
                          "unsigned_char_flag": "-funsigned-char"},
        "cli_checks": {"cases": 25, "successful_inputs": len(successful),
                       "expected_errors": 25 - len(successful),
                       "all_four_hard_refined_masks_identical": cli["all_four_hard_refined_masks_identical"]},
        "original_comparisons": {"cases": 6, "scope": original["scope"], "results": original["cases"]},
        "preserved_originals": preserved,
        "sanitizer_scope": {"address": True, "undefined_behavior": True,
                            "external_opencv_instrumented": False, "leak_check_claimed": False},
        "photograph": {"path": "updated/validation/fixtures/coffee.png", "sha256": photograph["sha256"],
                       "license": photograph["license"], "independent_ground_truth": False},
        "scope": "Executed native regression and CLI checks, including one photograph without independent ground truth; no photographic accuracy or general platform-correctness claim.",
        "source_sha256": source_hashes,
        "evidence_sha256": {"native_ctest_log": digest(args.native_log),
                            "unsigned_char_ctest_log": digest(args.unsigned_log),
                            "native_cmake_cache": digest(args.native_cache),
                            "unsigned_char_cmake_cache": digest(args.unsigned_cache),
                            "cli_report": digest(args.cli_report), "original_report": digest(args.original_report)},
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"passed": True, "system": platform.system(), "normal": 31,
                      "unsigned_char": 31, "cli_cases": 25, "original_comparisons": 6,
                      "output": str(args.output)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
