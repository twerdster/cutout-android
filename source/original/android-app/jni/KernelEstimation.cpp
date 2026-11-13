/*
 * KernelEstimation.cpp
 *
 *  Created on: Aug 11, 2014
 *      Author: elad
 */

#include "KernelEstimation.h"
#include "figtree.h"
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <android/log.h>
#include "PlatformDependent.h"
#include "Dijkstra.h"
#include "mw/openCVwithMW.h"
using std::endl;
using std::cout;
using std::stringstream;

Point3d KernelEstimation::labToRange(Point3f& p, float range) {
	int x, y, z;
	range = range - 1;
	/*Bound to range*/
	boundToEdges<float>(p.x, Lmin, Lmax);
	boundToEdges<float>(p.y, amin, amax);
	boundToEdges<float>(p.z, bmin, bmax);
	x = (range / (Lmax - Lmin)) * (p.x - Lmin);
	y = (range / (amax - amin)) * (p.y - amin);
	z = (range / (bmax - bmin)) * (p.z - bmin);
	return Point3d(x, y, z);
}

Mat KernelEstimation::simpleCalculatePDF(Mat& image, std::vector<Point> sources,
		Mat& mask, int range, int blurMode) {
	stringstream sstream;
	double time = (double) getTickCount();
	sstream.str("Running simpleCalculatePDF");
	showString(sstream);

	//int range = 10;
	int N = sources.size();

	/* Convert to Lab space*/

	Mat labImage;
	cvtColor(image, labImage, CV_BGR2Lab);
	double localTime = (double) getTickCount();
	sstream.str("\tCreating 3D Histogram");
	showString(sstream);

	/* Create the sources matrix */

	int sizes[] = { range, range, range };
	Mat hist = Mat(D, sizes, CV_32FC1, cv::Scalar(0));
	int radius = blurMode == 1 ? 5 : range;
	radius = blurMode > 2 ? blurMode : radius;
	radius += 1 - radius % 2;
	Mat gaussian3D = get3DGaussianFilter(radius);
	//Mat gaussian3D = Mat(D, sizes, CV_32FC1, Scalar(1.0/(D*D*D)));

	for (std::vector<Point>::iterator vectorIt = sources.begin();
			vectorIt != sources.end(); ++vectorIt) {
		float p1, p2, p3;
		p1 = labImage.at<cv::Vec3f>(*vectorIt)[0];
		p2 = labImage.at<cv::Vec3f>(*vectorIt)[1];
		p3 = labImage.at<cv::Vec3f>(*vectorIt)[2];
		//TODO: get the point immediately
		Point3f p = Point3f(p1, p2, p3);
		float sum;
		if (blurMode == 0) {

			//Range needs to be odd

			for (int i = 0; i < radius; i++) {
				for (int j = 0; j < radius; j++) {
					for (int k = 0; k < radius; k++) {
						Point3f neighbor = Point3f(p1 + (i - radius / 2),
								p2 + (j - radius / 2), p3 + (k - radius / 3));
						Point3d bucket = labToRange(neighbor, range);
						float hVal = hist.at<float>(bucket.x, bucket.y,
								bucket.z);
						float addVal = gaussian3D.at<float>(i, j, k);
						if (addVal < 0) {
							addVal = 0;
						}
						sum += addVal;
						hist.at<float>(bucket.x, bucket.y, bucket.z) = hVal
								+ addVal;
					}
				}
			}

		} else {
			Point3d q = labToRange(p, range);
			float hVal = hist.at<float>(q.x, q.y, q.z);
			hist.at<float>(q.x, q.y, q.z) = hVal + 1;
		}
		sum = 0;

	}

	localTime = ((double) getTickCount() - localTime) / getTickFrequency();
	sstream << "\tFinished in " << localTime;
	showString(sstream);

//Updated Histogram
//Blur Histogram

	if (blurMode == 1) {
		Mat blurredHist = Mat(D, sizes, CV_32FC1, cv::Scalar(0));
		//For each column
		for (int i = 0; i < range; i++) {
			for (int j = 0; j < range; j++) {
				for (int k = 0; k < range; k++) {
					float hVal = hist.at<float>(i, j, k);
					if (hVal == 0) {
						continue;
					}
					//The column val
					for (int ig = 0; ig < radius; ig++) {
						for (int jg = 0; jg < radius; jg++) {
							for (int kg = 0; kg < radius; kg++) {
								int x = boundToEdges(i + (ig - radius / 2), 0,
										range - 1);
								int y = boundToEdges(j + (jg - radius / 2), 0,
										range - 1);
								int z = boundToEdges(k + (kg - radius / 2), 0,
										range - 1);
								float toAdd = hVal
										* gaussian3D.at<float>(ig, jg, kg);
								blurredHist.at<float>(x, y, z) += toAdd;
							}
						}

					}
				}
			}
		}
		hist = blurredHist;
	}

//	GaussianBlur(hist, hist, Size(5,5), -1, -1);
	localTime = (double) getTickCount();
	sstream.str("\tTransfering Histogram");
	showString(sstream);
	/** Transfer to openCV matrix*/
	Mat result = Mat(labImage.rows, labImage.cols, CV_32FC1);
	float val;

	Size size = result.size();
	if (result.isContinuous() && mask.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	for (int i = 0; i < size.height; i++) {
		float* ptr = result.ptr<float>(i);
		uchar* maskPtr = mask.ptr<uchar>(i);
		for (int j = 0; j < size.width; j++) {
			if (maskPtr[j] == 0) { //J is a multiple of D
				ptr[j] = 0;
				continue;
			}
			float p1, p2, p3;
			p1 = labImage.at<cv::Vec3f>(i, j)[0];
			p2 = labImage.at<cv::Vec3f>(i, j)[1];
			p3 = labImage.at<cv::Vec3f>(i, j)[2];
			//TODO: get the point immediately
			Point3f p = Point3f(p1, p2, p3);
			Point3d q = labToRange(p, range);
			val = hist.at<float>(q.x, q.y, q.z);
			(ptr[j]) = val;
		}
	}
	if (blurMode > 2) {
		GaussianBlur(result, result, Size(radius, radius), 0, 0);
	}
//Efficiency: Using pointers to iterate over the Matrix.

	localTime = ((double) getTickCount() - localTime) / getTickFrequency();
	sstream << "\tFinished in " << localTime;
	showString(sstream);

	localTime = (double) getTickCount();
	sstream.str("\tNormalizing Result");
	showString(sstream);

	/** Normalize to get the PDF */
	double min;
	double max;
	cv::minMaxIdx(result, &min, &max);
	cv::Mat pdf;
	Mat PDF;
// Trying to normalize
	sstream << "\t\tMax is: " << max << ", Min is: " << min;
	showString(sstream);

	result.convertTo(pdf, CV_32FC1, 1.0 / N, 0);
	localTime = ((double) getTickCount() - localTime) / getTickFrequency();
	sstream << "\tFinished in " << localTime;
	showString(sstream);

	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "SimpleCalculatePDF finished in " << time;
	showString(sstream);
	hist.release();
	gaussian3D.release();
	return pdf;

}

Mat KernelEstimation::calculatePDF(Mat & image, std::vector<Point> sources,
		Mat & mask) {
	/* Convert to Lab space*/
	stringstream sstream;
	double time = (double) getTickCount();
	sstream.str("Running calculatePDF");
	showString(sstream);

	Mat labImage;
	cvtColor(image, labImage, CV_BGR2Lab);
	double localTime = (double) getTickCount();
	sstream.str("\tCreating X Matrix");
	showString(sstream);

	/* Create the sources matrix */
	int N = sources.size();
	double* x = new double[N * D];
	std::vector<Point>::iterator vectorIt = sources.begin();
	std::vector<Point>::iterator vectorEnd = sources.end();

	for (int i = 0; i < N; i++) {
		if (vectorIt == vectorEnd) {
			//Shouldn't happen
			sstream.str("\tOops out of range");
			showString(sstream);
			break;
		}
		//Add the pixel to the matrix
		double p1, p2, p3;
		x[i * D] = p1 = labImage.at<cv::Vec3f>(*vectorIt)[0];
		x[i * D + 1] = p2 = labImage.at<cv::Vec3f>(*vectorIt)[1];
		x[i * D + 2] = p3 = labImage.at<cv::Vec3f>(*vectorIt)[2];
		//efficiency: We access the matrix randomly, so .at is OK.
		vectorIt++;
	}

	localTime = ((double) getTickCount() - localTime) / getTickFrequency();

	sstream << "\tFinished in " << localTime;
	showString(sstream);

	/**Create the weights matrix*/
	double* q = new double[N];
	for (int i = 0; i < N; i++) {
		q[i] = 1;
	}
//memset(q, 1, sizeof(double) * N);

	localTime = (double) getTickCount();
	sstream.str("\tCreating Y Matrix");
	showString(sstream);
	/** Create the target matrix*/
//int M = labImage.cols * labImage.rows;
	int M = cv::countNonZero(mask);
	double* y = new double[M * D];
	int index = 0;
	cv::Size size = labImage.size();
	if (labImage.isContinuous() && mask.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	float p1, p2, p3;
	size.width *= D; //
	for (int i = 0; i < size.height; i++) {
		float* ptr = labImage.ptr<float>(i);
		uchar* maskPtr = mask.ptr<uchar>(i);
		for (int j = 0; j < size.width; j += D) {
			if (maskPtr[j / D] == 0) { //J is a multiple of D
				continue;
			}
			y[index++] = p1 = (ptr[j]);
			y[index++] = p2 = (ptr[j + 1]);
			y[index++] = p3 = (ptr[j + 2]);
		}
	}
//Efficiency: Using pointers for accessing.

	localTime = ((double) getTickCount() - localTime) / getTickFrequency();

	sstream << "\tFinished in " << localTime;
	showString(sstream);

	localTime = (double) getTickCount();
	sstream.str("\tRunning FIGTree");
	showString(sstream);
	/**Create the result matrix */
	double* g = new double[M];
	figtree(D, N, M, 1, x, 10, q, y, 1e-1, g);

	localTime = ((double) getTickCount() - localTime) / getTickFrequency();

	sstream << "\tFinished in " << localTime;
	showString(sstream);

	localTime = (double) getTickCount();
	sstream.str("\tTransfering FIGTree");
	showString(sstream);

	/** Transfer to openCV matrix*/
	Mat result = Mat(labImage.rows, labImage.cols, CV_32FC1);
	float sum = 0;
	float val;
	index = 0;
	size = result.size();
	if (result.isContinuous() && mask.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	for (int i = 0; i < size.height; i++) {
		float* ptr = result.ptr<float>(i);
		uchar* maskPtr = mask.ptr<uchar>(i);
		for (int j = 0; j < size.width; j++) {
			if (maskPtr[j] == 0) { //J is a multiple of D
				ptr[j] = 0;
				continue;
			}
			val = g[index++];
			sum += val;
			(ptr[j]) = val;
		}
	}
//Efficiency: Using pointers to iterate over the Matrix.

	localTime = ((double) getTickCount() - localTime) / getTickFrequency();
	sstream << "\tFinished in " << localTime;
	showString(sstream);

	localTime = (double) getTickCount();
	sstream.str("\tNormalizing Result");
	showString(sstream);

	/** Normalize to get the PDF */
	double min;
	double max;
	cv::minMaxIdx(result, &min, &max);
	cv::Mat pdf;
	Mat PDF;
// Trying to normalize
	sstream << "\t\tMax is: " << max << ", Min is: " << min << ", Sum is: "
			<< sum;
	showString(sstream);

	result.convertTo(pdf, CV_32FC1, 1.0 / N, 0);
	localTime = ((double) getTickCount() - localTime) / getTickFrequency();
	sstream << "\tFinished in " << localTime;
	showString(sstream);

	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "calculatePDF finished in " << time;
	showString(sstream);

	delete[] x;
	delete[] y;
	delete[] q;
	delete[] g;
	return pdf;
}

void KernelEstimation::calculateLikelihood(Mat& frontPDF, Mat& backPDF,
		Mat& frontLike, Mat& backLike, Mat& mask) {
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running calculateLikelihood");
	showString(sstream);
	for (int x = 0; x < frontPDF.rows; x++) {
		for (int y = 0; y < frontPDF.cols; y++) {
			if (mask.at<uchar>(x, y) == 0) {
				continue;
			}
			float f = frontPDF.at<float>(x, y);
			float b = backPDF.at<float>(x, y);
			float fVal = 0.f, bVal = 0.f;
			if (f != 0.f || b != 0.f) {
				fVal = f / (f + b);
				bVal = b / (f + b);
			}
			frontLike.at<float>(x, y) = fVal;
			backLike.at<float>(x, y) = bVal;
		}
	}
	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "calculateLikelihood finished in " << time;
	showString(sstream);
}

Mat KernelEstimation::findObject(Mat & frontDistances, Mat & backDistances,
		Mat & mask) {
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running findObject");
	showString(sstream);
	Mat resMask = Mat(frontDistances.rows, frontDistances.cols, CV_8UC1,
			Scalar(0));
	cv::Size size = resMask.size();
	if (resMask.isContinuous() && mask.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	for (int i = 0; i < size.height; i++) {
		uchar* resMaskPtr = resMask.ptr<uchar>(i);
		uchar* maskPtr = mask.ptr<uchar>(i);
		float* fptr = frontDistances.ptr<float>(i);
		float* bptr = backDistances.ptr<float>(i);

		for (int j = 0; j < size.width; j++) {
			if (maskPtr[j] == 0) {
				continue;
			}
			if (fptr[j] < bptr[j]) {
				resMaskPtr[j] = 255;
			}
		}
	}
	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "findObject finished in " << time;
	showString(sstream);
	return resMask;
}

void KernelEstimation::updateAlpha(Mat& frontDistances, Mat& backDistances,
		Mat& frontPDF, Mat& backPDF, Mat& bandMask, Mat& alpha,
		double intensity) {
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running findAlpha");
	showString(sstream);
	cv::Size size = alpha.size();
	if (alpha.isContinuous() && bandMask.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	for (int i = 0; i < size.height; i++) {
		float* alphaPtr = alpha.ptr<float>(i);
		uchar* maskPtr = bandMask.ptr<uchar>(i);
		float* fDistptr = frontDistances.ptr<float>(i);
		float* bDistptr = backDistances.ptr<float>(i);
		float* fPDFptr = frontPDF.ptr<float>(i);
		float* bPDFptr = backPDF.ptr<float>(i);
		for (int j = 0; j < size.width; j++) {
			if (maskPtr[j] == 0 || alphaPtr[j] == 1.0) {
				continue;
			}
			float fDist = fDistptr[j];
			float bDist = bDistptr[j];
			float fPDF = fPDFptr[j];
			float bPDF = bPDFptr[j];
			float fWeight, bWeight, alpha;
			fWeight = bWeight = std::numeric_limits<float>::infinity();
			alpha = 0;
			if (fDist > 0.f) {
				fWeight = fPDF / std::pow(fDist, intensity);
			}
			if (bDist > 0.f) {
				bWeight = bPDF / std::pow(bDist, intensity);
			}
			if (fWeight != 0.f || bWeight != 0.f) {
				alpha = fWeight / (fWeight + bWeight);
			}
			alphaPtr[j] = alpha;
		}
	}
	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "updateAlpha finished in " << time;
	showString(sstream);
}

Mat KernelEstimation::addAlpha(Mat & image, Mat & alpha) {
	Mat imageWithAlpha;
	cvtColor(image, imageWithAlpha, CV_BGR2BGRA);
	for (int i = 0; i < imageWithAlpha.rows; i++) {
		for (int j = 0; j < imageWithAlpha.cols; j++) {
			Vec4f& v = imageWithAlpha.at<Vec4f>(i, j);
			v[3] = alpha.at<float>(i, j);
		}
	}
	/*	cv::Size size = alpha.size();
	 if (alpha.isContinuous() && image.isContinuous()) {
	 size.width *= size.height;
	 size.height = 1;
	 }
	 size.width *= 4;
	 for (int i = 0; i < size.height; i++) {
	 float* alphaPtr = alpha.ptr<float>(i);
	 float* imagePtr = imageWithAlpha.ptr<float>(i);
	 for (int j = 0; j < size.width; j += 4) {
	 //imagePtr[j] = 1.0;//alphaPtr[j/4];
	 }
	 }*/
	return imageWithAlpha;
}

void KernelEstimation::updateObjectMask(Mat& image, Mat& objectMask,
		vector<vector<Point> > frontscribbles,
		vector<vector<Point> > backscribbles, Mat& frontPDF, Mat& backPDF,
		Mode mode, bool simple, int range, int blurMode, bool useLevelSet) {
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running updateObjectMask");
	showString(sstream);

	Mat cleanMask = Mat(image.rows, image.cols, CV_8UC1, Scalar(255));
	Mat regionMask = objectMask.clone();
	if (mode == ADD_FRONT) {
		//We only want to work outside the object
		regionMask = 255 - regionMask;
	}
	std::vector<Point> frontpath = Dijkstra::fillMultiplePaths(frontscribbles);
	std::vector<Point> backpath = Dijkstra::fillMultiplePaths(backscribbles);
	if (mode == FIRST || mode == ADD_FRONT) {
		if (simple) {
			frontPDF = KernelEstimation::simpleCalculatePDF(image, frontpath,
					cleanMask, range, blurMode);
		} else {
			frontPDF = KernelEstimation::calculatePDF(image, frontpath,
					cleanMask);
		}
	}
	if (mode == FIRST || mode == ADD_BACK) {
		if (simple) {
			backPDF = KernelEstimation::simpleCalculatePDF(image, backpath,
					cleanMask, range, blurMode);
		} else {
			backPDF = KernelEstimation::calculatePDF(image, backpath,
					cleanMask);
		}
	}
	Mat frontWeights = Mat(frontPDF.rows, frontPDF.cols, CV_32FC1);
	Mat backWeights = Mat(backPDF.rows, backPDF.cols, CV_32FC1);
	KernelEstimation::calculateLikelihood(frontPDF, backPDF, frontWeights,
			backWeights, cleanMask);
	Mat frontDistances, backDistances;
	if (!useLevelSet) {
		sstream << "Running Regular Dijkstra!";
		showString (sstream);
		double time2 = (double) getTickCount();
		frontDistances = Dijkstra::simpleComputePaths(frontWeights, frontpath,
				cleanMask);
		//showMatColorMap(frontDistances, "Front");
		backDistances = Dijkstra::simpleComputePaths(frontWeights, backpath,
				cleanMask);
		//showMatColorMap(backDistances, "Back");
		time2 = ((double) getTickCount() - time2) / getTickFrequency();
				sstream << "Regular Dijkstra finished in " << time2;
				showString(sstream);
	} else {
		sstream << "Running Level Set Dijkstra!";
		showString (sstream);
		double time2 = (double) getTickCount();
		Fimage likeliImage = openCVWithMW::matToFimage(frontWeights);
		Shapes resTree = mw_new_shapes();
		flst(NULL, likeliImage, resTree);
		mw_delete_fimage(likeliImage);
		frontDistances = openCVWithMW::DijkstraOnTree(resTree, frontpath,
				image.rows, image.cols);
		//showMatColorMap(frontDistances, "Front");
		backDistances = openCVWithMW::DijkstraOnTree(resTree, backpath,
				image.rows, image.cols);
		//showMatColorMap(backDistances, "Back");
		mw_delete_shapes(resTree);
		time2 = ((double) getTickCount() - time2) / getTickFrequency();
		sstream << "Level Set Dijkstra finished in " << time2;
		showString(sstream);
	}
	Mat resMask = KernelEstimation::findObject(frontDistances, backDistances,
			regionMask);
	if (mode == ADD_FRONT) {
		objectMask += resMask;
	} else {
		objectMask = resMask;
	}
	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "updateObjectMask finished in " << time;
	showString(sstream);
}

Mat KernelEstimation::generateAlpha(Mat& image, Mat& mask, int bandLen,
		double intensity, bool updatePixels, bool simple, int range,
		int blurMode) {
	Mat origImage = image.clone();
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running generateAlpha");
	showString(sstream);
	std::vector<std::vector<cv::Point> > roughContours, contours,
			scribbleContours;
	std::vector<cv::Vec4i> outerHirerchy;
	Mat mskCopy = mask.clone();
	Mat alpha;
	mask.convertTo(alpha, CV_32FC1, 1 / 255.0);
	cv::findContours(mskCopy, roughContours, outerHirerchy, CV_RETR_CCOMP,
			CV_CHAIN_APPROX_NONE);
//contours = smoothContours(roughContours, bandLen);
	contours = roughContours;
	mskCopy.release();
	bool isOuter = true;
	//Do alpha calculation for each contour
	for (int i = 0; i < contours.size(); i++) {
		vector<Point>& vec = contours[i];
		int size = vec.size();
		if (size <= 2 * bandLen) {
			//Too small for alpha, probably noise
			continue;
		}
		//TODO: think of a good way to avoid this noise
		isOuter = !(outerHirerchy[i][3] != -1); // Doesn't have a father
		stringstream sstream;
		Mat bandMask = Mat(image.rows, image.cols, CV_8UC1, Scalar(0)); //Defines the region of interest.
		Mat bandBounds = Mat(image.rows, image.cols, CV_8UC1, Scalar(0)); //Used for creating the inner and outer scribbles
		cv::drawContours(bandMask, contours, i, cv::Scalar(255), bandLen);
		sstream << i << "mask";
		//imshow(sstream.str().c_str(), bandMask);
		cv::drawContours(bandBounds, contours, i, cv::Scalar(255), bandLen);
		Mat emptyCanvas = Mat(image.rows, image.cols, CV_8UC1, Scalar(0)); //Used for creating the inner and outer scribbles
		std::vector<cv::Vec4i> hirerchy;

		cv::findContours(bandBounds, scribbleContours, hirerchy, CV_RETR_CCOMP,
				CV_CHAIN_APPROX_NONE);
		//For each object take care of all subcontours
		for (int j = 0; j < scribbleContours.size(); j++) {
			if (hirerchy[j][2] != -1) { // Has a child
				int index = isOuter ? j : hirerchy[j][2];
				int childIndex = isOuter ? hirerchy[j][2] : j;
				Mat showImage = origImage.clone();
				if (isOuter) {
					childIndex = biggestContour(scribbleContours, hirerchy, j);
				}

				sstream << i << "," << j;
				//imshow(sstream.str().c_str(), showImage);
				/*EO*/
				std::vector<Point> frontBand = scribbleContours[childIndex];
				std::vector<Point> backBand = scribbleContours[j];
				if (!assertScribbleVal(frontBand, mask, 255)) {
					sstream.str("False Scribble");
					showString(sstream);
					continue;
				}
				Mat frontBandPDF, backBandPDF;
				if (simple) {
					frontBandPDF = KernelEstimation::simpleCalculatePDF(
							origImage, frontBand, bandMask, range, blurMode);
				} else {
					frontBandPDF = KernelEstimation::calculatePDF(origImage,
							frontBand, bandMask);
				}
				if (simple) {
					backBandPDF = KernelEstimation::simpleCalculatePDF(
							origImage, backBand, bandMask, range, blurMode);
				} else {
					backBandPDF = KernelEstimation::calculatePDF(origImage,
							backBand, bandMask);
				}
				/*Mat frontBandPDF = KernelEstimation::simpleCalculatePDF(
				 origImage, frontBand, bandMask, 16, 8);
				 Mat backBandPDF = KernelEstimation::simpleCalculatePDF(
				 origImage, backBand, bandMask, 16, 8);*/
				Mat frontBandWeights = Mat(frontBandPDF.rows, frontBandPDF.cols,
				CV_32FC1);
				Mat backBandWeights = Mat(backBandPDF.rows, backBandPDF.cols,
				CV_32FC1);
				KernelEstimation::calculateLikelihood(frontBandPDF, backBandPDF,
						frontBandWeights, backBandWeights, bandMask);
				Mat frontBandDistances = Dijkstra::simpleComputePaths(
						frontBandWeights, frontBand, bandMask);
				Mat backBandDistances = Dijkstra::simpleComputePaths(
						frontBandWeights, backBand, bandMask);
				KernelEstimation::updateAlpha(frontBandDistances,
						backBandDistances, frontBandPDF, backBandPDF, bandMask,
						alpha, intensity);

				if (updatePixels) {
					//We've updated the alpha, now we want to choose the proper pixel
					updateBoundPixels(image, alpha, bandMask, frontBand,
							backBand, bandLen);
					sstream << "Resulting Image" << i << "," << j;
					imshow(sstream.str().c_str(), image);
					/*Define the reigo */
				}
				//sstream << "Alpha";
				//imshow(sstream.str().c_str(), partAlpha);
			}

		}

	}
	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "generateAlpha finished in " << time;
	showString(sstream);
	return alpha;

}

void KernelEstimation::updateBoundPixels(Mat& image, Mat& alpha, Mat& bandMask,
		vector<Point>& frontBand, vector<Point>& backBand, int radius) {
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running updateBoundPixels");
	showString(sstream);
	radius *= 2;
	Mat frontScribble, backScribble;
	frontScribble = Mat(image.rows, image.cols, CV_8UC1, Scalar(0));
	polylines(frontScribble, frontBand, false, Scalar(255));
	imshow("Front Band", frontScribble);
	backScribble = Mat(image.rows, image.cols, CV_8UC1, Scalar(0));
	polylines(backScribble, backBand, false, Scalar(255));
	imshow("Back Band", backScribble);
//drawContours(frontScribble, contours, childIndex, cv::Scalar(255));
//Mat frontInBound, backInBound;
	vector<Point> bandPoints;
	findNonZero(bandMask, bandPoints);

	for (std::vector<Point>::iterator it = bandPoints.begin();
			it != bandPoints.end(); ++it) {
		Point p = *it;
		float alphaVal = alpha.at<float>(p);
		if (alphaVal == 0.f) {	// || alphaVal == 1.f) {
			continue;
		}
		Vec3f& imageVal = image.at<Vec3f>(p);
		//Find bounding rectangle.
		int x1, y1, x2, y2;
		x1 = p.x - radius < 0 ? 0 : p.x - radius;
		y1 = p.y - radius < 0 ? 0 : p.y - radius;
		x2 = p.x + radius >= bandMask.cols ? bandMask.cols - 1 : p.x + radius;
		y2 = p.y + radius >= bandMask.rows ? bandMask.rows - 1 : p.y + radius;
		Rect bound = Rect(x1, y1, x2 - x1, y2 - y1);
		Mat frontInBound = frontScribble(bound);
		Mat backInBound = backScribble(bound);

		if (countNonZero(frontInBound) == 0 || countNonZero(backInBound) == 0) {
			continue;
		}
		//We've got our rectangle
		imshow("Front Bound", frontInBound);
		imshow("Back Bound", backInBound);
		if (it == bandPoints.begin()) {
			//waitKey(0);
		}

		//Time to go over it :o
		vector<Point> boundFrontPoints;
		findNonZero(frontInBound, boundFrontPoints);
		vector<Point> boundBackPoints;
		findNonZero(backInBound, boundBackPoints);
		//Now we have the vector for both foreground and background, need to find best couple

		//For now - Let's try every pair
		float minError = std::numeric_limits<float>::infinity();//C++ style ;)
		Vec3f fBestColor, bBestColor;
		for (int i = 0; i < boundFrontPoints.size(); i++) {
			Point fP = boundFrontPoints[i];
			Point translatedfP = Point(fP.x + x1, fP.y + y1);
			Vec3f fColor = image.at<Vec3f>(translatedfP);

			for (int j = 0; j < boundBackPoints.size(); j++) {
				Point bP = boundFrontPoints[i];
				Point translatedbP = Point(bP.x + x1, bP.y + y1);
				Vec3f bColor = image.at<Vec3f>(translatedbP);
				float calcError = 0.f;
				for (int k = 0; k < 3; k++) {
					float res = (fColor[k] * alphaVal
							+ bColor[k] * (1 - alphaVal) - imageVal[k]);
					calcError += res * res;
				}
				if (minError > calcError) {
					minError = calcError;
					fBestColor = fColor;
					bBestColor = bColor;
				}
			}
		}
		//Let's update the color
		for (int k = 0; k < 3; k++) {
			/*			imageVal[k] = (fBestColor[k] * alphaVal
			 + bBestColor[k] * (1 - alphaVal));*/
			imageVal[k] = fBestColor[k];
		}

	}
	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "updateBoundPixels finished in " << time;
	showString(sstream);
}

void KernelEstimation::cleanContours(
		std::vector<std::vector<cv::Point> >& contours, int minLen) {
	std::vector<std::vector<cv::Point> >::iterator it = contours.begin();

	while (it != contours.end()) {
		if (it->size() < minLen) {
			it = contours.erase(it);
		} else {
			it++;
		}
	}
}

bool KernelEstimation::assertScribbleVal(std::vector<cv::Point> scribble,
		Mat& mask, int val) {
	if (scribble.size() == 0) {
		return false;
	}
	std::vector<Point>::iterator vectorIt, vectorEnd;
	for (vectorIt = scribble.begin(); vectorIt != scribble.end(); vectorIt++) {
		Point p = *vectorIt;
		//int x = p.x, y = p.y;
		if (mask.at<uchar>(p) != val) {
			return false;
		}
	}
	return true;
}

void KernelEstimation::addToAlpha(Mat& alpha, Mat& toAdd) {
	cv::Size size = alpha.size();
	if (alpha.isContinuous() && toAdd.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	for (int i = 0; i < size.height; i++) {
		float* alphaPtr = alpha.ptr<float>(i);
		float* toAddPtr = toAdd.ptr<float>(i);
		for (int j = 0; j < size.width; j++) {
			float aVal = alphaPtr[j], tVal = toAddPtr[j];
			alphaPtr[j] = tVal > aVal ? tVal : aVal;
		}
	}
}
int KernelEstimation::biggestContour(vector<vector<Point> >& contours,
		std::vector<cv::Vec4i> hirerchy, int fatherIndex) {
	int maxIndex = -1;
	int maxSize = -1;
	int childIndex = hirerchy[fatherIndex][2];
	while (childIndex != -1) {
		vector<Point>& vec = contours[childIndex];
		int size = vec.size();
		if (size > maxSize) {
			maxIndex = childIndex;
			maxSize = size;
		}
		childIndex = hirerchy[childIndex][0];
	}
	return maxIndex;
}

Rect KernelEstimation::getBoundaries(Mat & alpha) {
	int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
	cv::Size size = alpha.size();
	for (int i = 0; i < size.height; i++) {
		float* alphaPtr = alpha.ptr<float>(i);
		for (int j = 0; j < size.width; j++) {
			float aVal = alphaPtr[j];
			if (aVal != 0.f) {
				x1 = x1 == -1 ? j : std::min(x1, j);
				x2 = std::max(x2, j);
				y1 = y1 == -1 ? i : std::min(y1, i);
				y2 = std::max(y2, i);
			}

		}
	}
	return Rect(x1, y1, x2 - x1, y2 - y1);
	//return Rקבא
}

void KernelEstimation::roundMask(Mat& mask) {
	cv::Size size = mask.size();
	if (mask.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	for (int i = 0; i < size.height; i++) {
		uchar* maskPtr = mask.ptr<uchar>(i);
		for (int j = 0; j < size.width; j++) {
			uchar aVal = maskPtr[j];
			if (aVal < 128) {
				maskPtr[j] = 0;
			} else {
				maskPtr[j] = 255;
			}
		}
	}
}

vector<vector<Point> > KernelEstimation::smoothContours(
		vector<vector<Point> >& contours, int k) {
	vector<vector<Point> > smoothed;
	vector<vector<Point> >::iterator vectorIt, vectorEnd;
	for (vectorIt = contours.begin(); vectorIt != contours.end(); vectorIt++) {
		vector<Point> smooth;
		//approxPolyDP(*vectorIt, smooth, k, true);
		Mat smoothMat = Mat(*vectorIt);
		Mat result;
		cv::copyMakeBorder(smoothMat, smoothMat, (k - 1) / 2, (k - 1) / 2, 0, 0,
				cv::BORDER_WRAP);
		cv::blur(smoothMat, result, cv::Size(k, k), cv::Point(-1, -1));
		result.copyTo(smooth);
		result.rowRange(cv::Range((k - 1) / 2, 1 + result.rows - (k - 1) / 2)).copyTo(
				smooth);
		smoothed.push_back(smooth);
	}
	return smoothed;
}

void KernelEstimation::mergeImages(Mat& object, Mat& back, Mat& merged,
		Mat& mask) {
	cv::Size size = mask.size();
	if (object.isContinuous() && back.isContinuous() && mask.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	size.width *= 3;
	for (int i = 0; i < size.height; i++) {
		float* maskPtr = mask.ptr<float>(i);
		float* objectPtr = object.ptr<float>(i);
		float* backPtr = back.ptr<float>(i);
		float* mergedPtr = merged.ptr<float>(i);
		for (int j = 0; j < size.width; j += 3) {
			float maskVal = maskPtr[j / 3];
			for (int k = 0; k < 3; k++) {
				float objectVal = objectPtr[j + k], backVal = backPtr[j + k],
						mergedVal;
				mergedVal = objectVal * maskVal + backVal * (1 - maskVal);
				//cout << mergedVal;
				mergedPtr[j + k] = mergedVal;
				/*			mergedPtr[j + k] = objectPtr[j + k] * maskPtr[j]
				 + backPtr[j + k] * (1 - maskPtr[j]);*/
			}
		}
	}
}

Mat KernelEstimation::getChannelMatrix(Mat& img, int channel) {
	Mat res = Mat(img.rows, img.cols, CV_32FC1, Scalar(0));
	cv::Size size = img.size();
	if (img.isContinuous() && res.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	size.width *= 3;
	for (int i = 0; i < size.height; i++) {
		float* imgPtr = img.ptr<float>(i);
		float* resPtr = res.ptr<float>(i);

		for (int j = 0; j < size.width; j += 3) {
			resPtr[j / 3] = imgPtr[j + channel];
		}
	}
	return res;
}

Mat KernelEstimation::blurImage(Mat& im, Mat& seg, bool blurForeground,
		int r_defocus, bool smartOutline) {
	double time = (double) getTickCount();
	stringstream sstream;
	sstream.str("Running blurImage");
	showString(sstream);
	sstream << "radius is" << r_defocus;
	showString(sstream);
	Mat im_defocus;
	r_defocus += 1 - r_defocus % 2; //Make odd
	Mat h_defocus = get2DGaussianFilter(r_defocus);
//GaussianBlur(im, im_defocus, Size(r_defocus , r_defocus), 0, 0);
	filter2D(im, im_defocus, -1, h_defocus);
	Mat merged = im.clone();

	if (blurForeground) { //Foreground occluding object is out of focus
		//Enlarge the background ('object' in this case) - to smooth its boundaries
		erode(seg, seg,
				getStructuringElement(MORPH_ELLIPSE,
						cv::Size(r_defocus / 2, r_defocus / 2))); //The minimum one :P
		//We get a bigger object to blur, since we are blurring the foreground object the result looks nice.
		Mat seg_inv;
		cv::threshold(seg, seg_inv, 0.5, 1.0, THRESH_BINARY_INV);
		Mat mask;
		conv2(seg_inv, get2DGaussianFilter(r_defocus / 2), CONVOLUTION_SAME,
				mask);
		threshold(1 - mask, mask, 0.f, 0.f, THRESH_TOZERO); //Reverse mask, but No less than 0
		mergeImages(im, im_defocus, merged, mask);

	} else { //We are blurring the background

		if (smartOutline) {
			Mat dilatedSeg;
			dilate(seg, dilatedSeg,
					getStructuringElement(MORPH_ELLIPSE,
							cv::Size(r_defocus + 1, r_defocus + 1))); //The maximum one :P
			//seg==0 if background
			//We want to blur less of the background, so we take the maximum.
			Mat seg_nb = dilatedSeg - seg;
			//showMat(seg_nb, "seg_nb");
			//seg_nb==0 if not background
			//We've increased the notbackground segment
			Mat seg_nbU; //Need uchar mat
			seg_nb.convertTo(seg_nbU, CV_8UC1, 255);
			Mat locations;
			findNonZero(seg_nbU, locations);
			seg_nbU.release();
			//Find non-zero points in seb_nb, means not-background points
			Mat paddedImage;
			copyMakeBorder(im, paddedImage, r_defocus / 2, r_defocus / 2,
					r_defocus / 2, r_defocus / 2, BORDER_REFLECT);
			Mat seg_bg;
			cv::threshold(seg, seg_bg, 0.5, 1.0, THRESH_BINARY_INV); //reverse seg
			copyMakeBorder(seg_bg, seg_bg, r_defocus / 2, r_defocus / 2,
					r_defocus / 2, r_defocus / 2, BORDER_REFLECT);

			vector<Mat> imChanneled;
			split(paddedImage, imChanneled);
			cv::Size size = locations.size();
			if (locations.isContinuous()) {
				size.width *= size.height;
				size.height = 1;
			}
			for (int i = 0; i < size.height; i++) {
				Point* locationPtr = locations.ptr<Point>(i);
				for (int j = 0; j < size.width; j++) {
					Point p = locationPtr[j];
					Rect region = Rect(p.x, p.y, r_defocus, r_defocus);
					Mat seg_patch = seg_bg(region);
					Vec3f& bVal = im_defocus.at<Vec3f>(p);
					for (int k = 0; k < 3; k++) {
						Mat im_patch = imChanneled[k](region);
						/*	Mat res1 = im_patch.mul(h_defocus).mul(seg_patch);
						 float val1 = sum(res1)[0];
						 Mat res2 = (h_defocus).mul(seg_patch);
						 float val2 = sum(res2)[0];
						 bVal[k] = val1 / val2;*/
						bVal[k] = sum(im_patch.mul(h_defocus).mul(seg_patch))[0]
								/ sum(h_defocus.mul(seg_patch))[0];
						//bVal[k] = sum(im_patch.mul(h_defocus))[0] / sum(h_defocus)[0];
					}
				}
			}

		}

		Mat seg_inv;
		//seg_inv = ~seg; //Rubbish
		cv::threshold(seg, seg_inv, 0.5, 1.0, THRESH_BINARY_INV);
		Mat mask;
		conv2(seg_inv, get2DGaussianFilter(3), CONVOLUTION_SAME, mask);
		mask = 1 - mask;
		threshold(mask, mask, 0.f, 0.f, THRESH_TOZERO); //No more than 1
		mergeImages(im, im_defocus, merged, mask);
	}

	time = ((double) getTickCount() - time) / getTickFrequency();
	sstream << "blurImage finished in " << time;
	showString(sstream);
	return merged;

}

Mat KernelEstimation::get2DGaussianFilter(int radius) {
	Mat kernelX = getGaussianKernel(radius, 0, CV_32F);
//cout << kernelX << endl;
	Mat kernelY = getGaussianKernel(radius, 0, CV_32F);
//cout << kernelY << endl;
	Mat kernelXY = kernelX * kernelY.t();
//cout << kernelXY << endl;
	return kernelXY;
}

Mat KernelEstimation::get3DGaussianFilter(int radius) {
//Get 3D gaussion
	int sizes[] = { radius, radius, radius };
	Mat gaussian = getGaussianKernel(radius, 0, CV_32F);
	Mat gaussian2D = get2DGaussianFilter(radius);
//cout << "1D:\n" << gaussian << endl;
//cout << "2D:\n" << gaussian2D << endl;
//cout << "3D:" << endl;
	Mat gaussian3D = Mat(3, sizes, CV_32FC1, Scalar(0));
	for (int k = 0; k < radius; k++) {
		//cout << "Slice " << k << ":" << endl;
		for (int i = 0; i < radius; i++) {
			for (int j = 0; j < radius; j++) {
				//TODO: find smarter way to calculate
				float val = gaussian2D.at<float>(i, j) * gaussian.at<float>(k);
				gaussian3D.at<float>(i, j, k) = val;
//						cout << val << ",";
			}
		}
		//cout << gaussian2D * gaussian.at<float>(k) << endl;
	}
	return gaussian3D;
}

void KernelEstimation::conv2(const Mat &img, const Mat& kernel,
		ConvolutionType type, Mat& dest) {
	Mat source = img;
	if (CONVOLUTION_FULL == type) {
		source = Mat();
		const int additionalRows = kernel.rows - 1, additionalCols = kernel.cols
				- 1;
		copyMakeBorder(img, source, (additionalRows + 1) / 2,
				additionalRows / 2, (additionalCols + 1) / 2,
				additionalCols / 2, BORDER_CONSTANT, Scalar(0));
	}

	Point anchor(kernel.cols - kernel.cols / 2 - 1,
			kernel.rows - kernel.rows / 2 - 1);
	int borderMode = BORDER_CONSTANT;
	Mat flippedKernel;
	cv::flip(kernel, flippedKernel, -1);
	filter2D(source, dest, img.depth(), flippedKernel, anchor, 0, borderMode);
	if (CONVOLUTION_VALID == type) {
		dest =
				dest.colRange((kernel.cols - 1) / 2,
						dest.cols - kernel.cols / 2).rowRange(
						(kernel.rows - 1) / 2, dest.rows - kernel.rows / 2);
	}
}

Mat KernelEstimation::smoothMask(Mat& mask, int k) {
//blur(mask, mask, Size(13,13));
	/*
	 Mat workWith = mask.clone();
	 vector<vector<Point> > contours, smoothed, smoothed2;
	 cv::findContours(workWith, contours, CV_RETR_EXTERNAL,
	 CV_CHAIN_APPROX_NONE);
	 vector<vector<Point> >::iterator vectorIt, vectorEnd;
	 for (vectorIt = contours.begin(); vectorIt != contours.end(); vectorIt++) {
	 vector<Point> smooth, smooth2;
	 approxPolyDP(*vectorIt, smooth2, 2, true);
	 smoothed2.push_back(smooth2);
	 Mat smoothMat = Mat(*vectorIt);
	 Mat result;
	 cv::copyMakeBorder(smoothMat, smoothMat, (k - 1) / 2, (k - 1) / 2, 0, 0,
	 cv::BORDER_WRAP);
	 cv::blur(smoothMat, result, cv::Size(k, k), cv::Point(-1, -1));
	 result.copyTo(smooth);
	 result.rowRange(cv::Range((k - 1) / 2, 1 + result.rows - (k - 1) / 2)).copyTo(
	 smooth);
	 smoothed.push_back(smooth);
	 }
	 cv::drawContours(mask, smoothed, -1, Scalar(255), 1);
	 Mat show = Mat(mask.rows, mask.cols, CV_8UC3, Scalar(0, 0, 0));
	 cv::drawContours(show, contours, -1, Scalar(0, 0, 255), 1);
	 showMat(show, "Original Contour");
	 show = Mat(mask.rows, mask.cols, CV_8UC3, Scalar(0, 0, 0));
	 cv::drawContours(show, smoothed, -1, Scalar(0, 255, 0), 1);
	 showMat(show, "Blur Contour");
	 show = Mat(mask.rows, mask.cols, CV_8UC3, Scalar(0, 0, 0));
	 cv::drawContours(show, smoothed2, -1, Scalar(255, 0, 0), 1);
	 showMat(show, "Approx Contour");
	 //imshow("Showing", show);
	 */
	Mat maskCpy = mask.clone();
	Mat res;
	blur(maskCpy, res, Size(k, k), Point(-1, -1));
	return res;
}

Mat KernelEstimation::grayScaleMat(Mat& image, float intensity, Mat& mask) {
	Mat res = image.clone();
	Mat grey;
	cvtColor(image, grey, CV_BGR2GRAY);
	Size size = res.size();
	if (grey.isContinuous() && res.isContinuous() && mask.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}
	size.width *= 3;
	for (int i = 0; i < size.height; i++) {
		float* greyPtr = grey.ptr<float>(i);
		float* resPtr = res.ptr<float>(i);
		uchar* maskPtr = mask.ptr<uchar>(i);
		for (int j = 0; j < size.width; j++) {
//			resPtr[j / 3] = imgPtr[j + channel];
			if (maskPtr[j / 3] == 0) {
				continue;
			}
			resPtr[j] = greyPtr[j / 3] * intensity
					+ resPtr[j] * (1 - intensity);

		}
	}
	return res;

}

//bool isInner(std::vector<Point> scrible)
