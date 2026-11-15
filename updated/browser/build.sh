#!/usr/bin/env bash
set -euo pipefail
# Activate Emscripten 4.0.20 before running. Pass an OpenCV 4.12.0 source tree.
if [[ $# -ne 2 ]]; then
  echo 'Usage: bash updated/browser/build.sh OPENCV_SOURCE BUILD_DIRECTORY' >&2
  exit 2
fi
if ! emcc --version | head -1 | grep -q '4\.0\.20'; then
  echo 'Activate Emscripten 4.0.20 before building.' >&2
  exit 2
fi
repo_root=$(cd "$(dirname "$0")/../.." && pwd)
opencv_source=$(cd "$1" && pwd)
mkdir -p "$2"
browser_build=$(cd "$2" && pwd)
emcmake cmake -S "$opencv_source" -B "$browser_build/opencv" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$browser_build/opencv-install" \
  -DCMAKE_CXX_FLAGS=-fexceptions -DCMAKE_C_FLAGS=-fexceptions \
  -DBUILD_LIST=core,imgproc -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF \
  -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_opencv_apps=OFF \
  -DBUILD_opencv_js=OFF -DBUILD_JAVA=OFF -DBUILD_opencv_python3=OFF \
  -DWITH_IPP=OFF -DWITH_OPENCL=OFF -DWITH_TBB=OFF -DWITH_PTHREADS_PF=OFF \
  -DWITH_ITT=OFF -DWITH_PROTOBUF=OFF -DWITH_ADE=OFF -DCV_ENABLE_INTRINSICS=OFF \
  -DWITH_FFMPEG=OFF -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF -DWITH_V4L=OFF \
  -DWITH_TIFF=OFF -DWITH_PNG=OFF -DWITH_JPEG=OFF -DWITH_WEBP=OFF \
  -DWITH_OPENEXR=OFF -DWITH_OPENJPEG=OFF -DWITH_JASPER=OFF -DWITH_AVIF=OFF \
  -DCPU_BASELINE= -DCPU_DISPATCH=
cmake --build "$browser_build/opencv" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
cmake --install "$browser_build/opencv"
emcmake cmake -S "$repo_root/updated/browser" -B "$browser_build/core" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DOpenCV_DIR="$browser_build/opencv-install/lib/cmake/opencv4"
cmake --build "$browser_build/core" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
python3 "$repo_root/updated/browser/package.py" "$browser_build/core" "$browser_build/site" \
  "$opencv_source" "$(dirname "$(command -v emcc)")/LICENSE"
