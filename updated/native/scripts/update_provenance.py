#!/usr/bin/env python3
"""Refresh copied-file hashes; this script never edits the original snapshot."""
from pathlib import Path
import hashlib
import json

repo = Path(__file__).resolve().parents[3]
native = repo / 'updated/native'
original = repo / 'source/original/android-app'
records = []

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def record(source, updated):
    records.append({
        'original_path': source.relative_to(repo).as_posix(),
        'original_sha256': digest(source),
        'updated_path': updated.relative_to(repo).as_posix(),
        'updated_sha256': digest(updated),
        'byte_identical': source.read_bytes() == updated.read_bytes(),
    })

for path in sorted((native / 'legacy').rglob('*')):
    if path.is_file():
        source = original / 'jni' / path.relative_to(native / 'legacy')
        if source.is_file():
            record(source, path)
for directory in ('ann', 'figtree'):
    for path in sorted((native / 'vendor' / directory).rglob('*')):
        if path.is_file():
            record(original / 'src' / directory / path.relative_to(native / 'vendor' / directory), path)
for path in sorted((native / 'vendor/include').rglob('*')):
    if path.is_file():
        record(original / 'include' / path.relative_to(native / 'vendor/include'), path)

manifest = {
    'implementation': 'Updated 2026 native CutOut port',
    'date': '2026-11-13',
    'original_implementation_author': 'Elad Richardson',
    'updated_wrapper_and_fixes': 'Codex for Aaron Wetzler',
    'original_manifest': '../../source/ANDROID_PROVENANCE.json',
    'preservation': 'The received Android snapshot is unchanged. Copied files and fixes live separately under updated/native.',
    'terms': 'Mixed original terms and notices are retained. No blanket repository license is assigned. See ../../docs/original-android.md and ../../docs/licenses/README.md.',
    'dependency_credits': {
        'ANN': 'Sunil Arya, David Mount and the University of Maryland',
        'FIGTree': 'Vlad Morariu, Vikas Raykar and Changjiang Yang',
        'FLST': 'Pascal Monasse',
        'MegaWave2': 'Original system/module authors; CMLA, Ecole Normale Superieure de Cachan, 61 avenue du President Wilson, 94235 Cachan cedex, France',
    },
    'intentional_changes': [
        'OpenCV 4 includes; remove unused GUI/features/photo dependencies and Android-only logging.',
        'C++ const queue comparator; remove obsolete register specifiers in the updated MegaWave adapter.',
        'Ignore empty stroke paths; validate image/seed/option ranges and reject conflicting reduced seeds.',
        'Clamp Lab bins, use actual histogram rows for strided input, remove unused accumulator and center the third Gaussian offset with radius/2.',
        'Use RAII buffers and check FIGTree status; a constant source/target Lab domain uses the exact Gaussian sum N before normalization.',
        'Replace time-seeded process-global FIGTree random choices with fixed-seed-0 thread-local mt19937 choices reset for each PDF; reject nonpositive choice counts.',
        'Finite bounded alpha for zero/unreachable distances; deterministic band-likelihood initialization.',
        'Correct optional boundary color replacement to index the background sample vector.',
        'Return inclusive crop bounds and an empty rectangle for an empty alpha mask.',
        'Use explicitly signed MegaWave hole-count lookup entries on platforms with unsigned char.',
        'Replace recursive tree-edge extraction with iteration over existing shape parent links; retain the same topology and edge weights.',
        'Replace recovered no-op MegaWave error handler with exceptions for errors and stderr output for warnings.',
        'Enforce explicitly supplied foreground/background labels on full-size mask and alpha.',
        'Expose full-size hard/refined alpha output; default refined band is 3 pixels rather than original JNI 10/scale, with no historical crop.',
        'Serialize legacy global state through a mutex; recompute all PDFs per invocation rather than expose cached ADD_FRONT/ADD_BACK interaction.',
    ],
    'copied_file_count': len(records),
    'copied_files': records,
    'opencv_build': {
        'scope': 'isolated macOS desktop build; Android SDK details are recorded separately',
        'version': '4.12.0',
        'source_url': 'https://github.com/opencv/opencv/archive/refs/tags/4.12.0.tar.gz',
        'source_archive_sha256': '44c106d5bb47efec04e531fd93008b3fcd1d27138985c5baf4eafac0e1ec9e9d',
        'recipe': 'scripts/build_opencv.sh',
        'modules': ['core', 'imgproc', 'imgcodecs'],
        'external_opencv_is_instrumented': False,
    },
}
(native / 'PROVENANCE.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'Recorded {len(records)} copied files; originals read only.')
