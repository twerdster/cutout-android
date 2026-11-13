// Updated 2026 for CutOut restoration; original authors/notices retained below.
/*
 * KernelEstimation.h
 *
 *  Created on: Aug 11, 2014
 *      Author: elad
 */

#ifndef KERNELESTIMATION_H_
#define KERNELESTIMATION_H_

#include <opencv2/core/core.hpp>
#include <set>
#include <utility>
#include <vector>

using namespace cv;
using std::vector;
class KernelEstimation {
	static const int D = 3;
	static const int Lmin = 0;
	static const int Lmax = 100;
	static const int amin = -127;
	static const int amax = 127;
	static const int bmin = -127;
	static const int bmax = 127;
	static void cleanContours(std::vector<std::vector<cv::Point> >& contours,
			int minLen);
	static void addToAlpha(Mat& alpha, Mat& toAdd);
	static bool assertScribbleVal(std::vector<cv::Point> scribble, Mat& mask,
			int val);
	static int biggestContour(std::vector<std::vector<cv::Point> >& contours,
			std::vector<cv::Vec4i> hirerchy, int fatherIndex);
public:
	typedef enum Mode {
			FIRST = 0, ADD_FRONT = 1, ADD_BACK = 2
		} Mode;

		/**
		 * calculatePDF - computes PDF for each pixel
		 * @param sources - a vector of the points that are the base for the PDF
		 * @param image - the full image
		 * @param mask - defines the region of interest
		 * @return - The PDF value for each pixel
		 */
		static Mat calculatePDF(Mat& image, std::vector<Point> sources, Mat& mask);

		static Mat simpleCalculatePDF(Mat& image, std::vector<Point> sources,
				Mat& mask, int range, int blurMode);

		static Point3d labToRange(Point3f& p, float range);

		/**
		 * calculateLikelihood - Computes the relative likelihood for each pixel
		 * @param frontPDF, backPDF - matrices with PDF values for each pixel
		 * @param frontLike, backLike - matrices that will contain the the likelihood for each pixel
		 * @param mask - defines the region of interest
		 */
		static void calculateLikelihood(Mat& frontPDF, Mat& backPDF, Mat& frontLike,
				Mat& backLike, Mat& mask);

		/**
		 * findObject - Determine which pixels belong to the object, based on the distances.
		 * @param frontDistances, backDistances - The distance from the scribble to the object.
		 * @param mask - defines the region of interest
		 * @return - a mask representing the object.
		 */
		static Mat findObject(Mat& frontDistances, Mat& backDistances, Mat& mask);

		/**
		 * findAlpha - Determine the alpha values in the band.
		 * @param frontDistances, backDistances - The distance from the scribble to the object.
		 * @param frontPDF, backPDF - the PDF for each scribble
		 * @param mask - defines the region of interest
		 * @param bandMask - defines the band region
		 * @return - a matrix representing the alpha values in 32FC1
		 */
		static void updateAlpha(Mat& frontDistances, Mat& backDistances,
				Mat& frontPDF, Mat& backPDF, Mat& bandMask, Mat& alpha,
				double intensity);

		/**
		 * addAlpha - Adds an Alpha Channel to an image
		 * @param image - the BGR image, in 32FC3
		 * @param alpha - the Alpha matrix in 32FC1
		 * @return the BGRA image in 32FC4
		 */
		static Mat addAlpha(Mat& image, Mat& alpha);

		/*
		 * updateObjectMask - updates the mask representing the object for the segmentation
		 * @param image - the original image in 32FC3 Format
		 * @param objectMask - the binary mask in 8UC1
		 * @param frontScribbles - The vector of front scribbles
		 * @param frontScribbles - The vector of back scribbles
		 * @param the function Mode
		 * * FRIST - first run, need to create both PDFs
		 * * ADD_FRONT - adding a front scribble, need to update the front PDF
		 * * ADD_BACK - adding a back scribble, need to update the back PDF
		 */
		static void updateObjectMask(Mat& image, Mat& objectMask,
				vector<vector<Point> > frontScribbles,
				vector<vector<Point> > backScribbles, Mat& frontPDF, Mat& backPDF,
				Mode mode, bool simple = false, int range = 32, int blurMode = 0, bool useLevelSet = false);

		/*
		 * generateAlpha - Generate the alpha channel
		 * @param image - the original image
		 * @param mask - the object's mask
		 * @return - the alpha channel in 32FC1
		 */
		static Mat generateAlpha(Mat& image, Mat& mask, int bandLen,
				double intensity = 1.5, bool updatePixels = false, bool simple = false, int range = 32, int blurMode = 0);

		static void updateBoundPixels(Mat& image, Mat& alpha, Mat& bandMask,
				vector<Point>& frontBand, vector<Point>& backBand, int radius);
		static Rect getBoundaries(Mat& alpha);

		static void roundMask(Mat& mask);

		static vector<vector<Point> > smoothContours(
				vector<vector<Point> >& contours, int k);

		/**
		 * blurImage - blur an Image's background of foreground
		 * @param image - the image to blur
		 * @param seg - the 32FC1 mask of the area to blur (blur if equals 0)
		 * @param blurBackground - Does the mask represent the background
		 * @param radius - The raduis of the blurring
		 */
		static Mat blurImage(Mat& image, Mat& seg, bool blurForeground,
				int r_defocus, bool smartOutline);

		static Mat grayScaleMat(Mat& image, float intensity, Mat& mask);

		/**
		 * mergeImages - Merges two images
		 * object - the front object, denoted by mask
		 * back - the back object
		 * mask - the 32FC1 mask
		 *
		 */
		static void mergeImages(Mat& object, Mat& back, Mat& merged, Mat& mask);

		static Mat smoothMask(Mat& mask, int k);
		enum ConvolutionType {
			/* Return the full convolution, including border */
			CONVOLUTION_FULL,

			/* Return only the part that corresponds to the original image */
			CONVOLUTION_SAME,

			/* Return only the submatrix containing elements that were not influenced by the border */
			CONVOLUTION_VALID
		};

		static void conv2(const Mat &img, const Mat& kernel, ConvolutionType type,
				Mat& dest);

		static Mat getChannelMatrix(Mat& img, int channel);

		static Mat get2DGaussianFilter(int radius);
		static Mat get3DGaussianFilter(int radius);

		template<typename T>
		static T boundToEdges(T val, T min, T max) {
			val = (val < min) ? min : val;
			val = (val > max) ? max : val;
			return val;
		}
};

#endif /* KERNELESTIMATION_H_ */
