/*
 * openCVWithMW.cpp
 *
 *  Created on: Oct 30, 2014
 *      Author: elad
 */

#include "openCVwithMW.h"
#include <assert.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <vector>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/photo/photo.hpp>
#include <opencv2/highgui/highgui.hpp>  // Video write
#include <iostream>
#include <string>
#include <iomanip>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

#include <iostream>

using std::endl;
using std::cout;
using std::stringstream;

Fimage openCVWithMW::matToFimage(Mat& mat) {
	Fimage img = mw_new_fimage();
	mw_alloc_fimage(img, mat.rows, mat.cols);
	cv::Size size = mat.size();
	float p1;
	for (int i = 0; i < size.height; i++) {
		float* ptr = mat.ptr<float>(i);
		for (int j = 0; j < size.width; j++) {
			p1 = ptr[j];
			mw_plot_fimage(img, j, i, p1); //TODO: verify!
		}
	}
	return img;
	return NULL;
}

void openCVWithMW::PrintShapes(Shapes tree) {
	PrintShape(tree->the_shapes + 0, 0, tree->the_shapes);
}

void openCVWithMW::PrintShape(Shape node, int level, Shape theRoot) {
	if (node == NULL) {
		return;
	}
	int n = node - theRoot;
	assert(theRoot + n == node);
	printf("%d: %f\n", n, node->value);

	PrintShape(node->next_sibling, level, theRoot);
	PrintShape(node->child, level + 1, theRoot);
}

Mat openCVWithMW::DijkstraOnTree(Shapes tree, std::vector<Point> path, int rows, int cols) {
	Mat distances = Mat(rows, cols, CV_32FC1, Scalar(0.f));
	//We will build the graph for the dijkstra run based on the tree
	adjacency_list_t adjacency_list(tree->nb_shapes);
	Shape root = tree->the_shapes;
	RecursiveCreateTree(root->child, root, adjacency_list);
	std::vector<vertex_t> sources;
	for (std::vector<Point>::iterator vectorIt = path.begin();
			vectorIt != path.end(); ++vectorIt) {
		//Define the sources
		Point p = *vectorIt;
		Shape shape = mw_get_smallest_shape(tree, p.x, p.y);
		vertex_t vertex = shape - tree->the_shapes;
		sources.push_back(vertex);
	}
	std::vector<weight_t> min_distance;
	std::vector<vertex_t> previous;
	DijkstraComputePaths(sources, adjacency_list, min_distance, previous);
	cv::Size size = distances.size();
	float p1;
	for (int i = 0; i < size.height; i++) {
		float* ptr = distances.ptr<float>(i);
		for (int j = 0; j < size.width; j++) {
			Shape shape = mw_get_smallest_shape(tree, j, i);
			int index = shape - tree->the_shapes;
			p1 = min_distance[index];
			ptr[j] = p1;

		}
	}
	return distances;
}

void openCVWithMW::RecursiveCreateTree(Shape node, Shape theRoot,
		adjacency_list_t& adjacency_list) {
	//Add edge to father
	if (node == NULL) {
		return;
	}
	int nodeN = node - theRoot;
	int parentN = mw_get_parent_shape(node) - theRoot;
	float edgeWeight = fabsf(node->value - mw_get_parent_shape(node)->value);
	adjacency_list[nodeN].push_back(neighbor(parentN, edgeWeight));
	adjacency_list[parentN].push_back(neighbor(nodeN, edgeWeight));
	RecursiveCreateTree(mw_get_next_sibling_shape(node), theRoot,
			adjacency_list);
	RecursiveCreateTree(mw_get_first_child_shape(node), theRoot,
			adjacency_list);
	return;
}


void openCVWithMW::DijkstraComputePaths(std::vector<vertex_t> sources,
		const adjacency_list_t &adjacency_list,
		std::vector<weight_t> &min_distance, std::vector<vertex_t> &previous) {
	int n = adjacency_list.size();
	min_distance.clear();
	const weight_t max_weight = std::numeric_limits<float>::infinity();
	min_distance.resize(n, max_weight);
	previous.clear();
	previous.resize(n, -1);
	std::set<std::pair<weight_t, vertex_t> > vertex_queue;
	for (std::vector<vertex_t>::iterator vectorIt = sources.begin();
			vectorIt != sources.end(); ++vectorIt) {
		min_distance[*vectorIt] = 0;
		vertex_queue.insert(std::make_pair(min_distance[*vectorIt], *vectorIt));
	}
	while (!vertex_queue.empty()) {
		weight_t dist = vertex_queue.begin()->first;
		vertex_t u = vertex_queue.begin()->second;
		vertex_queue.erase(vertex_queue.begin());

		// Visit each edge exiting u
		const std::vector<neighbor> &neighbors = adjacency_list[u];
		for (std::vector<neighbor>::const_iterator neighbor_iter =
				neighbors.begin(); neighbor_iter != neighbors.end();
				neighbor_iter++) {
			vertex_t v = neighbor_iter->target;
			weight_t weight = neighbor_iter->weight;
			weight_t distance_through_u = dist + weight;
			if (distance_through_u < min_distance[v]) {
				vertex_queue.erase(std::make_pair(min_distance[v], v));

				min_distance[v] = distance_through_u;
				previous[v] = u;
				vertex_queue.insert(std::make_pair(min_distance[v], v));

			}

		}
	}
}

