#!/usr/bin/env python3
"""Compile only the hash-verified original update function in a new C++ driver.

No MATLAB, MEX runtime, BinaryHeap, historical binary or full app is executed.
This compares finite local stencil updates, not whole-grid solver behavior.
"""

from pathlib import Path
import hashlib
import json
import math
import random
import shutil
import subprocess
import sys
import tempfile

UPDATED = Path(__file__).resolve().parents[1]
SOURCE = UPDATED.parent / 'source/original/related-prototype/fmmMex.cpp'
EXPECTED_SHA256 = 'ba2df8bf6f00b9c56ae3a5646997b6228718b0d7118463f25b62cafa49ee8bad'
sys.path.insert(0, str(UPDATED))
from fmm import eikonal_update


def original_function() -> str:
    raw = SOURCE.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED_SHA256:
        raise ValueError('original source hash changed; review it before compiling')
    text = raw.decode().replace('\r\n', '\n')
    begin = text.index('inline void update(')
    end = text.index('\n}\n', begin) + 2
    function = text[begin:end]
    if text.count('inline void update(') != 1 or 'mex' in function.lower():
        raise ValueError('ambiguous or unexpected original function')
    return function


def compare() -> dict:
    compiler = shutil.which('clang++') or shutil.which('c++')
    if compiler is None:
        raise RuntimeError('a C++ compiler is required for this optional comparison')
    function = original_function()
    driver = '''#include <math.h>
#include <iostream>
#include <iomanip>
using namespace std;
#define IND(r,c) ((r)+(c)*height)
#define MIN(x,y) (((x)<(y))?(x):(y))
''' + function + '''
int main() {
    double a,b,f,old;
    cout << setprecision(17);
    while (cin >> a >> b >> f >> old) {
        double T[9] = {a,b,a,a,old,a,a,b,a};
        double F[9] = {f,f,f,f,f,f,f,f,f};
        update(1,1,3,3,T,F);
        cout << T[4] << "\\n";
    }
}
'''
    # T indices at center: vertical=(3,5)=a; horizontal=(1,7)=b.
    rng = random.Random(20261112)
    cases = [(1, 1, 1, 1e100), (0, 10, 1, 1e100), (2, 2.4, 1.5, 1e100),
             (1, 1, 1, 1.1), (0, 1, 1, 1e100)]
    for _ in range(1000):
        a, b, cost = rng.uniform(-3, 30), rng.uniform(-3, 30), rng.uniform(0.01, 10)
        cases.append((a, b, cost, 1e100))
    payload = ''.join(' '.join(format(x, '.17g') for x in case) + '\n' for case in cases)
    with tempfile.TemporaryDirectory(prefix='cutout-stencil-') as task_dir:
        work = Path(task_dir)
        cpp, exe = work/'stencil.cpp', work/'stencil'
        cpp.write_text(driver)
        subprocess.run([compiler, '-std=c++17', '-O2', str(cpp), '-o', str(exe)], check=True)
        run = subprocess.run([str(exe)], input=payload, capture_output=True, text=True, check=True)
    values = [float(x) for x in run.stdout.splitlines()]
    if len(values) != len(cases):
        raise AssertionError('reference output count mismatch')
    errors = []
    for actual, (a, b, cost, old) in zip(values, cases):
        expected = min(old, eikonal_update(a, b, cost))
        if not math.isclose(actual, expected, rel_tol=2e-14, abs_tol=2e-14):
            raise AssertionError(f'original local stencil differs: {actual} vs {expected}')
        errors.append(abs(actual - expected))
    return {'finite_local_cases':len(cases), 'max_absolute_error':max(errors),
            'original_sha256':EXPECTED_SHA256, 'compiler':compiler,
            'scope':'Original local update only; no legacy queue, MEX or Android app executed.'}


if __name__ == '__main__':
    print(json.dumps(compare(), indent=2))
