"""New 2026 educational composition of FMM fields, not recovered app behavior.

Every image and scribble here is procedural. The intensity-to-cost mapping,
two-label competition and SVG visualization were newly written by Codex for
Aaron Wetzler; the only recovered numerical relationship is the local stencil
documented in fmm.py. No model, photograph or original app result is included.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from html import escape
import math
from pathlib import Path

from fmm import Grid, NEIGHBORS, Point, fast_marching


@dataclass
class Scene:
    image: Grid
    truth: list[list[bool]]
    foreground: list[Point]
    background: list[Point]


@dataclass
class Segmentation:
    costs: Grid
    foreground_times: Grid
    background_times: Grid
    mask: list[list[bool]]


def procedural_scene(size: int = 48) -> Scene:
    """Synthetic textured disk and foreground/background scribbles."""
    if isinstance(size, bool) or not isinstance(size, int) or size < 16 or size > 256:
        raise ValueError("size must be an integer from 16 to 256")
    center, radius = (size - 1) / 2.0, size * 0.26
    truth = [[(r - center) ** 2 + (c - center) ** 2 <= radius ** 2
              for c in range(size)] for r in range(size)]
    image = [[(0.78 if truth[r][c] else 0.16)
              + 0.018 * math.sin(0.7 * r) * math.cos(0.6 * c)
              for c in range(size)] for r in range(size)]
    middle = size // 2
    foreground = [(middle, c) for c in range(middle - 2, middle + 3)]
    positions = sorted(set(range(0, size, max(1, size // 8))) | {size - 1})
    background = sorted({(0, p) for p in positions} | {(size - 1, p) for p in positions}
                        | {(p, 0) for p in positions} | {(p, size - 1) for p in positions})
    return Scene(image, truth, foreground, background)


def image_costs(image: Grid, edge_weight: float = 24.0) -> Grid:
    """NEW demo rule: one plus a weighted local intensity-edge magnitude."""
    edge_weight = float(edge_weight)
    if not math.isfinite(edge_weight) or edge_weight < 0:
        raise ValueError("edge_weight must be finite and nonnegative")
    if not image or not image[0] or any(len(row) != len(image[0]) for row in image):
        raise ValueError("image must be a nonempty rectangle")
    if any(not math.isfinite(v) or v < 0 or v > 1 for row in image for v in row):
        raise ValueError("image values must be finite grayscale values in [0, 1]")
    height, width = len(image), len(image[0])
    costs = [[1.0] * width for _ in range(height)]
    for r in range(height):
        for c in range(width):
            gradient = max((abs(image[r][c] - image[r + dr][c + dc])
                            for dr, dc in NEIGHBORS
                            if 0 <= r + dr < height and 0 <= c + dc < width), default=0.0)
            costs[r][c] += edge_weight * gradient
    return costs


def segment(scene: Scene, edge_weight: float = 24.0) -> Segmentation:
    """NEW two-label demo: each pixel takes the earlier arriving label."""
    if set(scene.foreground) & set(scene.background):
        raise ValueError("foreground and background scribbles must not overlap")
    costs = image_costs(scene.image, edge_weight)
    fg = fast_marching(costs, scene.foreground)
    bg = fast_marching(costs, scene.background)
    mask = [[math.isfinite(a) and a <= b for a, b in zip(ar, br)] for ar, br in zip(fg, bg)]
    return Segmentation(costs, fg, bg, mask)


def intersection_over_union(mask: list[list[bool]], truth: list[list[bool]]) -> float:
    if len(mask) != len(truth) or any(len(a) != len(b) for a, b in zip(mask, truth)):
        raise ValueError("masks must have equal dimensions")
    intersection = sum(a and b for ar, br in zip(mask, truth) for a, b in zip(ar, br))
    union = sum(a or b for ar, br in zip(mask, truth) for a, b in zip(ar, br))
    return intersection / union if union else 1.0


def _heat(value: float, maximum: float) -> str:
    if not math.isfinite(value):
        return '#aaaaaa'
    x = min(1.0, value / maximum) if maximum else 0.0
    return '#%02x%02x%02x' % (round(40 + 205 * x), round(55 + 120 * (1 - x)), round(230 * (1 - x)))


def render_svg(scene: Scene, result: Segmentation) -> str:
    """Four panels: synthetic scribbles, foreground/background arrivals, mask."""
    height, width = len(scene.image), len(scene.image[0])
    # Keep labels readable and notebook output width stable as grid size varies.
    panel_width, gap, top, bottom = 192, 20, 56, 35
    scale = panel_width / width
    total_width = 4 * panel_width + 5 * gap
    total_height = height * scale + top + bottom
    maximum = max(v for field in [result.foreground_times, result.background_times]
                  for row in field for v in row if math.isfinite(v))
    title = 'Updated 2026 educational FMM demo — procedural data, not an Android app result'
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{total_width}" height="{total_height}" viewBox="0 0 {total_width} {total_height}" shape-rendering="crispEdges">',
           f'<title>{escape(title)}</title>', '<rect width="100%" height="100%" fill="#ffffff"/>',
           f'<text x="20" y="19" font-family="sans-serif" font-size="13">{escape(title)}</text>']
    labels = ['Synthetic image + scribbles', 'Foreground arrival time', 'Background arrival time', 'New two-label composition']
    foreground, background = set(scene.foreground), set(scene.background)
    for panel, label in enumerate(labels):
        offset = gap + panel * (panel_width + gap)
        out.append(f'<text x="{offset}" y="40" font-family="sans-serif" font-size="11">{label}</text>')
        for r in range(height):
            for c in range(width):
                gray = round(255 * scene.image[r][c])
                if panel == 0:
                    color = f'#{gray:02x}{gray:02x}{gray:02x}'
                    if (r, c) in foreground:
                        color = '#ff355c'
                    elif (r, c) in background:
                        color = '#15c8ff'
                elif panel == 1:
                    color = _heat(result.foreground_times[r][c], maximum)
                elif panel == 2:
                    color = _heat(result.background_times[r][c], maximum)
                else:
                    color = '#f85c74' if result.mask[r][c] else '#203647'
                out.append(f'<rect x="{offset + scale*c}" y="{top + scale*r}" width="{scale}" height="{scale}" fill="{color}"/>')
    iou = intersection_over_union(result.mask, scene.truth)
    out.append(f'<text x="20" y="{total_height-13}" font-family="sans-serif" font-size="12">Pink: foreground. Cyan: background scribbles. Synthetic mask IoU: {iou:.3f}; this is not a research benchmark.</text></svg>')
    return '\n'.join(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--size', type=int, default=48)
    parser.add_argument('--edge-weight', type=float, default=24.0)
    parser.add_argument('--output', type=Path, default=Path('build/demo.svg'))
    args = parser.parse_args()
    scene = procedural_scene(args.size)
    result = segment(scene, args.edge_weight)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_svg(scene, result), encoding='utf-8')
    print(f'Synthetic demonstration: {args.size}x{args.size}, edge weight {args.edge_weight:g}')
    print(f'Synthetic disk mask IoU: {intersection_over_union(result.mask, scene.truth):.6f}')
    print(f'SVG: {args.output}')


if __name__ == '__main__':
    main()
