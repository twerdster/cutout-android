"""Updated 2026 Fast Marching component, prepared for Aaron Wetzler by Codex.

The local Eikonal update follows the recovered ``fmmMex.cpp`` prototype,
whose original authorship is unverified. The heapq queue, explicit accepted
state, boundary checks, infinity obstacles and multiple-source API are new.
This is not the historical CutOut Android application. No license is assigned.
"""

from __future__ import annotations

import heapq
import math
from collections.abc import Iterable, Sequence

Grid = list[list[float]]
Point = tuple[int, int]  # zero-based (row, column)
NEIGHBORS = ((-1, 0), (1, 0), (0, -1), (0, 1))


def eikonal_update(vertical: float, horizontal: float, cost: float) -> float:
    """Solve the recovered first-order, unit-grid Eikonal stencil.

    Inputs are the best available arrival in each axis and local positive
    *cost/slowness*, not speed. ``inf`` neighbors denote an unavailable axis.
    The expression is algebraically equivalent to the original quadratic,
    with normalized arithmetic to avoid unnecessarily squaring large costs.
    """
    if math.isnan(vertical) or math.isnan(horizontal):
        raise ValueError("neighbor arrivals cannot be NaN")
    if vertical == -math.inf or horizontal == -math.inf:
        raise ValueError("neighbor arrivals cannot be negative infinity")
    if not math.isfinite(cost) or cost <= 0:
        raise ValueError("the local update needs a finite positive cost")
    a, b = sorted((vertical, horizontal))
    if a == math.inf:
        return math.inf
    delta = b - a
    if delta >= cost:
        return a + cost
    return a + (delta + cost * math.sqrt(2.0 - (delta / cost) ** 2)) / 2.0


def checked_grid(values: Sequence[Sequence[float]], *, allow_infinity: bool = True) -> Grid:
    """Copy and validate a nonempty rectangular positive-cost grid."""
    if not values or not values[0]:
        raise ValueError("grid must have at least one row and column")
    width = len(values[0])
    result = []
    for row in values:
        if len(row) != width:
            raise ValueError("grid must be rectangular")
        copied = [float(v) for v in row]
        if any(math.isnan(v) or v <= 0 or (not allow_infinity and not math.isfinite(v))
               for v in copied):
            raise ValueError("costs must be positive; only +infinity can denote an obstacle")
        result.append(copied)
    return result


def fast_marching(costs: Sequence[Sequence[float]], seeds: Iterable[Point],
                  *, start_time: float = 0.0) -> Grid:
    """Return first-order arrival times on a four-neighbor unit grid.

    ``seeds`` are zero-based ``(row, column)`` pairs sharing ``start_time``.
    Positive finite costs are traversable; +infinity is an impassable cell.
    Unreachable cells retain +infinity. Border seeds are supported explicitly.

    The original prototype read all tentative neighbor times and relied on a
    blocked outer border. This implementation uses accepted neighbors only
    and rejects invalid inputs, so whole-grid bit identity is not claimed.
    """
    grid = checked_grid(costs)
    height, width = len(grid), len(grid[0])
    start_time = float(start_time)
    if not math.isfinite(start_time):
        raise ValueError("start_time must be finite")
    sources: set[Point] = set()
    for source in seeds:
        if len(source) != 2:
            raise ValueError("each seed must be a (row, column) pair")
        row, col = source
        if (isinstance(row, bool) or isinstance(col, bool)
                or not isinstance(row, int) or not isinstance(col, int)):
            raise ValueError("seed coordinates must be integers")
        if not (0 <= row < height and 0 <= col < width):
            raise ValueError("seed outside grid")
        if not math.isfinite(grid[row][col]):
            raise ValueError("a seed cannot be placed on an obstacle")
        sources.add((row, col))
    if not sources:
        raise ValueError("at least one seed is required")

    times = [[math.inf] * width for _ in range(height)]
    accepted = [[False] * width for _ in range(height)]
    frontier: list[tuple[float, int, int]] = []
    for row, col in sorted(sources):
        times[row][col] = start_time
        heapq.heappush(frontier, (start_time, row, col))

    def accepted_time(row: int, col: int) -> float:
        if 0 <= row < height and 0 <= col < width and accepted[row][col]:
            return times[row][col]
        return math.inf

    while frontier:
        value, row, col = heapq.heappop(frontier)
        if accepted[row][col] or value != times[row][col]:
            continue  # lazy replacement for the missing decrease-key heap
        accepted[row][col] = True
        for dr, dc in NEIGHBORS:
            r, c = row + dr, col + dc
            if not (0 <= r < height and 0 <= c < width):
                continue
            if accepted[r][c] or not math.isfinite(grid[r][c]):
                continue
            vertical = min(accepted_time(r - 1, c), accepted_time(r + 1, c))
            horizontal = min(accepted_time(r, c - 1), accepted_time(r, c + 1))
            candidate = eikonal_update(vertical, horizontal, grid[r][c])
            if candidate < times[r][c]:
                times[r][c] = candidate
                heapq.heappush(frontier, (candidate, r, c))
    return times
