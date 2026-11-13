# Optional OpenCV 4.12 source build

Use this recipe when a suitable OpenCV 4 development package is unavailable or when a small, separately installed dependency is preferred. It builds `core`, `imgproc` and `imgcodecs` with bundled PNG/JPEG/zlib. It disables optional GUI, video, TBB, OpenEXR and AVIF dependencies. It does not replace a system package.

The recipe requires a C++ compiler, CMake 3.22+, Ninja, `curl`, `tar` and Python 3. Run from the repository root:

```sh
mkdir -p build/opencv-src
curl -fL https://github.com/opencv/opencv/archive/refs/tags/4.12.0.tar.gz \
  -o build/opencv-4.12.0.tar.gz
python3 - <<'PY'
from pathlib import Path
import hashlib
archive = Path('build/opencv-4.12.0.tar.gz')
expected = '44c106d5bb47efec04e531fd93008b3fcd1d27138985c5baf4eafac0e1ec9e9d'
if hashlib.sha256(archive.read_bytes()).hexdigest() != expected:
    raise SystemExit('OpenCV archive SHA-256 mismatch')
print('OpenCV archive SHA-256 verified')
PY
tar -xzf build/opencv-4.12.0.tar.gz -C build/opencv-src
bash updated/native/scripts/build_opencv.sh \
  build/opencv-src/opencv-4.12.0 build/opencv-build "$PWD/build/opencv-install"
cmake -S updated/native -B build/native -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DOpenCV_DIR="$PWD/build/opencv-install/lib/cmake/opencv4"
cmake --build build/native --parallel 2
ctest --test-dir build/native --output-on-failure
```

`CMAKE_BUILD_PARALLEL_LEVEL` controls the dependency build's parallelism; the default is two compiler jobs. The script accepts source, build and install directories, uses ordinary CMake install rules and needs no administrator privileges when those directories are writable. Keep source/build/install trees on a filesystem that supports the toolchain's normal file and link operations.

The archive is the [official OpenCV 4.12.0 source](https://github.com/opencv/opencv/releases/tag/4.12.0). OpenCV's [configuration reference](https://docs.opencv.org/4.x/db/d05/tutorial_config_reference.html) explains the build options. Retain the license texts from the dependency you distribute. The [native guide](README.md) describes CutOut's inputs, outputs and runtime limits.
