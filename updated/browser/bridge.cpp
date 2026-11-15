// Updated 2026 browser bridge. Shared recovered operators retain their credits.
#include <cutout/core.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <exception>
#include <cstring>
#include <cstdint>
#include <stdexcept>
namespace { std::string last_error; }
extern "C" {
const char* cutout_error() { return last_error.c_str(); }
int cutout_segment(const unsigned char* rgba, const unsigned char* labels,
                   int width, int height, int figtree, int levelset,
                   int refine, int max_dimension, unsigned char* output,
                   unsigned char* mask_output) {
    try {
        last_error.clear();
        if (width < 1 || height < 1 || int64_t(width)*height > 1000000)
            throw std::invalid_argument("Browser input must have 1..1,000,000 pixels");
        if (!rgba || !labels || !output || !mask_output)
            throw std::invalid_argument("Missing input or output buffer");
        if (max_dimension != 400 && max_dimension != 800)
            throw std::invalid_argument("Browser working limit must be 400 or 800 pixels");
        cv::Mat input_rgba(height,width,CV_8UC4,const_cast<unsigned char*>(rgba));
        cv::Mat label(height,width,CV_8UC1,const_cast<unsigned char*>(labels));
        cv::Mat image; cv::cvtColor(input_rgba,image,cv::COLOR_RGBA2BGR);
        cutout::Options options;
        options.use_figtree = figtree != 0;
        options.use_level_set = levelset != 0;
        options.refine_alpha = refine != 0;
        options.max_dimension = max_dimension;
        const auto result=cutout::segment(image,label==1,label==2,options);
        if (result.rgba.size()!=image.size() || result.rgba.type()!=CV_8UC4 ||
            result.mask.size()!=image.size() || result.mask.type()!=CV_8UC1)
            throw std::runtime_error("Native result has an unexpected size or type");
        cv::Mat result_rgba; cv::cvtColor(result.rgba,result_rgba,cv::COLOR_BGRA2RGBA);
        for(int r=0;r<height;++r) {
            std::memcpy(output+size_t(r)*width*4,result_rgba.ptr(r),size_t(width)*4);
            std::memcpy(mask_output+size_t(r)*width,result.mask.ptr(r),size_t(width));
        }
        return 0;
    } catch(const std::exception& error) { last_error=error.what(); }
      catch(...) { last_error="Segmentation failed in the native operator"; }
    return 1;
}
}
