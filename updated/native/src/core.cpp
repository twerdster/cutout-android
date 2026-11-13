// Updated 2026 wrapper around Elad Richardson's recovered CutOut operators.
#include <cutout/core.hpp>
#include "KernelEstimation.h"
#include "Dijkstra.h"
#include "mw/openCVwithMW.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace cutout {
namespace {
std::mutex algorithm_mutex; // FIGTree/ANN and FLST retain process-global scratch state.
void require(bool value, const char* message) {
    if (!value) throw std::invalid_argument(message);
}
void finite(const cv::Mat& value, const char* name) {
    if (!cv::checkRange(value)) throw std::runtime_error(std::string(name) + " contains nonfinite values");
}
cv::Mat reduced_seeds(const cv::Mat& full, cv::Size size) {
    cv::Mat result(size, CV_8UC1, cv::Scalar(0));
    std::vector<cv::Point> points; cv::findNonZero(full, points);
    for (const auto& p : points) {
        const int x = std::min(size.width-1, int(double(p.x) * size.width / full.cols));
        const int y = std::min(size.height-1, int(double(p.y) * size.height / full.rows));
        result.at<unsigned char>(y,x) = 255;
    }
    return result;
}
}
cv::Mat scribble_mask(cv::Size size, const std::vector<std::vector<cv::Point>>& strokes,
                     int brush_radius) {
    require(size.width > 0 && size.height > 0 && int64_t(size.width) * size.height <= 40000000,
            "image dimensions must be positive and at most 40 million pixels");
    require(brush_radius >= 0 && brush_radius <= 64, "brush radius must be between 0 and 64");
    cv::Mat mask(size, CV_8UC1, cv::Scalar(0));
    for (const auto& stroke : strokes) {
        for (const auto& p : stroke)
            require(p.x >= 0 && p.y >= 0 && p.x < size.width && p.y < size.height,
                    "scribble coordinate is outside image");
        if (stroke.empty()) continue;
        for (const auto& p : stroke) cv::circle(mask, p, brush_radius, cv::Scalar(255), -1, cv::LINE_8);
        for (size_t i=1; i<stroke.size(); ++i)
            cv::line(mask, stroke[i-1], stroke[i], cv::Scalar(255), 2*brush_radius+1, cv::LINE_8);
    }
    return mask;
}
Result segment(const cv::Mat& input, const cv::Mat& fg_input, const cv::Mat& bg_input,
               const Options& options) {
    require(!input.empty() && input.type() == CV_8UC3, "image must be nonempty BGR8 (CV_8UC3)");
    require(int64_t(input.rows) * input.cols <= 40000000, "image exceeds 40 million pixel limit");
    require(fg_input.type() == CV_8UC1 && bg_input.type() == CV_8UC1 &&
            input.size() == fg_input.size() && input.size() == bg_input.size(),
            "scribble masks must be image-sized CV_8UC1");
    require(options.histogram_bins >= 2 && options.histogram_bins <= 64,
            "histogram bins must be between 2 and 64");
    require(options.histogram_blur == 0 || options.histogram_blur == 1 ||
            (options.histogram_blur >= 3 && options.histogram_blur <= 31),
            "histogram blur must be 0, 1, or 3..31");
    require(options.max_dimension == 0 || (options.max_dimension >= 2 && options.max_dimension <= 4096),
            "max dimension must be 0 or between 2 and 4096");
    require(options.alpha_band >= 1 && options.alpha_band <= 32, "alpha band must be between 1 and 32");
    cv::Mat fg = fg_input != 0, bg = bg_input != 0, overlap;
    require(cv::countNonZero(fg) && cv::countNonZero(bg), "both foreground and background scribbles are required");
    cv::bitwise_and(fg, bg, overlap);
    require(cv::countNonZero(overlap) == 0, "foreground and background scribbles overlap");
    const double start = cv::getTickCount();
    cv::Size working = input.size();
    const int longest = std::max(input.cols, input.rows);
    if (options.max_dimension && longest > options.max_dimension) {
        const double scale = double(options.max_dimension)/longest;
        working = cv::Size(std::max(1, int(std::lround(input.cols*scale))),
                           std::max(1, int(std::lround(input.rows*scale))));
    }
    require(!options.use_level_set || (working.width <= 32767 && working.height <= 32767),
            "level-set working dimensions exceed the recovered 16-bit coordinate range; enable reduction");
    // Same-class fine seeds may merge safely; conflicting classes may not.
    cv::Mat small_fg = reduced_seeds(fg,working), small_bg = reduced_seeds(bg,working);
    cv::bitwise_and(small_fg,small_bg,overlap);
    require(cv::countNonZero(overlap) == 0,
            "opposing scribbles collide after reduction; separate scribbles or increase working resolution");
    cv::Mat image;
    input.convertTo(image,CV_32FC3,1.0/255.0);
    if (working != input.size()) cv::resize(image,image,working,0,0,cv::INTER_AREA);
    cv::Mat mask(working,CV_8UC1,cv::Scalar(255));
    std::vector<cv::Point> fp,bp; cv::findNonZero(small_fg,fp);cv::findNonZero(small_bg,bp);
    std::lock_guard<std::mutex> guard(algorithm_mutex);
    cv::Mat fpdf = options.use_figtree ? KernelEstimation::calculatePDF(image,fp,mask) :
        KernelEstimation::simpleCalculatePDF(image,fp,mask,options.histogram_bins,options.histogram_blur);
    cv::Mat bpdf = options.use_figtree ? KernelEstimation::calculatePDF(image,bp,mask) :
        KernelEstimation::simpleCalculatePDF(image,bp,mask,options.histogram_bins,options.histogram_blur);
    finite(fpdf,"foreground PDF"); finite(bpdf,"background PDF");
    Result result;result.working_size=working;
    result.foreground_likelihood=cv::Mat::zeros(working,CV_32FC1);
    cv::Mat back_likelihood=cv::Mat::zeros(working,CV_32FC1);
    KernelEstimation::calculateLikelihood(fpdf,bpdf,result.foreground_likelihood,back_likelihood,mask);
    finite(result.foreground_likelihood,"foreground likelihood");
    if (options.use_level_set) {
        // Preserve the recovered FLST operator and parent/child value differences.
        std::unique_ptr<struct fimage,decltype(&mw_delete_fimage)> mw_image(
            openCVWithMW::matToFimage(result.foreground_likelihood),mw_delete_fimage);
        std::unique_ptr<struct shapes,decltype(&mw_delete_shapes)> tree(mw_new_shapes(),mw_delete_shapes);
        flst(nullptr,mw_image.get(),tree.get());
        result.foreground_distances=openCVWithMW::DijkstraOnTree(tree.get(),fp,working.height,working.width);
        result.background_distances=openCVWithMW::DijkstraOnTree(tree.get(),bp,working.height,working.width);
    } else {
        result.foreground_distances=Dijkstra::simpleComputePaths(result.foreground_likelihood,fp,mask);
        result.background_distances=Dijkstra::simpleComputePaths(result.foreground_likelihood,bp,mask);
    }
    finite(result.foreground_distances,"foreground distances");finite(result.background_distances,"background distances");
    cv::Mat small_mask=KernelEstimation::findObject(result.foreground_distances,result.background_distances,mask);
    if (working != input.size()) {
        cv::resize(small_mask,result.mask,input.size(),0,0,cv::INTER_CUBIC);
        KernelEstimation::roundMask(result.mask);
        if (options.smooth_mask) {
            const int kernel = std::max(1, int(5.0 * longest / options.max_dimension));
            result.mask=KernelEstimation::smoothMask(result.mask,kernel);
            KernelEstimation::roundMask(result.mask);
        }
    } else result.mask=small_mask;
    // Enforce explicit labels, including flat/tied level-set regions.
    result.mask.setTo(255,fg);result.mask.setTo(0,bg);
    if (options.refine_alpha) {
        cv::Mat full_float;input.convertTo(full_float,CV_32FC3,1.0/255.0);
        result.alpha=KernelEstimation::generateAlpha(full_float,result.mask,options.alpha_band,
                    1.5,false,!options.use_figtree,options.histogram_bins,options.histogram_blur);
        finite(result.alpha,"refined alpha");
        cv::max(result.alpha,0,result.alpha);cv::min(result.alpha,1,result.alpha);
        result.alpha.setTo(1,fg);result.alpha.setTo(0,bg);
    } else result.mask.convertTo(result.alpha,CV_32FC1,1.0/255.0);
    cv::cvtColor(input,result.rgba,cv::COLOR_BGR2BGRA);
    cv::Mat alpha_byte;result.alpha.convertTo(alpha_byte,CV_8UC1,255);
    cv::insertChannel(alpha_byte,result.rgba,3);
    result.outline=input.clone();
    std::vector<std::vector<cv::Point>> contours;cv::findContours(result.mask.clone(),contours,cv::RETR_LIST,cv::CHAIN_APPROX_SIMPLE);
    cv::drawContours(result.outline,contours,-1,cv::Scalar(0,255,0),1);
    result.elapsed_seconds=(cv::getTickCount()-start)/cv::getTickFrequency();
    return result;
}
}
