// New UIKit bridge (2026). Recovered algorithm authorship remains in updated/native.
#include <cutout/core.hpp>
#include <cmath>
#include <algorithm>
#include <opencv2/imgproc.hpp>
#import "CutoutBridge.h"
#import <QuartzCore/QuartzCore.h>

@implementation CutoutRun
@end

// The shared core preserves straight color channels and inserts alpha separately.
// Return a copying data provider so no UIImage refers to a released cv::Mat buffer.
static UIImage *imageFromRGBA(const cv::Mat& rgba) {
    if(rgba.empty()||rgba.type()!=CV_8UC4)throw std::runtime_error("Invalid RGBA output buffer");
    cv::Mat packed=rgba.isContinuous()?rgba:rgba.clone();
    NSData *bytes=[NSData dataWithBytes:packed.data length:packed.total()*packed.elemSize()];
    if(!bytes)throw std::runtime_error("Cannot allocate output image data");
    CGColorSpaceRef space=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    if(!space)throw std::runtime_error("Cannot create output color space");
    CGDataProviderRef provider=CGDataProviderCreateWithCFData((__bridge CFDataRef)bytes);
    if(!provider){CGColorSpaceRelease(space);throw std::runtime_error("Cannot create output data provider");}
    CGImageRef image=CGImageCreate(packed.cols,packed.rows,8,32,packed.cols*4,space,kCGImageAlphaLast|kCGBitmapByteOrderDefault,provider,nullptr,false,kCGRenderingIntentDefault);
    CGColorSpaceRelease(space);CGDataProviderRelease(provider);
    if(!image)throw std::runtime_error("Cannot create output CGImage");
    UIImage *result=[UIImage imageWithCGImage:image scale:1 orientation:UIImageOrientationUp];
    CGImageRelease(image);
    if(!result)throw std::runtime_error("Cannot create output UIImage");
    return result;
}
static UIImage *imageFromMask(const cv::Mat& mask) {
    cv::Mat rgba;cv::cvtColor(mask,rgba,cv::COLOR_GRAY2RGBA);return imageFromRGBA(rgba);
}

@implementation CutoutBridge
+ (CutoutRun *)segment:(UIImage *)image strokes:(NSArray<NSDictionary *> *)strokes options:(NSDictionary *)options error:(NSError **)error {
    try {
        double started=CACurrentMediaTime();
        if(!image||!image.CGImage)throw std::invalid_argument("A decoded image is required");
        if(image.scale!=1)throw std::invalid_argument("Normalize the image scale to 1 before segmentation");
        if(image.imageOrientation!=UIImageOrientationUp)throw std::invalid_argument("Normalize the image orientation before segmentation");
        int width=(int)CGImageGetWidth(image.CGImage),height=(int)CGImageGetHeight(image.CGImage);
        if(width<1||height<1||width>1600||height>1600)throw std::invalid_argument("Image dimensions must be 1–1600 pixels");
        cv::Mat rgba(height,width,CV_8UC4),bgr;
        CGColorSpaceRef space=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        if(!space)throw std::runtime_error("Cannot create input color space");
        CGContextRef context=CGBitmapContextCreate(rgba.data,width,height,8,width*4,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
        CGColorSpaceRelease(space);
        if(!context)throw std::runtime_error("Cannot create image conversion context");
        // Native inputs are opaque BGR; source transparency is composited over white.
        CGContextSetRGBFillColor(context,1,1,1,1);CGContextFillRect(context,CGRectMake(0,0,width,height));
        CGContextDrawImage(context,CGRectMake(0,0,width,height),image.CGImage);CGContextRelease(context);
        cv::cvtColor(rgba,bgr,cv::COLOR_RGBA2BGR);
        cv::Mat fg=cv::Mat::zeros(height,width,CV_8UC1),bg=fg.clone();
        // App-side ordered strokes: a new label erases the opposite label it covers.
        for(NSDictionary *stroke in strokes) {
            bool foreground=[stroke[@"foreground"] boolValue];NSArray *points=stroke[@"points"];
            cv::Mat& active=foreground?fg:bg;cv::Mat& opposite=foreground?bg:fg;cv::Point previous;
            for(NSUInteger i=0;i<points.count;i++) {
                NSArray *xy=points[i];if(xy.count!=2)throw std::invalid_argument("Each stroke point requires x and y");
                double x=[xy[0] doubleValue],y=[xy[1] doubleValue];
                if(!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=width||y>=height)throw std::invalid_argument("Stroke coordinates must be inside the image");
                cv::Point point((int)std::lround(x),(int)std::lround(y));point.x=std::min(width-1,point.x);point.y=std::min(height-1,point.y);
                if(i==0){cv::circle(active,point,2,255,-1);cv::circle(opposite,point,2,0,-1);}
                else{cv::line(active,previous,point,255,5);cv::line(opposite,previous,point,0,5);}previous=point;
            }
        }
        if(!cv::countNonZero(fg)||!cv::countNonZero(bg))throw std::invalid_argument("Both foreground and background scribbles are required");
        cutout::Options config;config.use_figtree=[options[@"figtree"] boolValue];config.use_level_set=[options[@"level_set"] boolValue];config.refine_alpha=[options[@"refine_alpha"] boolValue];config.max_dimension=[options[@"max_dimension"] intValue];
        auto native=cutout::segment(bgr,fg,bg,config);cv::Mat output;cv::cvtColor(native.rgba,output,cv::COLOR_BGRA2RGBA);
        CutoutRun *run=[CutoutRun new];run.input=imageFromRGBA(rgba);run.cutout=imageFromRGBA(output);run.mask=imageFromMask(native.mask);run.foreground=imageFromMask(fg);run.background=imageFromMask(bg);
        run.metrics=@{@"width":@(width),@"height":@(height),@"figtree":@(config.use_figtree),@"level_set":@(config.use_level_set),@"refine_alpha":@(config.refine_alpha),@"max_dimension":@(config.max_dimension),@"histogram_bins":@(config.histogram_bins),@"histogram_blur":@(config.histogram_blur),@"smooth_mask":@(config.smooth_mask),@"alpha_band":@(config.alpha_band),@"selected_pixels":@(cv::countNonZero(native.mask)),@"elapsed_ms":@((CACurrentMediaTime()-started)*1000),@"opencv_version":[NSString stringWithUTF8String:cv::getVersionString().c_str()],@"working_width":@(native.working_size.width),@"working_height":@(native.working_size.height),@"input_color_space":@"sRGB",@"input_alpha":@"composited over white",@"cutout_alpha":@"straight",@"seed_rasterization":@"app-side ordered OpenCV radius-2 first circles / thickness-5 lines; later label erases the opposite"};
        return run;
    }catch(const std::exception& problem){if(error)*error=[NSError errorWithDomain:@"CutOut.Native" code:1 userInfo:@{NSLocalizedDescriptionKey:[NSString stringWithUTF8String:problem.what()]}];return nil;}
    catch(...){if(error)*error=[NSError errorWithDomain:@"CutOut.Native" code:2 userInfo:@{NSLocalizedDescriptionKey:@"Unexpected native segmentation failure"}];return nil;}
}
@end
