#!/usr/bin/env python3
"""Auditable ordered-cell smoke runner for this repository's trusted notebook.

This executes Python cells with the same privileges as Python itself; it is
not a sandbox or Jupyter kernel. Inspect notebook code before using it.
"""

from pathlib import Path
import contextlib
import io
import json
import sys


def run(path: Path) -> int:
    notebook = json.loads(path.read_text())
    if notebook.get('nbformat') != 4 or not isinstance(notebook.get('cells'), list):
        raise ValueError('expected an nbformat-4 notebook')
    namespace = {'__name__':'__notebook__'}
    executed = 0
    for index, cell in enumerate(notebook['cells']):
        if cell['cell_type'] != 'code':
            continue
        source = cell['source']
        code = ''.join(source) if isinstance(source, list) else source
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            exec(compile(code, f'{path.name}:cell-{index}', 'exec'), namespace)
        if output.getvalue():
            print(output.getvalue(), end='')
        executed += 1
    print(f'Executed {executed} code cells; notebook file left clean and unchanged.')
    return executed


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: python3 tools/run_notebook.py notebooks/fmm_scribbles.ipynb')
    run(Path(sys.argv[1]))
