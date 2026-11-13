#!/usr/bin/env bash
# Updated 2026 isolated OpenCV build; no system packages are replaced.
set -euo pipefail
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 OPENCV_4_12_0_SOURCE BUILD_DIRECTORY INSTALL_PREFIX" >&2
    exit 2
fi
cutout_source=$1
cutout_build=$2
cutout_prefix=$3
if [ ! -f "$cutout_source/modules/core/include/opencv2/core.hpp" ]; then
    echo "Source directory does not contain an OpenCV checkout." >&2
    exit 2
fi
cmake -S "$cutout_source" -B "$cutout_build" -G Ninja \
    -DCMAKE_INSTALL_PREFIX="$cutout_prefix" -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_LIST=core,imgproc,imgcodecs -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF \
    -DBUILD_opencv_apps=OFF -DBUILD_JAVA=OFF \
    -DBUILD_opencv_python3=OFF -DBUILD_opencv_python2=OFF \
    -DWITH_TBB=OFF -DWITH_OPENEXR=OFF -DWITH_OPENJPEG=OFF -DWITH_JASPER=OFF \
    -DWITH_GSTREAMER=OFF -DWITH_FFMPEG=OFF -DWITH_QT=OFF -DWITH_GTK=OFF \
    -DWITH_VTK=OFF -DWITH_OPENCL=OFF -DWITH_IPP=OFF -DWITH_PROTOBUF=OFF \
    -DWITH_1394=OFF -DWITH_AVFOUNDATION=OFF -DWITH_WEBP=OFF -DWITH_TIFF=OFF \
    -DWITH_JPEGXL=OFF -DWITH_LAPACK=OFF -DWITH_EIGEN=OFF \
    -DWITH_ACCELERATE=OFF -DWITH_AVIF=OFF -DWITH_GDCM=OFF \
    -DBUILD_PNG=ON -DBUILD_ZLIB=ON -DBUILD_JPEG=ON
cmake --build "$cutout_build" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
cmake --install "$cutout_build"
