#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <vector>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/photo/photo.hpp>
#include <android/log.h>
#include <stdio.h>
#include <string.h> // for memset
#include <math.h>   // for abs
#include "../include/figtree/figtree.h"
#include "Dijkstra.h"
#include "KernelEstimation.h"
#include "PlatformDependent.h"

using namespace std;
using namespace cv;

extern "C" {

/**
 * DoKernelEstimation - Find the object using Kernel Estimation
 * @param addrImg - 8UC4 Matrix - The address of the original Image.
 * @param addrFrontSources/addrBackSources - 32FC2 Matrix - The address of 2Channel the Front/Back Scribbles Matrix
 *		* Translated to a vector of vectors using matToVectors.	*
 *		* Each row contains one vector, where the coordinates are saved in the 1th, 2th channels	*
 *		* The end of scribbles that are shorter than the number of cols is denoted by (-1,-1) coordi nate *
 * @param addrRes - 8UC3 Matrix - The address of the result matrix, where the object is outlined.
 * @param mode - The scribbling mode, as defined by KernelEstimation::Mode
 * @param addrFrontPDF/addrBackPDF - 32FC1 Matrix - The address of the PDF matching the front/back sources, saved between calls
 * @param addrObjectMask - 8UC1 Matrix - The resulting mask.
 * 		* mask == 255 where object is located *
 */
JNIEXPORT void JNICALL Java_com_eladrich_scribbler_MainActivity_DoKernelEstimation(
		JNIEnv*, jobject, jlong addrImg, jlong addrFrontSources,
		jlong addrBackSources, jlong addrRes, jint mode, jlong addrFrontPDF,
		jlong addrBackPDF, jlong addrObjectMask, jboolean useFIGTree, jboolean useLevelset);

/**
 * DoAlphaFromMask - Generate image with alpha channel, according to the mask.
 * @param addrImg - 8UC4 Matrix - The address of the original Image.
 * @param addrMask - 8UC1 Matrix - The address of the object's mask.
 * @param addrRes - 8UC4 Matrix - The address where the result will be saved, with the new alpha channel.
 */
JNIEXPORT void JNICALL Java_com_eladrich_scribbler_MainActivity_DoAlphaFromMask(
		JNIEnv*, jobject, jlong addrImg, jlong addrMask, jlong addrRes, jlong addrBound, jboolean useFIGTree);

/**
 * DoBlurFromMask - Blur the background/foreground of the image.
 * @param addrImg - 8UC4 Matrix - The address of the original Image.
 * @param addrMask - 8UC1 Matrix - The address of the object's mask.
 * @param addrRes - 8UC4 Matrix - The address where the result will be saved, after being blurred
 * @param radius - The blurring raduis
 * 		* positive value means we are are blurring the object *
 * 		* negative value means we are blurring the background *
 */
JNIEXPORT void JNICALL Java_com_eladrich_scribbler_MainActivity_DoBlurFromMask(
		JNIEnv*, jobject, jlong addrImg, jlong addrMask, jlong addrRes,
		jint radius);

/**
 * DoGrayFromMask - Gray out the background/foreground of the image.
 * @param addrImg - 8UC4 Matrix - The address of the original Image.
 * @param addrMask - 8UC1 Matrix - The address of the object's mask.
 * @param addrRes - 8UC4 Matrix - The address where the result will be saved, after being blurred
 * @param jIntensity - The graying intensity
 * 		* positive value means we are are blurring the object *
 * 		* negative value means we are blurring the background *
 */
JNIEXPORT void JNICALL Java_com_eladrich_scribbler_MainActivity_DoGrayFromMask(
		JNIEnv*, jobject, jlong addrImg, jlong addrMask, jlong addrRes,
		jfloat jIntensity);

/**
 * matToVectors - Convert a matrix repressing a vector of vectors
 * @param scribblesMat - The matrix to convert
 * @param scribbles - The vector of vectors to fill
 * @param scale - scaling parameter
 *		* Each row contains one vector, where the coordinates are saved in the 1th, 2th channels	*
 *		* The end of scribbles that are shorter than the number of cols is denoted by (-1,-1) coordi nate *
 */
void matToVectors(Mat& scribblesMat, vector<vector<Point> >& scribbles,
		float scale);

/**
 * mergeWithAlpha - Merge an image with a new alpha channel
 * @param image - 32FC4 Matrix - The original Image
 * @param alpha - 32FC1 Matrix - the new alpha channel
 * @result - 32FC4 Matrrix - The combined Matrix
 */
Mat mergeWithAlpha(Mat& image, Mat& alpha);

/**
 * Converts Matrix type to matching string
 */
string type2str(int type);

JNIEXPORT void JNICALL Java_com_eladrich_scribbler_MainActivity_DoKernelEstimation(
		JNIEnv*, jobject, jlong addrImg, jlong addrFrontSources,
		jlong addrBackSources, jlong addrRes, jint mode, jlong addrFrontPDF,
		jlong addrBackPDF, jlong addrObjectMask, jboolean useFIGTree, jboolean useLevelset) {
	Mat& imgMat4U = *(Mat*) addrImg;
	Mat& frontSourcesMat = *(Mat*) addrFrontSources;
	Mat& backSourcesMat = *(Mat*) addrBackSources;
	Mat& resMat3U = *(Mat*) addrRes;
	Mat& frontPDF = *(Mat*) addrFrontPDF;
	Mat& backPDF = *(Mat*) addrBackPDF;
	Mat& objectMask = *(Mat*) addrObjectMask;
	stringstream sstream;
	Mat imgMat3U, imgMat3F;
	vector<vector<Point> > frontScribbles, backScribbles;

	/** Change to format we can work with */
	cvtColor(imgMat4U, imgMat3U, CV_BGRA2BGR);
	imgMat3U.convertTo(imgMat3F, CV_32FC3, 1 / 255.0);

	sstream.str("Let's get started!");
	showString(sstream);

	int im = (int) mode;
	KernelEstimation::Mode m;
	switch (im) {
	case 1:
		m = KernelEstimation::ADD_FRONT;
		break;
	case 2:
		m = KernelEstimation::ADD_BACK;
		break;
	default:
		m = KernelEstimation::FIRST;
		break;
	}
	int maxLen = imgMat3F.rows < imgMat3F.cols ? imgMat3F.cols : imgMat3F.rows;
	float scale = 1.0;
	if (maxLen > 400.0) {
		scale = 400.0 / maxLen;
		sstream << "Scaling by" << scale;
		showString(sstream);
		Mat scaledImage, scaledObjectMask;
		Size size = Size(0, 0);
		resize(imgMat3F, scaledImage, size, scale, scale, INTER_AREA);
		resize(objectMask, scaledObjectMask, size, scale, scale, INTER_AREA);
		sstream << "Scaled to" << scaledImage.size;
		showString(sstream);
		matToVectors(frontSourcesMat, frontScribbles, scale);
		matToVectors(backSourcesMat, backScribbles, scale);
		KernelEstimation::updateObjectMask(scaledImage, scaledObjectMask,
				frontScribbles, backScribbles, frontPDF, backPDF, m, !useFIGTree, 16, 8, useLevelset);
		resize(scaledObjectMask, objectMask, objectMask.size(), 0, 0,
				INTER_CUBIC);
		KernelEstimation::roundMask(objectMask);
		objectMask = KernelEstimation::smoothMask(objectMask, 5 / scale);
		KernelEstimation::roundMask(objectMask);
	} else {
		matToVectors(frontSourcesMat, frontScribbles, 1.0);
		matToVectors(backSourcesMat, backScribbles, 1.0);
		KernelEstimation::updateObjectMask(imgMat3F, objectMask, frontScribbles,
				backScribbles, frontPDF, backPDF, m, !useFIGTree, 16 ,8, useLevelset);
	}

	/** Draw outline around object*/
	vector<vector<Point> > contours;
	Mat tmpMask = objectMask.clone(); //findContours changes source
	cv::findContours(tmpMask, contours, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE);
	tmpMask.release(); //Only needed for finding contours

	resMat3U = imgMat3U.clone();
	cv::drawContours(resMat3U, contours, -1, Scalar(130, 120, 255), 5 / scale);

	sstream.str("Done :)");
	showString(sstream);
}

JNIEXPORT void JNICALL Java_com_eladrich_scribbler_MainActivity_DoAlphaFromMask(
		JNIEnv*, jobject, jlong addrImg, jlong addrMask, jlong addrRes, jlong addrBound, jboolean useFIGTree) {
	Mat& imgMat4U = *(Mat*) addrImg;
	Mat& maskMat = *(Mat*) addrMask;
	Mat& resMat = *(Mat*) addrRes;
	Mat& boundMat = *(Mat*) addrBound;
	Mat imgMat4F, imgMat;
	stringstream sstream;

	/** Change to format we can work with */
	imgMat4U.convertTo(imgMat4F, CV_32FC4, 1 / 255.0);
	cvtColor(imgMat4F, imgMat, CV_BGRA2BGR);

	int maxLen = maskMat.rows < maskMat.cols ? maskMat.cols : maskMat.rows;
	float scale = 1.0;
	if (maxLen > 400.0) {
		scale = 400.0 / maxLen;
	}

	Mat alpha = KernelEstimation::generateAlpha(imgMat, maskMat, 10 / scale, 1.5, false, !useFIGTree, 16, 8);
	Rect bound = KernelEstimation::getBoundaries(alpha);
	boundMat = Mat(1,2, CV_32FC1);
	float cx = bound.x + bound.width / 2;
	float cy = bound.y + bound.height / 2;
	boundMat.at<float>(0,0) = bound.x + bound.width / 2;
	boundMat.at<float>(0,1) = bound.y + bound.height / 2;
	Mat alphaReady;
	cvtColor(imgMat, alphaReady, CV_BGR2BGRA);
	Mat merged = mergeWithAlpha(alphaReady, alpha);
	circle(alphaReady, Point(cx, cy), 20, Scalar(0.5,0,0,1),2);
	merged.convertTo(merged, CV_8UC4, 255); //Change format
	resMat = merged(bound);
}

JNIEXPORT void JNICALL Java_com_eladrich_scribbler_MainActivity_DoBlurFromMask(
		JNIEnv*, jobject, jlong addrImg, jlong addrMask, jlong addrRes,
		jint radius) {
	Mat& imgMat4U = *(Mat*) addrImg;
	Mat& maskMat = *(Mat*) addrMask;
	Mat& resMat = *(Mat*) addrRes;
	Mat imgMat4F, imgMat;
	imgMat4U.convertTo(imgMat4F, CV_32FC4, 1 / 255.0);
	cvtColor(imgMat4F, imgMat, CV_BGRA2BGR);
	int r_defocus = (int) radius;
	if (r_defocus == 0) {
		imgMat4U.copyTo(resMat);
		return;
		//TODO: maybe just do nothing
	} else if (r_defocus > 0) {
		Mat seg;
		maskMat.convertTo(seg, CV_32FC1, 1 / 255.0);
		seg = 1 - seg;
		resMat = KernelEstimation::blurImage(imgMat, seg, true, r_defocus,
				false);
	} else {
		Mat seg;
		maskMat.convertTo(seg, CV_32FC1, 1 / 255.0);
		resMat = KernelEstimation::blurImage(imgMat, seg, false, -r_defocus,
				false);
	}
	resMat.convertTo(resMat, CV_8UC3, 255); //Fix format
}

JNIEXPORT void JNICALL Java_com_eladrich_scribbler_MainActivity_DoGrayFromMask(
		JNIEnv*, jobject, jlong addrImg, jlong addrMask, jlong addrRes,
		jfloat jIntensity) {
	Mat& imgMat4U = *(Mat*) addrImg;
	Mat& maskMat = *(Mat*) addrMask;
	Mat& resMat = *(Mat*) addrRes;
	Mat imgMat4F, imgMat;
	imgMat4U.convertTo(imgMat4F, CV_32FC4, 1 / 255.0);
	cvtColor(imgMat4F, imgMat, CV_BGRA2BGR);
	float intensity = (float) jIntensity;
	if (intensity == 0) {
		imgMat4U.copyTo(resMat);
		return;
		//TODO: maybe just do nothing
	} else if (intensity > 0) {
		Mat seg = 255 - maskMat;
		resMat = KernelEstimation::grayScaleMat(imgMat, intensity,
				seg);
	} else {
		resMat = KernelEstimation::grayScaleMat(imgMat, -intensity,
				maskMat);
	}
	resMat.convertTo(resMat, CV_8UC3, 255); //Fix format
}

void matToVectors(Mat& scribblesMat, vector<vector<Point> >& scribbles,
		float scale) {
	stringstream sstream;
	cv::Size size = scribblesMat.size();
	size.width *= 2; //Two dimensions
	for (int i = 0; i < size.height; i++) {
		float* ptr = scribblesMat.ptr<float>(i);
		vector<Point> scribble;
		for (int j = 0; j < size.width; j += 2) {
			float x = ptr[j], y = ptr[j + 1];
			if (x == -1 && y == -1) {
				break;
			}

			Point p = Point(int(x * scale), int(y * scale));
			scribble.push_back(p);
			//sstream << "Pushing" << p << "To Scribble " << i;
			//showString(sstream);
		}
		scribbles.push_back(scribble);
	}
}

Mat mergeWithAlpha(Mat & image, Mat & alpha) {
	stringstream sstream;
	sstream << "Taking a " << type2str(image.type())
			<< "Mat, adding alpha from a " << type2str(alpha.type()) << "Mat";
	showString(sstream);
	Mat imageWithAlpha = image.clone();
	for (int i = 0; i < imageWithAlpha.rows; i++) {
		for (int j = 0; j < imageWithAlpha.cols; j++) {
			Vec4f& v = imageWithAlpha.at<Vec4f>(i, j);
			float val = alpha.at<float>(i, j);
			float replacing = v[3];
			v[3] = val;
		}
	}
	return imageWithAlpha;
}

string type2str(int type) {
	string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:
		r = "8U";
		break;
	case CV_8S:
		r = "8S";
		break;
	case CV_16U:
		r = "16U";
		break;
	case CV_16S:
		r = "16S";
		break;
	case CV_32S:
		r = "32S";
		break;
	case CV_32F:
		r = "32F";
		break;
	case CV_64F:
		r = "64F";
		break;
	default:
		r = "User";
		break;
	}

	r += "C";
	r += (chans + '0');

	return r;
}
}
