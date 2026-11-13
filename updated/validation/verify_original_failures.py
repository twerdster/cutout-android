#!/usr/bin/env python3
"""Compare isolated, hash-verified original methods with updated regressions.

This compiles selected original function bodies, not the historical application.
OpenCV include/lib flags are read from pkg-config or --opencv-prefix. Each unsafe
original case runs in a separate process with address/undefined sanitizers.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

REPO = Path(__file__).resolve().parents[2]
CASES = ["empty_path", "crop_empty", "crop_single_pixel", "crop_full",
         "lab_bins_clamp", "alpha_degenerate_distances"]

def extract(source: str, signature: str) -> str:
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--updated-regressions", type=Path, required=True)
    parser.add_argument("--opencv-prefix", type=Path)
    parser.add_argument("--compiler", default="clang++")
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads((REPO / "source/ANDROID_PROVENANCE.json").read_text())
    expected = {row["path"]: row["published_sha256"] for row in manifest["files"]}
    paths = ["source/original/android-app/jni/KernelEstimation.cpp",
             "source/original/android-app/jni/Dijkstra.cpp",
             "source/original/android-app/jni/KernelEstimation.h",
             "source/original/android-app/jni/Dijkstra.h"]
    verified = {}
    for relative in paths:
        digest = hashlib.sha256((REPO / relative).read_bytes()).hexdigest()
        if expected[relative] != digest:
            raise RuntimeError(f"original source changed: {relative}")
        verified[relative] = digest
    kernel = (REPO / paths[0]).read_text()
    dijkstra = (REPO / paths[1]).read_text()
    selected = [extract(kernel, "Point3d KernelEstimation::labToRange"),
                extract(kernel, "Rect KernelEstimation::getBoundaries"),
                extract(kernel, "void KernelEstimation::updateAlpha"),
                extract(dijkstra, "std::vector<Point> Dijkstra::fillPath"),
                extract(dijkstra, "std::vector<Point> Dijkstra::simpleCompleteLine")]
    # Assertions use the same input/expectation as the updated native regression.
    harness = r'''
#include "KernelEstimation.h"
#include "Dijkstra.h"
#include <opencv2/core.hpp>
#include <sstream>
#include <cmath>
#include <limits>
#include <string>
using std::stringstream;
void showString(std::stringstream&) {}
__SELECTED__
int main(int argc,char**argv) {
 if(argc!=2)return 3;
 std::string which=argv[1];
 if(which=="empty_path")return Dijkstra::fillPath({}).empty()?0:1;
 cv::Mat alpha(4,5,CV_32FC1,cv::Scalar(0));
 if(which=="crop_empty")return KernelEstimation::getBoundaries(alpha)==cv::Rect()?0:1;
 if(which=="crop_single_pixel") {alpha.at<float>(3,4)=1;return KernelEstimation::getBoundaries(alpha)==cv::Rect(4,3,1,1)?0:1;}
 if(which=="crop_full"){alpha.setTo(1);return KernelEstimation::getBoundaries(alpha)==cv::Rect(0,0,5,4)?0:1;}
 if(which=="lab_bins_clamp"){cv::Point3f p(-200,-1000,1000);return KernelEstimation::labToRange(p,8)==cv::Point3d(0,0,7)?0:1;}
 if(which=="alpha_degenerate_distances"){
  cv::Mat f=(cv::Mat_<float>(1,3)<<0,3,0),b=(cv::Mat_<float>(1,3)<<3,0,0);
  cv::Mat fp(1,3,CV_32FC1,cv::Scalar(.6)),bp(1,3,CV_32FC1,cv::Scalar(.4)),a(1,3,CV_32FC1,cv::Scalar(.5));
  cv::Mat band(1,3,CV_8UC1,cv::Scalar(255));KernelEstimation::updateAlpha(f,b,fp,bp,band,a,1.5);
  for(int x=0;x<3;++x)if(!std::isfinite(a.at<float>(0,x))||a.at<float>(0,x)<0||a.at<float>(0,x)>1)return 1;
  return 0;
 }
 return 3;
}
'''.replace("__SELECTED__", "\n\n".join(selected))
    if args.opencv_prefix:
        flags = ["-I" + str(args.opencv_prefix / "include/opencv4"),
                 "-L" + str(args.opencv_prefix / "lib"),
                 "-Wl,-rpath," + str(args.opencv_prefix / "lib"),
                 "-lopencv_core"]
    else:
        flags = shlex.split(subprocess.check_output(
            ["pkg-config", "--cflags", "--libs", "opencv4"], text=True))
    rows = []
    with tempfile.TemporaryDirectory(prefix="cutout-original-methods-") as scratch:
        scratch = Path(scratch)
        source = scratch / "original_methods.cpp"
        source.write_text(harness)
        binary = scratch / "original_methods"
        command = [args.compiler, "-std=c++17", "-O1", "-g", "-fsanitize=address,undefined",
                   "-fno-omit-frame-pointer", "-I" + str(REPO / "source/original/android-app/jni"),
                   str(source), "-o", str(binary), *flags]
        subprocess.run(command, check=True, capture_output=True, text=True)
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
        for case in CASES:
            original = subprocess.run([str(binary), case], env=environment, capture_output=True, text=True, timeout=30)
            updated = subprocess.run([str(args.updated_regressions.resolve()), case], env=environment,
                                     capture_output=True, text=True, timeout=30)
            sanitized = "runtime error:" in original.stderr or "AddressSanitizer" in original.stderr
            rows.append({"case": case, "original_exit_code": original.returncode,
                         "original_rejected_by_check_or_sanitizer": original.returncode == 1 or sanitized,
                         "original_sanitizer_report": sanitized,
                         "updated_exit_code": updated.returncode,
                         "updated_passed": updated.returncode == 0,
                         "updated_output": updated.stdout.strip()})
    passed = all(row["original_rejected_by_check_or_sanitizer"] and row["updated_passed"] for row in rows)
    report = {"scope": "Selected original function bodies compiled in isolation; not an original Android application build.",
              "original_source_sha256": verified, "cases": rows, "passed": passed}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"cases": len(rows), "passed": passed, "report": str(args.report)}))
    return 0 if passed else 1

if __name__ == "__main__":
    raise SystemExit(main())
