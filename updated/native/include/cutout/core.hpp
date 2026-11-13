// Updated CutOut interface, 2026. Recovered CutOut implementation: Elad Richardson.
#pragma once
#include <opencv2/core.hpp>
#include <vector>

namespace cutout {
struct Options {
    bool use_figtree = true;
    bool use_level_set = false;
    int histogram_bins = 16;
    int histogram_blur = 8;
    int max_dimension = 400; // 0 disables reduction; positive values must be >= 2.
    bool smooth_mask = true;
    bool refine_alpha = false;
    int alpha_band = 3;
};
struct Result {
    cv::Mat mask; // Full input dimensions, CV_8UC1, 0 or 255.
    cv::Mat rgba; // Full input dimensions, CV_8UC4, OpenCV BGRA byte order.
    cv::Mat outline; // CV_8UC3 BGR, full dimensions.
    cv::Mat foreground_likelihood; // CV_32FC1, working resolution.
    cv::Mat foreground_distances, background_distances; // CV_32FC1, working resolution.
    cv::Mat alpha; // CV_32FC1 in [0,1], full dimensions.
    cv::Size working_size;
    double elapsed_seconds = 0;
};
// BGR 8-bit image and same-size 8-bit single-channel nonzero scribble masks.
// Rejects empty or overlapping seed sets. Validates all options before allocation.
// Internally serializes legacy FIGTree/MegaWave global state for thread safety.
Result segment(const cv::Mat& image_bgr, const cv::Mat& foreground_mask,
               const cv::Mat& background_mask, const Options& options = {});
// Rasterizes original-resolution xy strokes; isolated points are supported.
cv::Mat scribble_mask(cv::Size size, const std::vector<std::vector<cv::Point>>& strokes,
                     int brush_radius = 2);
}
