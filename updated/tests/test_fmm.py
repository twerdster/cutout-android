"""Numerical and edge tests on analytic or explicitly synthetic inputs."""

from pathlib import Path
import math
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from fmm import eikonal_update, fast_marching
from scribble_demo import (image_costs, intersection_over_union, procedural_scene,
                           render_svg, segment)


class LocalUpdateTests(unittest.TestCase):
    def test_one_axis_and_causality_branch(self):
        self.assertEqual(eikonal_update(2, math.inf, 3), 5)
        self.assertEqual(eikonal_update(math.inf, 2, 3), 5)
        self.assertEqual(eikonal_update(0, 10, 1), 1)
        self.assertEqual(eikonal_update(math.inf, math.inf, 1), math.inf)

    def test_two_axes_have_analytic_quadratic_solution(self):
        self.assertAlmostEqual(eikonal_update(1, 1, 1), 1 + math.sqrt(0.5))
        value = eikonal_update(2, 2.4, 1.5)
        self.assertGreaterEqual(value, 2.4)
        self.assertAlmostEqual((value - 2) ** 2 + (value - 2.4) ** 2, 1.5 ** 2)

    def test_stencil_translation_and_axis_permutation(self):
        value = eikonal_update(1.2, 1.7, 2.3)
        self.assertEqual(value, eikonal_update(1.7, 1.2, 2.3))
        self.assertAlmostEqual(value + 100, eikonal_update(101.2, 101.7, 2.3))

    def test_invalid_local_inputs(self):
        for cost in [0, -1, math.nan, math.inf]:
            with self.assertRaises(ValueError):
                eikonal_update(1, 2, cost)
        for arrival in [math.nan, -math.inf]:
            with self.assertRaises(ValueError):
                eikonal_update(arrival, 2, 1)


class MarchingTests(unittest.TestCase):
    def assertGridsAlmostEqual(self, a, b):
        self.assertEqual(len(a), len(b))
        for ar, br in zip(a, b):
            self.assertEqual(len(ar), len(br))
            for av, bv in zip(ar, br):
                if math.isinf(av) or math.isinf(bv):
                    self.assertEqual(av, bv)
                else:
                    self.assertAlmostEqual(av, bv, places=11)

    def test_boundary_source_and_first_order_diagonal(self):
        times = fast_marching([[1] * 4 for _ in range(4)], [(0, 0)])
        self.assertEqual(times[0], [0, 1, 2, 3])
        self.assertEqual([row[0] for row in times], [0, 1, 2, 3])
        self.assertAlmostEqual(times[1][1], 1 + 1 / math.sqrt(2))
        # The recovered stencil is a first-order discretization, not exact
        # Euclidean distance: sqrt(2) is deliberately not the expected answer.
        self.assertGreater(times[1][1], math.sqrt(2))

    def test_single_cell_single_row_and_column(self):
        self.assertEqual(fast_marching([[2]], [(0, 0)]), [[0]])
        self.assertEqual(fast_marching([[2, 3, 4]], [(0, 0)]), [[0, 3, 7]])
        self.assertEqual(fast_marching([[2], [3], [4]], [(0, 0)]), [[0], [3], [7]])

    def test_uniform_grid_reflection_and_transpose_symmetry(self):
        times = fast_marching([[1] * 9 for _ in range(9)], [(4, 4)])
        self.assertGridsAlmostEqual(times, times[::-1])
        self.assertGridsAlmostEqual(times, [row[::-1] for row in times])
        self.assertGridsAlmostEqual(times, list(map(list, zip(*times))))

    def test_transpose_for_variable_costs_and_obstacles(self):
        costs = [[1 + (r + 2*c) / 10 for c in range(7)] for r in range(5)]
        costs[2][2] = math.inf
        costs[2][3] = math.inf
        direct = fast_marching(costs, [(0, 6), (4, 0)])
        transposed = fast_marching(list(map(list, zip(*costs))), [(6, 0), (0, 4)])
        self.assertGridsAlmostEqual(direct, list(map(list, zip(*transposed))))

    def test_cost_scaling_and_start_time_translation(self):
        costs = [[1, 2, 1], [1.5, 2, 0.5], [1, 1, 3]]
        baseline = fast_marching(costs, [(1, 1)])
        scaled = fast_marching([[3.5*v for v in row] for row in costs], [(1, 1)])
        translated = fast_marching(costs, [(1, 1)], start_time=7.2)
        self.assertGridsAlmostEqual(scaled, [[3.5*v for v in row] for row in baseline])
        self.assertGridsAlmostEqual(translated, [[v+7.2 for v in row] for row in baseline])

    def test_obstacle_wall_leaves_disconnected_region_unreachable(self):
        costs = [[1, 1, math.inf, 1, 1] for _ in range(5)]
        times = fast_marching(costs, [(2, 0)])
        self.assertTrue(all(math.isfinite(row[0]) and math.isfinite(row[1]) for row in times))
        self.assertTrue(all(all(math.isinf(v) for v in row[2:]) for row in times))

    def test_multi_source_order_duplicates_and_monotonicity(self):
        costs = [[1] * 8 for _ in range(6)]
        seeds = [(0, 0), (5, 7)]
        both = fast_marching(costs, seeds)
        self.assertEqual(both, fast_marching(costs, seeds[::-1] + seeds))
        self.assertEqual(both[0][0], 0)
        self.assertEqual(both[5][7], 0)
        for single in seeds:
            one = fast_marching(costs, [single])
            self.assertTrue(all(a <= b + 1e-12 for ar, br in zip(both, one) for a, b in zip(ar, br)))
        # A discrete multi-source stencil can combine neighbors originating
        # at different seeds; equality with min(single-source fields) is not assumed.

    def test_invalid_grids_seeds_and_time(self):
        for grid in [[], [[]], [[1], [1, 2]], [[0]], [[-1]], [[math.nan]], [[-math.inf]]]:
            with self.subTest(grid=grid), self.assertRaises(ValueError):
                fast_marching(grid, [(0, 0)])
        for seeds in [[], [(-1, 0)], [(0, 2)], [(0.5, 0)], [(True, 0)], [(0, 0, 0)]]:
            with self.subTest(seeds=seeds), self.assertRaises(ValueError):
                fast_marching([[1, 1]], seeds)
        with self.assertRaises(ValueError):
            fast_marching([[math.inf]], [(0, 0)])
        for value in [math.inf, -math.inf, math.nan]:
            with self.assertRaises(ValueError):
                fast_marching([[1]], [(0, 0)], start_time=value)


class SyntheticDemoTests(unittest.TestCase):
    def test_edge_rule_is_zero_on_constant_image(self):
        self.assertEqual(image_costs([[0.4] * 4 for _ in range(3)], 100), [[1] * 4 for _ in range(3)])
        self.assertEqual(image_costs([[0, 1]], 10), [[11, 11]])

    def test_procedural_segmentation_respects_scribbles_and_known_shape(self):
        scene = procedural_scene(48)
        result = segment(scene, 24)
        self.assertTrue(all(result.mask[r][c] for r, c in scene.foreground))
        self.assertTrue(all(not result.mask[r][c] for r, c in scene.background))
        self.assertGreater(intersection_over_union(result.mask, scene.truth), 0.9)
        # This checks a known procedural disk only, never original app accuracy.

    def test_demo_validation(self):
        for size in [0, 15, 257, 48.0, True]:
            with self.assertRaises(ValueError):
                procedural_scene(size)
        for image in [[], [[]], [[-0.1]], [[1.1]], [[math.nan]], [[0], [0, 1]]]:
            with self.assertRaises(ValueError):
                image_costs(image)
        for weight in [-1, math.inf, math.nan]:
            with self.assertRaises(ValueError):
                image_costs([[0]], weight)

    def test_svg_is_parseable_and_explicitly_synthetic(self):
        import xml.etree.ElementTree as ET
        scene = procedural_scene(16)
        svg = render_svg(scene, segment(scene))
        root = ET.fromstring(svg)
        self.assertTrue(root.tag.endswith('svg'))
        self.assertIn('procedural data', svg)
        self.assertIn('not an Android app result', svg)
        self.assertNotIn('<script', svg)
        self.assertNotIn('http://127.', svg)


if __name__ == '__main__':
    unittest.main()
