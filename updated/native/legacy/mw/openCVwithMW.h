// Updated 2026 for CutOut restoration; original authors/notices retained below.
/*
 * openCVwithMW.h
 *
 *  Created on: Oct 30, 2014
 *      Author: elad
 */

#ifndef OPENCVWITHMW_H_
#define OPENCVWITHMW_H_

#include <opencv2/core/core.hpp>
#include "mw.h"

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <set>
#include <utility>
#include <vector>

#include <limits> // for numeric_limits

#include <set>
#include <utility> // for pair
#include <algorithm>
#include <iterator>

using namespace std;
void flst(int* pMinArea, Fimage pImageInput, Shapes pTree);

using namespace cv;
class openCVWithMW {
public:

	typedef int vertex_t;
	typedef float weight_t;



	struct neighbor {
	    vertex_t target;
	    weight_t weight;
	    neighbor(vertex_t arg_target, weight_t arg_weight)
	        : target(arg_target), weight(arg_weight) { }
	    friend ostream& operator<<(ostream& os, const openCVWithMW::neighbor& n);
	};

	typedef std::vector<std::vector<neighbor> > adjacency_list_t;
	static Fimage matToFimage(Mat& mat);
	static void PrintShapes(Shapes tree);
	static void PrintShape(Shape node, int level, Shape root);
	static Mat DijkstraOnTree(Shapes tree, std::vector<Point> path, int rows, int cols);
	static void RecursiveCreateTree(Shape node, Shape theRoot, adjacency_list_t& adjacency_list);
	static void updatePixelsInShape(Mat& mat, Shape shape, float val);
	static void DijkstraComputePaths(std::vector<vertex_t> source,
    const adjacency_list_t &adjacency_list,
    std::vector<weight_t> &min_distance,
    std::vector<vertex_t> &previous);
};

#endif /* OPENCVWITHMW_H_ */
