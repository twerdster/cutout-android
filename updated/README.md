# Fast marching and scribble exercise (updated 2026)

This guide covers the earlier Python exercise. For the restored Android C++ operators and working interfaces, see the [native](native/README.md), [desktop drawing](desktop/README.md), [Android](android/README.md) and [iOS](ios/README.md) guides. They are separate implementations with separate validation records.

This Python component computes arrival times on a grid and uses two arrival fields in a procedural scribble-segmentation exercise. It was written in 2026 by Codex for Aaron Wetzler. It uses the local numerical update in the recovered CutOut-related `fmmMex.cpp`, whose authorship remains unverified. The [Android source recovered separately on 2026-11-13](../docs/original-android.md) is not the basis of this exercise, and its original Eclipse build remains unvalidated.

The synthetic picture, scribbles, image-to-cost rule and competition between two arrival fields are newly written educational examples. They are not recovered CutOut behavior or evidence of original app accuracy.

## Run

Python 3.10+ and the standard library; no package installation, image downloads, camera or MATLAB is required. Run from this `updated/` directory:

```sh
python3 -m unittest discover -s tests -v
python3 scribble_demo.py --size 48 --edge-weight 24 --output build/demo.svg
python3 tools/run_notebook.py notebooks/fmm_scribbles.ipynb
```

Open `build/demo.svg` in a browser to see the procedural image and scribbles, both arrival fields and the new mask. Try `--edge-weight 0`, `8`, `24` or `60`. No server is started.

## Original and updated

| Related prototype | Updated component |
| --- | --- |
| MATLAB wrapper and C++ MEX function; generic heap missing | Python `heapq`, without the omitted heap implementation |
| Single 1-based `(x, y)` source | Multiple zero-based `(row, column)` seeds |
| Unguarded neighbor indexing; requires a suitable blocked border | Explicit boundaries; corner, edge and single-cell grids supported |
| Local update reads available tentative arrival values | Local update uses accepted neighbors only |
| `exp(100)` sentinel; MATLAB wrapper removes values above `exp(20)` | Positive infinity for obstacles/unreachable cells; no arbitrary finite cutoff |
| Incomplete `testScribbles.m` experiment | New procedural image/cost rule and two-label composition |
| These three prototype files do not contain an Android interface | No Android UI, capture, matting or app rebuild claimed; the separate recovered Android source is not reproduced here |

`fast_marching(costs, seeds)` accepts a rectangular grid of positive costs and zero-based `(row, column)` seed pairs. Positive infinity marks an obstacle; the returned nested list contains arrival times, with infinity for unreachable cells. Arrival times scale with the supplied cost and have no assigned physical unit.

The original local stencil solves a first-order Eikonal discretization on a unit-spaced grid. `F` is **arrival cost/slowness**, not speed. At a unit-cost source, axis steps cost one and the first diagonal is `1 + 1/sqrt(2)`, rather than exact Euclidean `sqrt(2)`. The Python expression is algebraically equivalent to the recovered quadratic but normalizes the arithmetic to avoid unnecessary large squares.

The complete queue and boundary behavior is newly implemented; whole-grid byte or numerical identity with the incomplete original MEX implementation is not claimed. No original source file was changed.

## Validation

[Validation and limitations](../docs/VALIDATION.md#python-exercise) summarizes the earlier component tests, notebook execution and local-stencil comparison. [VALIDATION.json](VALIDATION.json) records the actual environment and results. These checks cover the independent procedural exercise, not the recovered Android implementation or photographic accuracy.

Optional original-formula comparison, requiring `clang++` or another C++ compiler:

```sh
python3 harness/compare_original_stencil.py
```

The harness checks the original `fmmMex.cpp` SHA-256 before extracting the local function. It compiles no MEX wrapper, SDK, generic heap or historical binary, and uses temporary native-filesystem build files. Re-review a changed original before updating the expected hash.

## Notebook and interactive use

Open [`notebooks/fmm_scribbles.ipynb`](notebooks/fmm_scribbles.ipynb) in Jupyter, or clone this repository into Colab and change into `updated/` before running its cells. For example, in a separate Colab setup cell:

```python
!git clone https://github.com/twerdster/cutout-android.git
%cd cutout-android/updated
```

The notebook itself downloads nothing. Change its grid size and edge weight, rerun, and inspect the image, scribbles, two propagation fields and mask. The supplied ordered-cell runner is a smoke check for this trusted notebook, not a Jupyter kernel or an execution sandbox.

A future interface for this Python exercise could accept scribbles and edge weights, then display propagation fields. The current core uses the Python standard library and does not require a service or hardware. No Space or web application is deployed for the Python exercise. The separate native implementation has a local drawing interface. Extending this toy cost model to real image segmentation requires additional work and evaluation.

## Attribution

The local formula derives from the related prototype, whose authorship remains unverified. Original files and hashes remain under `../source/`; the queue, boundary handling, renderer, tests and procedural composition are updated work. See [attribution and terms](../docs/ATTRIBUTION.md) for project and updated-code credits. The omitted generic heap is not redistributed, and no blanket license is assigned.
