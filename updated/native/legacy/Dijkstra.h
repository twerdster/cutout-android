// Updated 2026 for CutOut restoration; original authors/notices retained below.
/*
 * Dijkstra.h
 *
 *  Created on: May 20, 2014
 *      Author: Elad Richardson
 */

#ifndef DIJKSTRA_H_
#define DIJKSTRA_H_

#include <opencv2/core/core.hpp>
//#include <opencv2/core/types_c.h>
//
//
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/imgproc/types_c.h>
//#include <limits>
#include <set>
#include <utility>
#include <vector>

using namespace cv;
typedef float weight_t;

/**
 * Commenting your code is like cleaning your bathroom.
 * you never want to do it, but it really does create a more pleasant experience for you and your guests.
 */

class Dijkstra {

	/*static  std::vector<Point> getNeighbors(Point& p, Mat& visited, int maxY,
	 int maxX);*/

	/**
	 * Gets the neighbors of a r of matrix cell
	 *
	 * @param p - a point from the matrix
	 * @param maxY - the Y limit of the matrix
	 * @param maxX - the X limit of the matrix
	 * @return - the cell's neighbors as a vector of points
	 */
	static std::vector<Point> simpleGetNeighbors(Point& p, int maxY, int maxX, Mat& mask);

	/**
	 * Returns the weight according to the difference between the two points
	 * @param weights - The weights matrix
	 * @param p,q - the points
	 * @return - the weight as float
	 */
	static float getWeight(Mat& weights, Point& p, Point& q);

	/**
	 * comparePoints - a comparison object between two points
	 * compares first according to the x value and then according to the y value
	 */
	class comparePoints {
		bool pointIsLess(Point p, Point q) const {
			return p.x < q.x || (!(q.x < p.x) && p.y < q.y);
		}
		bool isLess(std::pair<weight_t, Point> p,
				std::pair<weight_t, Point> q) const {
			return p.first < q.first
					|| (!(q.first < p.first) && pointIsLess(p.second, q.second));
		}
	public:
		bool operator()(std::pair<weight_t, Point> p,
				std::pair<weight_t, Point> q) const {
			return isLess(p, q);
		}
	};

public:

	//static Mat computePaths(Mat& weights, std::vector<Point> sources);
	/**
	 * simpleComputePaths - Compute distances from the sources to each pixel according to their weights
	 * @param weights - the original matrix, used for weights
	 * @param sources - the sources, i.e scribble
	 * @return - The matrix representing the distances
	 */
	static Mat simpleComputePaths(Mat& weights, std::vector<Point> sources, Mat& mask);
	//static std::vector<Point> completeLine(Point p, Point q);
	/**
	 * simpleCompleteLine - completes a path between two points, so that it's continuous.
	 * @param p - the source point
	 * @param q - the destination point
	 * @return - the full path as a vector
	 */
	static std::vector<Point> simpleCompleteLine(Point p, Point q);
	/**
	 * fillPath - completes an existing path so that it's continuous
	 * @param toFill - the existing path
	 * @return - the filled path
	 */
	static std::vector<Point> fillPath(std::vector<Point> toFill);

	static std::vector<Point> fillMultiplePaths(std::vector<std::vector<Point> > toFillScribbles);



};

#endif /* DIJKSTRA_H_ */
