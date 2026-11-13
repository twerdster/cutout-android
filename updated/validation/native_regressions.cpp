// Updated 2026 regression checks by Codex for Aaron Wetzler.
// These checks exercise the restored native components; they are not a
// photographic segmentation benchmark or validation of the historical APK.
#include "Dijkstra.h"
#include "KernelEstimation.h"
#include "mw/openCVwithMW.h"
#include <cutout/core.hpp>
#include "figtree_random.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void near(float actual, float expected, float tolerance, const std::string& message) {
    if (std::fabs(actual - expected) > tolerance || !std::isfinite(actual))
        throw std::runtime_error(message + " (got " + std::to_string(actual) +
                                 ", expected " + std::to_string(expected) + ")");
}
void finite_matrix(const cv::Mat& matrix) {
    require(matrix.type() == CV_32FC1, "expected scalar float matrix");
    for (int y = 0; y < matrix.rows; ++y)
        for (int x = 0; x < matrix.cols; ++x)
            require(std::isfinite(matrix.at<float>(y, x)), "nonfinite matrix entry");
}

void empty_path() {
    require(Dijkstra::fillPath({}).empty(), "empty stroke must remain empty");
    auto result = Dijkstra::fillMultiplePaths({{}, {{1, 2}, {1, 4}}, {}});
    require(result == std::vector<cv::Point>({{1, 2}, {1, 3}, {1, 4}}),
            "empty sentinel rows must not erase a valid stroke");
}
void line_endpoints() {
    auto points = Dijkstra::simpleCompleteLine({0, 4}, {5, 0});
    require(points.front() == cv::Point(0, 4) && points.back() == cv::Point(5, 0),
            "line endpoint lost");
    for (size_t i = 1; i < points.size(); ++i)
        require(std::abs(points[i].x - points[i-1].x) <= 1 &&
                std::abs(points[i].y - points[i-1].y) <= 1, "line gap");
}
void crop_empty() {
    cv::Mat alpha(4, 5, CV_32FC1, cv::Scalar(0));
    require(KernelEstimation::getBoundaries(alpha) == cv::Rect(),
            "empty alpha requires an empty, nonnegative rectangle");
}
void crop_single_pixel() {
    cv::Mat alpha(4, 5, CV_32FC1, cv::Scalar(0));
    alpha.at<float>(3, 4) = 1.f;
    require(KernelEstimation::getBoundaries(alpha) == cv::Rect(4, 3, 1, 1),
            "last-row last-column single pixel must not disappear");
}
void crop_full() {
    cv::Mat alpha(4, 5, CV_32FC1, cv::Scalar(1));
    require(KernelEstimation::getBoundaries(alpha) == cv::Rect(0, 0, 5, 4),
            "crop must include the occupied maximum coordinates");
}
void lab_bins_clamp() {
    cv::Point3f low(-200.f, -1000.f, 1000.f);
    cv::Point3d bins = KernelEstimation::labToRange(low, 8);
    require(bins == cv::Point3d(0, 0, 7), "out-of-domain Lab values not clamped");
    cv::Point3f high(200.f, 1000.f, -1000.f);
    bins = KernelEstimation::labToRange(high, 8);
    require(bins == cv::Point3d(7, 7, 0), "upper/lower Lab bins not clamped");
}
void saturated_histogram() {
    cv::Mat image(7, 9, CV_32FC3);
    const cv::Vec3f colors[] = {{0,0,0}, {1,1,1}, {1,0,0}, {0,1,0}, {0,0,1}};
    for (int y = 0; y < image.rows; ++y)
        for (int x = 0; x < image.cols; ++x)
            image.at<cv::Vec3f>(y,x) = colors[(x + y) % 5];
    cv::Mat mask(image.size(), CV_8UC1, cv::Scalar(255));
    auto pdf = KernelEstimation::simpleCalculatePDF(image, {{0,0},{8,0},{0,6},{8,6}}, mask, 8, 0);
    finite_matrix(pdf);
    double minimum, maximum;
    cv::minMaxLoc(pdf, &minimum, &maximum);
    require(minimum >= 0 && maximum > 0, "histogram PDF invalid");
}
void histogram_roi() {
    cv::Mat parent(9, 13, CV_32FC3);
    for (int y = 0; y < parent.rows; ++y)
        for (int x = 0; x < parent.cols; ++x)
            parent.at<cv::Vec3f>(y,x) = cv::Vec3f(x/13.f, y/9.f, ((x+y)%7)/7.f);
    cv::Mat image = parent(cv::Rect(2, 1, 7, 5));
    cv::Mat mask_parent(parent.size(), CV_8UC1, cv::Scalar(255));
    cv::Mat mask = mask_parent(cv::Rect(2, 1, 7, 5));
    cv::Mat contiguous = image.clone(), contiguous_mask = mask.clone();
    std::vector<cv::Point> seeds{{0,0},{6,4},{3,2}};
    auto roi_pdf = KernelEstimation::simpleCalculatePDF(image, seeds, mask, 8, 1);
    auto contiguous_pdf = KernelEstimation::simpleCalculatePDF(contiguous, seeds, contiguous_mask, 8, 1);
    finite_matrix(roi_pdf); finite_matrix(contiguous_pdf);
    require(cv::norm(roi_pdf, contiguous_pdf, cv::NORM_INF) <= 1e-7,
            "PDF depends on row stride/ROI storage");
}
void pixel_constant() {
    cv::Mat weights(5, 6, CV_32FC1, cv::Scalar(.25));
    cv::Mat mask(weights.size(), CV_8UC1, cv::Scalar(255));
    auto distances = Dijkstra::simpleComputePaths(weights, {{0,0}}, mask);
    for (int y=0; y<weights.rows; ++y)
        for (int x=0; x<weights.cols; ++x)
            near(distances.at<float>(y,x), (x+y)*1e-5f, 2e-10f, "constant-grid Manhattan cost");
}
void pixel_reference() {
    // Independent Bellman-Ford relaxation on a tiny masked four-neighbor grid.
    // This verifies the recovered edge cost |w(p)-w(q)| + 1e-5 and multi-seeds.
    cv::Mat weights(5, 6, CV_32FC1);
    for (int y=0; y<weights.rows; ++y)
        for (int x=0; x<weights.cols; ++x)
            weights.at<float>(y,x) = ((x*17+y*11)%23)/23.f;
    cv::Mat mask(weights.size(), CV_8UC1, cv::Scalar(255));
    mask.at<uchar>(1,2)=0; mask.at<uchar>(2,2)=0; mask.at<uchar>(3,2)=0;
    const std::vector<cv::Point> seeds{{0,0},{5,4}};
    auto actual = Dijkstra::simpleComputePaths(weights, seeds, mask);
    cv::Mat expected(weights.size(), CV_32FC1, cv::Scalar(std::numeric_limits<float>::infinity()));
    for (auto p:seeds) expected.at<float>(p)=0;
    const cv::Point offsets[]={{-1,0},{1,0},{0,-1},{0,1}};
    for (int pass=0; pass<weights.rows*weights.cols; ++pass)
        for (int y=0; y<weights.rows; ++y)
            for (int x=0; x<weights.cols; ++x) {
                cv::Point p(x,y); if (!mask.at<uchar>(p)) continue;
                for(auto offset:offsets) {
                    auto q=p+offset;
                    if(q.x<0||q.y<0||q.x>=weights.cols||q.y>=weights.rows||!mask.at<uchar>(q))continue;
                    float candidate=expected.at<float>(p)+std::fabs(weights.at<float>(p)-weights.at<float>(q))+1e-5f;
                    expected.at<float>(q)=std::min(expected.at<float>(q),candidate);
                }
            }
    for (int y=0; y<weights.rows; ++y)
        for (int x=0; x<weights.cols; ++x)
            if(mask.at<uchar>(y,x)) near(actual.at<float>(y,x),expected.at<float>(y,x),1e-6f,"pixel shortest-path mismatch");
            else require(std::isinf(actual.at<float>(y,x)),"masked pixel became reachable");
}
void pixel_disconnected() {
    cv::Mat weights(5, 6, CV_32FC1, cv::Scalar(.5));
    cv::Mat mask(weights.size(), CV_8UC1, cv::Scalar(255));
    mask.col(3).setTo(0);
    auto distances = Dijkstra::simpleComputePaths(weights, {{0,0}}, mask);
    require(std::isfinite(distances.at<float>(4,2)) && std::isinf(distances.at<float>(4,4)),
            "blocked component must remain unreachable");
}
void alpha_degenerate_distances() {
    const float infinity=std::numeric_limits<float>::infinity();
    cv::Mat front=(cv::Mat_<float>(1,5)<<0,3,0,infinity,3);
    cv::Mat back=(cv::Mat_<float>(1,5)<<3,0,0,infinity,4);
    cv::Mat foreground_pdf(1,5,CV_32FC1,cv::Scalar(.6));
    cv::Mat background_pdf(1,5,CV_32FC1,cv::Scalar(.4));
    cv::Mat band(1,5,CV_8UC1,cv::Scalar(255));
    cv::Mat alpha(1,5,CV_32FC1,cv::Scalar(.5));
    KernelEstimation::updateAlpha(front,back,foreground_pdf,background_pdf,band,alpha,1.5);
    finite_matrix(alpha);
    for(int x=0;x<5;++x) require(alpha.at<float>(0,x)>=0 && alpha.at<float>(0,x)<=1,"alpha outside [0,1]");
    near(alpha.at<float>(0,0),1.f,1e-6,"foreground boundary seed must be opaque");
    near(alpha.at<float>(0,1),0.f,1e-6,"background boundary seed must be transparent");
}
void constant_level_tree() {
    cv::Mat weights(8,9,CV_32FC1,cv::Scalar(.5));
    Fimage image = openCVWithMW::matToFimage(weights);
    Shapes tree = mw_new_shapes();
    flst(nullptr,image,tree);
    require(tree && tree->nb_shapes>=1,"FLST omitted constant root");
    auto distances=openCVWithMW::DijkstraOnTree(tree,{{0,0}},weights.rows,weights.cols);
    mw_delete_shapes(tree);mw_delete_fimage(image);
    finite_matrix(distances);
    require(cv::norm(distances,cv::NORM_INF)==0,"one constant tree must have zero distances");
}
void nested_level_tree(bool nested) {
    const int side=nested?11:9;
    cv::Mat weights(side,side,CV_32FC1,cv::Scalar(0));
    cv::rectangle(weights,nested?cv::Rect(1,1,9,9):cv::Rect(2,2,5,5),cv::Scalar(1),-1);
    if(nested) {
        cv::rectangle(weights,cv::Rect(3,3,5,5),cv::Scalar(0),-1);
        cv::rectangle(weights,cv::Rect(4,4,3,3),cv::Scalar(1),-1);
    } else weights.at<float>(4,4)=0;
    std::unique_ptr<struct fimage,decltype(&mw_delete_fimage)> image(openCVWithMW::matToFimage(weights),mw_delete_fimage);
    std::unique_ptr<struct shapes,decltype(&mw_delete_shapes)> tree(mw_new_shapes(),mw_delete_shapes);
    flst(nullptr,image.get(),tree.get());
    require(tree->nb_shapes==(nested?4:3),"FLST nested-square shape count changed");
    const cv::Point center(side/2,side/2);
    Shape node=mw_get_smallest_shape(tree.get(),center.x,center.y);
    const std::vector<int> areas=nested?std::vector<int>{9,25,81,121}:std::vector<int>{1,25,81};
    const std::vector<float> values=nested?std::vector<float>{1,0,1,0}:std::vector<float>{0,1,0};
    for(size_t i=0;i<areas.size();++i) {
        require(node!=nullptr,"FLST parent chain ended early");
        require(node->area==areas[i],"FLST parent-chain area mismatch");
        near(node->value,values[i],0,"FLST parent-chain level mismatch");
        if(i+1<areas.size()) node=mw_get_parent_shape(node);
    }
    require(node==tree->the_shapes&&node->parent==nullptr,"FLST chain did not reach root");
    auto distances=openCVWithMW::DijkstraOnTree(tree.get(),{center},side,side);
    finite_matrix(distances);
    near(distances.at<float>(center),0,0,"FLST source distance");
    near(distances.at<float>(0,0),nested?3.f:2.f,0,"FLST sum of parent-edge level changes");
}
struct Example {
    cv::Mat image = cv::Mat(24,32,CV_8UC3,cv::Scalar(40,120,30));
    cv::Mat foreground = cv::Mat::zeros(image.size(),CV_8UC1);
    cv::Mat background = cv::Mat::zeros(image.size(),CV_8UC1);
    Example() {
        cv::rectangle(image,cv::Rect(8,5,16,14),cv::Scalar(30,40,220),-1);
        cv::line(foreground,{13,9},{18,14},cv::Scalar(255),1);
        cv::line(background,{2,2},{2,21},cv::Scalar(255),1);
    }
};
void api_operator(bool figtree,bool levelset,bool refined) {
    Example example;
    cutout::Options options;
    options.use_figtree=figtree;options.use_level_set=levelset;options.refine_alpha=refined;
    auto result=cutout::segment(example.image,example.foreground,example.background,options);
    require(result.mask.type()==CV_8UC1 && result.mask.size()==example.image.size(),"mask format/dimensions");
    require(result.rgba.type()==CV_8UC4 && result.rgba.size()==example.image.size(),"BGRA format/dimensions");
    finite_matrix(result.foreground_likelihood);finite_matrix(result.foreground_distances);
    finite_matrix(result.background_distances);finite_matrix(result.alpha);
    require(cv::countNonZero((result.mask!=0)&(result.mask!=255))==0,"nonbinary mask");
    require(cv::countNonZero((result.mask!=255)&example.foreground)==0,"foreground seed disagreement");
    require(cv::countNonZero(result.mask&example.background)==0,"background seed disagreement");
    std::vector<cv::Mat> channels;cv::split(result.rgba,channels);
    cv::Mat alpha_bytes;result.alpha.convertTo(alpha_bytes,CV_8UC1,255);
    require(cv::norm(channels[3],alpha_bytes,cv::NORM_INF)==0,"PNG alpha differs from returned alpha");
    if(!refined) require(cv::norm(channels[3],result.mask,cv::NORM_INF)==0,"hard alpha differs from mask");
    require(std::isfinite(result.elapsed_seconds)&&result.elapsed_seconds>=0,"elapsed time invalid");
}
template<class Function> void invalid(Function function,const std::string& message) {
    try { function(); } catch(const std::invalid_argument&) {return;}
    throw std::runtime_error(message);
}
void api_invalid_seeds() {
    Example example;
    cv::Mat empty=cv::Mat::zeros(example.image.size(),CV_8UC1);
    invalid([&]{cutout::segment(example.image,empty,example.background);},"empty foreground accepted");
    invalid([&]{cutout::segment(example.image,example.foreground,empty);},"empty background accepted");
    invalid([&]{cutout::segment(example.image,example.foreground,example.foreground);},"overlapping labels accepted");
    invalid([&]{cutout::segment(example.image,example.image,example.background);},"multichannel seed mask accepted");
}
void api_invalid_options() {
    Example example;cutout::Options options;
    invalid([]{cutout_figtree::index(0);},"zero random-index range accepted");
    invalid([]{cutout_figtree::index(-1);},"negative random-index range accepted");
    options.histogram_bins=100000;
    invalid([&]{cutout::segment(example.image,example.foreground,example.background,options);},"unbounded histogram allocation accepted");
    options=cutout::Options();options.max_dimension=-1;
    invalid([&]{cutout::segment(example.image,example.foreground,example.background,options);},"negative reduction accepted");
    invalid([&]{cutout::scribble_mask(example.image.size(),{{{-1,0}}},2);},"negative stroke coordinate accepted");
    invalid([&]{cutout::scribble_mask(example.image.size(),{{{32,0}}},2);},"right-outside stroke coordinate accepted");
    invalid([&]{cutout::scribble_mask({65536,65536},{},0);},"pixel-limit guard overflowed before allocation");
    cv::Mat wide(2,32768,CV_8UC3,cv::Scalar(0));
    auto wide_fg=cutout::scribble_mask(wide.size(),{{{0,0}}},0);
    auto wide_bg=cutout::scribble_mask(wide.size(),{{{32767,0}}},0);
    options=cutout::Options();options.use_level_set=true;options.max_dimension=0;
    invalid([&]{cutout::segment(wide,wide_fg,wide_bg,options);},"MegaWave short-coordinate capacity exceeded");
}
void api_reduction_collision() {
    cv::Mat image(64,64,CV_8UC3,cv::Scalar(0));
    cv::Mat fg=cv::Mat::zeros(image.size(),CV_8UC1),bg=fg.clone();
    fg.at<uchar>(1,1)=255;bg.at<uchar>(2,2)=255;
    cutout::Options options;options.max_dimension=2;
    invalid([&]{cutout::segment(image,fg,bg,options);},"opposing seeds merged silently during reduction");
}
void api_reduction_dimensions() {
    cv::Mat image(48,96,CV_8UC3,cv::Scalar(40,120,30));
    cv::rectangle(image,cv::Rect(24,10,48,28),cv::Scalar(30,40,220),-1);
    auto fg=cutout::scribble_mask(image.size(),{{{42,20},{54,26}}},1);
    auto bg=cutout::scribble_mask(image.size(),{{{4,4},{4,42}}},1);
    cutout::Options options;options.max_dimension=32;options.use_figtree=false;
    auto result=cutout::segment(image,fg,bg,options);
    require(result.working_size==cv::Size(32,16),"aspect-ratio reduction changed");
    require(result.mask.size()==image.size()&&result.rgba.size()==image.size(),"reduced output not restored to full size");
    require(cv::countNonZero((result.mask!=255)&fg)==0&&cv::countNonZero(result.mask&bg)==0,"full-resolution labels lost in resize/smoothing");
}
void api_thin_images() {
    // Level-set trees with no child shapes and one spatial dimension must not
    // dereference a nonexistent neighbor or divide by an absent tree edge.
    for(auto size:{cv::Size(9,1),cv::Size(1,9),cv::Size(257,1),cv::Size(1,257)}) {
        cv::Mat image(size,CV_8UC3,cv::Scalar(100,100,100));
        auto fg=cutout::scribble_mask(size,{{{0,0}}},0);
        auto bg=cutout::scribble_mask(size,{{{size.width-1,size.height-1}}},0);
        for(bool figtree:{false,true}) {
            cutout::Options options;options.use_level_set=true;options.use_figtree=figtree;options.max_dimension=32;
            auto result=cutout::segment(image,fg,bg,options);
            finite_matrix(result.foreground_distances);finite_matrix(result.background_distances);finite_matrix(result.alpha);
            require(result.mask.size()==size,"thin input dimensions lost");
            require(result.mask.at<uchar>(0,0)==255&&result.mask.at<uchar>(size.height-1,size.width-1)==0,"thin input seeds lost");
        }
    }
    cv::Mat image(1,1,CV_8UC3,cv::Scalar(0)),seed(1,1,CV_8UC1,cv::Scalar(255));
    invalid([&]{cutout::segment(image,seed,seed);},"one-pixel image accepted opposing labels");
}
void api_noncontiguous_input() {
    Example example;
    cv::Mat image_parent(30,40,CV_8UC3,cv::Scalar(0)),fg_parent(30,40,CV_8UC1,cv::Scalar(0)),bg_parent=fg_parent.clone();
    const cv::Rect rectangle(3,2,32,24);
    cv::Mat image=image_parent(rectangle),fg=fg_parent(rectangle),bg=bg_parent(rectangle);
    example.image.copyTo(image);example.foreground.copyTo(fg);example.background.copyTo(bg);
    require(!image.isContinuous()&&!fg.isContinuous(),"test failed to construct a strided ROI");
    for(bool figtree:{false,true})for(bool levelset:{false,true}) {
        cutout::Options options;options.use_figtree=figtree;options.use_level_set=levelset;options.refine_alpha=true;
        auto strided=cutout::segment(image,fg,bg,options);
        auto packed=cutout::segment(example.image,example.foreground,example.background,options);
        require(cv::norm(strided.mask,packed.mask,cv::NORM_INF)==0,"mask depends on input row stride");
        require(cv::norm(strided.alpha,packed.alpha,cv::NORM_INF)<=1e-6,"alpha depends on input row stride");
    }
}
void api_small_random_grids() {
    // Deterministic small stress cases vary colors, borders, shapes and contour
    // structure. Finite output and seed constraints are invariants, not accuracy.
    cv::RNG generator(20261113);
    for(int example=0;example<10;++example) {
        const cv::Size size(6+example,7+(example*3)%10);
        cv::Mat image(size,CV_8UC3);generator.fill(image,cv::RNG::UNIFORM,0,256);
        auto fg=cutout::scribble_mask(size,{{{1,1}}},0);
        auto bg=cutout::scribble_mask(size,{{{size.width-2,size.height-2}}},0);
        for(bool figtree:{false,true})for(bool levelset:{false,true}) {
            cutout::Options options;options.use_figtree=figtree;options.use_level_set=levelset;options.refine_alpha=true;
            auto result=cutout::segment(image,fg,bg,options);
            finite_matrix(result.foreground_likelihood);finite_matrix(result.foreground_distances);
            finite_matrix(result.background_distances);finite_matrix(result.alpha);
            require(result.mask.at<uchar>(1,1)==255&&result.mask.at<uchar>(size.height-2,size.width-2)==0,"random grid seed violated");
            double minimum,maximum;cv::minMaxLoc(result.alpha,&minimum,&maximum);
            require(minimum>=0&&maximum<=1,"random grid alpha outside [0,1]");
        }
    }
}
void api_repeatable_density() {
    Example example;
    for(bool figtree:{false,true})for(bool levelset:{false,true}) {
        cutout::Options options;options.use_figtree=figtree;options.use_level_set=levelset;
        auto first=cutout::segment(example.image,example.foreground,example.background,options);
        auto repeat=cutout::segment(example.image,example.foreground,example.background,options);
        options.refine_alpha=true;
        auto refined=cutout::segment(example.image,example.foreground,example.background,options);
        require(cv::norm(first.foreground_likelihood,repeat.foreground_likelihood,cv::NORM_INF)==0,
                "repeated density evaluation changed its likelihood field");
        require(cv::norm(first.mask,repeat.mask,cv::NORM_INF)==0,"repeated segmentation changed its binary mask");
        require(cv::norm(first.mask,refined.mask,cv::NORM_INF)==0,"alpha refinement changed the binary mask");
    }
}
}

int main(int argc,char**argv) {
    const std::vector<std::pair<std::string,std::function<void()>>> tests={
      {"empty_path",empty_path},{"line_endpoints",line_endpoints},
      {"crop_empty",crop_empty},{"crop_single_pixel",crop_single_pixel},{"crop_full",crop_full},
      {"lab_bins_clamp",lab_bins_clamp},{"saturated_histogram",saturated_histogram},{"histogram_roi",histogram_roi},
      {"pixel_constant",pixel_constant},{"pixel_reference",pixel_reference},{"pixel_disconnected",pixel_disconnected},
      {"alpha_degenerate_distances",alpha_degenerate_distances},{"constant_level_tree",constant_level_tree},
      {"ring_level_tree",[]{nested_level_tree(false);}},{"nested_level_tree",[]{nested_level_tree(true);}},
      {"api_histogram_pixel_hard",[]{api_operator(false,false,false);}},
      {"api_histogram_levelset_hard",[]{api_operator(false,true,false);}},
      {"api_figtree_pixel_hard",[]{api_operator(true,false,false);}},
      {"api_figtree_levelset_hard",[]{api_operator(true,true,false);}},
      {"api_histogram_pixel_refined",[]{api_operator(false,false,true);}},
      {"api_histogram_levelset_refined",[]{api_operator(false,true,true);}},
      {"api_figtree_pixel_refined",[]{api_operator(true,false,true);}},
      {"api_figtree_levelset_refined",[]{api_operator(true,true,true);}},
      {"api_invalid_seeds",api_invalid_seeds},{"api_invalid_options",api_invalid_options},
      {"api_reduction_collision",api_reduction_collision},{"api_reduction_dimensions",api_reduction_dimensions},
      {"api_thin_images",api_thin_images},{"api_noncontiguous_input",api_noncontiguous_input},
      {"api_small_random_grids",api_small_random_grids},{"api_repeatable_density",api_repeatable_density}};
    int passed=0,failed=0;
    for(const auto& entry:tests) {
      if(argc>1 && entry.first!=argv[1])continue;
      try {entry.second();++passed;std::cout<<"PASS "<<entry.first<<"\n";}
      catch(const std::exception& error){++failed;std::cerr<<"FAIL "<<entry.first<<": "<<error.what()<<"\n";}
    }
    std::cout<<passed<<" passed, "<<failed<<" failed\n";
    return failed || passed==0 ? 1 : 0;
}
