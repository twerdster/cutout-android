/*
 * Dijkstra.cpp
 *
 *  Created on: May 20, 2014
 *      Author: elad
 */

#include "Dijkstra.h"
#include "PlatformDependent.h"
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/core/mat.hpp>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

#include <iostream>

using std::endl;
using std::cout;
using std::stringstream;

float Dijkstra::getWeight(Mat& weights, Point& p, Point& q) {
	float result = fabs(weights.at<float>(p) - weights.at<float>(q));
	return result;
}

std::vector<Point> Dijkstra::simpleGetNeighbors(Point& p, int maxY, int maxX,
		Mat& mask) {
	std::vector<Point> neighbors;
	if (p.x > 0) {
		Point q = Point(p.x - 1, p.y);
		if (mask.at<uchar>(q) == 255) {
			neighbors.push_back(q);
		} else {
			int i = 0;
		}
	}
	if (p.x < maxX - 1) {
		Point q = Point(p.x + 1, p.y);
		if (mask.at<uchar>(q) == 255) {
			neighbors.push_back(q);
		}
	}
	if (p.y > 0) {
		Point q = Point(p.x, p.y - 1);
		if (mask.at<uchar>(q) == 255) {
			neighbors.push_back(q);
		}
	}
	if (p.y < maxY - 1) {
		Point q = Point(p.x, p.y + 1);
		if (mask.at<uchar>(q) == 255) {
			neighbors.push_back(q);
		}
	}
	return neighbors;
}

Mat Dijkstra::simpleComputePaths(Mat& weights, std::vector<Point> sources, Mat& mask) {
	/* Some initializations */
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running simpleComputePaths");
	showString(sstream);
	weight_t infty = std::numeric_limits<float>::infinity(); //C++ style ;)
	Mat distances = Mat(weights.rows, weights.cols, CV_32F, Scalar(infty));
	std::set<std::pair<weight_t, Point>, comparePoints> points_queue;
	//Initialize sources
	for (std::vector<Point>::iterator it = sources.begin(); it != sources.end();
			++it) {
		Point p = *it;
		distances.at<float>(p) = 0.f;
		points_queue.insert(std::make_pair(0.f, p));
	}
	float lastDist = 0;
	/* Dijkstra step */
	while (!points_queue.empty()) {
		float dist_to_p = points_queue.begin()->first;
		Point p = points_queue.begin()->second;
		std::vector<Point> neighbors = simpleGetNeighbors(p, weights.rows,
				weights.cols, mask);
		points_queue.erase(points_queue.begin());
		for (std::vector<Point>::iterator it = neighbors.begin();
				it != neighbors.end(); ++it) {
			Point q = *it;
			float dist_to_q = distances.at<float>(q);
			float dist_through_p = dist_to_p + getWeight(weights, p, q) + 1e-5;
			if (dist_to_q > dist_through_p) {
				points_queue.erase(std::make_pair(dist_to_q, q));
				points_queue.insert(std::make_pair(dist_through_p, q));
				distances.at<float>(q) = dist_through_p;
				lastDist = dist_through_p;
			}
		}
	}
	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "simpleComputePaths finished in " << time;
	showString(sstream);
	sstream << "The max Dist was " << lastDist;
	showString(sstream);
	//Efficiency: Since we random access the matrix here I think we ought to use .at
	return distances;
}

std::vector<Point> Dijkstra::fillMultiplePaths(std::vector<std::vector<Point> > toFillScribbles) {
	std::vector<Point> filled;
	std::vector<std::vector<Point> >::iterator iterator = toFillScribbles.begin();
	for (; iterator != toFillScribbles.end(); iterator++) {
		std::vector<Point> tmp = fillPath(*iterator);
		filled.insert(filled.end(), tmp.begin(), tmp.end());
	}
	return filled;
}

std::vector<Point> Dijkstra::fillPath(std::vector<Point> toFill) {
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running fillPath");
	showString(sstream);
	std::vector<Point> filled;
	std::vector<Point>::iterator iterator = toFill.begin();
	Point p = *iterator;
	iterator++;
	filled.push_back(p);
	for (; iterator != toFill.end(); iterator++) {
		Point q = *iterator;
		std::vector<Point> path = simpleCompleteLine(p, q);
		//cout << "Path is = " << Mat(path) << endl << endl;
		filled.insert(filled.end(), ++path.begin(), path.end()); //We don't take the first point to avoid duplication
		p = q;
	}
	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "fillPath finished in " << time;
	showString(sstream);
	return filled;
}

std::vector<Point> Dijkstra::simpleCompleteLine(Point p, Point q) {
	std::vector<Point> path;
	//		path.push_back(p);
	int dx = abs(q.x - p.x);
	int dy = abs(q.y - p.y);
	int sx = p.x < q.x ? 1 : -1;
	int sy = p.y < q.y ? 1 : -1;
	int err = dx - dy;
	while (p.x != q.x || p.y != q.y) {
		path.push_back(p);
		int e2 = 2 * err;
		if (e2 > -dy) {
			err = err - dy;
			p.x = p.x + sx;
		}
		if (e2 < dx) {
			err = err + dx;
			p.y = p.y + sy;
		}
	}
	path.push_back(q);
	return path;
}

