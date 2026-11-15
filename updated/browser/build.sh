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
sdk_root=$(cd "$(dirname "$(command -v emcc)")/../.." && pwd)
# Error messages in OpenCV use __FILE__; keep build-machine paths out of Wasm.
map_flags=$(python3 - "$repo_root" "$opencv_source" "$browser_build" "$sdk_root" <<'PY'
import shlex, sys
pairs=zip(sys.argv[1:], ('cutout','opencv','build','emsdk'))
flags=['-fexceptions']
for source, target in pairs:
    flags.extend((f'-ffile-prefix-map={source}={target}', f'-fmacro-prefix-map={source}={target}'))
print(shlex.join(flags))
PY
)
emcmake cmake -S "$opencv_source" -B "$browser_build/opencv" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$browser_build/opencv-install" \
  "-DCMAKE_CXX_FLAGS=$map_flags" "-DCMAKE_C_FLAGS=$map_flags" \
  -DBUILD_LIST=core,imgproc -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF \
  -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_opencv_apps=OFF \
  -DBUILD_opencv_js=OFF -DBUILD_JAVA=OFF -DBUILD_opencv_python3=OFF \
  -DWITH_IPP=OFF -DWITH_OPENCL=OFF -DWITH_TBB=OFF -DWITH_PTHREADS_PF=OFF \
  -DWITH_ITT=OFF -DWITH_PROTOBUF=OFF -DWITH_ADE=OFF -DCV_ENABLE_INTRINSICS=OFF \
  -DWITH_FFMPEG=OFF -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF -DWITH_V4L=OFF \
  -DWITH_TIFF=OFF -DWITH_PNG=OFF -DWITH_JPEG=OFF -DWITH_WEBP=OFF \
  -DWITH_OPENEXR=OFF -DWITH_OPENJPEG=OFF -DWITH_JASPER=OFF -DWITH_AVIF=OFF \
  -DBUILD_INFO_SKIP_TIMESTAMP=ON \
  -DCPU_BASELINE= -DCPU_DISPATCH=
# OpenCV also embeds its configure summary, including the compiler flags above.
# Normalize only that generated metadata; the dependency source stays unchanged.
python3 - "$browser_build/opencv/modules/core/version_string.inc" \
  "$repo_root" "$opencv_source" "$browser_build" "$sdk_root" <<'PY'
from pathlib import Path
import sys
path=Path(sys.argv[1])
text=path.read_text()
for source,target in zip(sys.argv[2:], ('cutout','opencv','build','emsdk')):
    text=text.replace(source,target)
path.write_text(text)
PY
cmake --build "$browser_build/opencv" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
cmake --install "$browser_build/opencv"
emcmake cmake -S "$repo_root/updated/browser" -B "$browser_build/core" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DOpenCV_DIR="$browser_build/opencv-install/lib/cmake/opencv4" \
  "-DCMAKE_CXX_FLAGS=$map_flags" "-DCMAKE_C_FLAGS=$map_flags"
cmake --build "$browser_build/core" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
python3 "$repo_root/updated/browser/package.py" "$browser_build/core" "$browser_build/site" \
  "$opencv_source" "$(dirname "$(command -v emcc)")/LICENSE"
