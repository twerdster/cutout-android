// Updated 2026 command-line runner for the recovered CutOut operator chain.
#include <cutout/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
void help() {
    std::cout << "CutOut updated native runner (2026)\n"
        "cutout_cli --image image.png --scribbles strokes.json --output-dir output\n"
        "  [--density figtree|histogram] [--distance pixel|levelset]\n"
        "  [--alpha hard|refined] [--alpha-band 3] [--max-dimension 400] [--no-smooth]\n"
        "  [--histogram-bins 16] [--histogram-blur 8]\n"
        "Alternative seeds: --foreground foreground.png --background background.png\n"
        "JSON: {\"foreground\":[[[x,y],...],...],\"background\":[...],\"brush_radius\":2}\n"
        "Coordinates refer to the full input image. PNG masks: nonzero means a seed.\n"
        "Outputs: mask.png, cutout.png (BGRA/PNG), outline.png, alpha.png,\n"
        "foreground_likelihood.png and result.json. Default alpha is a hard binary mask.\n";
}
int integer(const std::string& value, const char* name) {
    try {
        size_t consumed=0;int parsed=std::stoi(value,&consumed);
        if (consumed==value.size()) return parsed;
    } catch (const std::exception&) { }
    throw std::invalid_argument(std::string(name)+" must be a representable integer");
}
std::vector<std::vector<cv::Point>> strokes(const cv::FileNode& node, cv::Size size) {
    if (!node.isSeq()) throw std::invalid_argument("foreground/background must be arrays of strokes");
    std::vector<std::vector<cv::Point>> result;
    size_t count=0;
    for (const auto& stroke:node) {
        if (!stroke.isSeq()) throw std::invalid_argument("a stroke must be an array of [x,y] points");
        std::vector<cv::Point> points;
        for (const auto& point:stroke) {
            if (!point.isSeq() || point.size()!=2) throw std::invalid_argument("each scribble point must contain x and y");
            if ((!point[0].isInt() && !point[0].isReal()) || (!point[1].isInt() && !point[1].isReal()))
                throw std::invalid_argument("scribble x and y values must be numeric integers");
            double x=double(point[0]),y=double(point[1]);
            if (!std::isfinite(x)||!std::isfinite(y)||x!=std::floor(x)||y!=std::floor(y)||
                x<0||y<0||x>=size.width||y>=size.height)
                throw std::invalid_argument("scribble points must be integer xy coordinates inside the image");
            if (++count>1000000) throw std::invalid_argument("too many scribble points");
            points.emplace_back(int(x),int(y));
        }
        result.push_back(std::move(points));
    }
    return result;
}
void save(const std::filesystem::path& path,const cv::Mat& image) {
    if (!cv::imwrite(path.string(),image)) throw std::runtime_error("could not write "+path.string());
}
}
int main(int argc,char** argv) {
    try {
        const std::map<std::string,bool> valid={{"--image",true},{"--scribbles",true},{"--output-dir",true},
            {"--density",true},{"--distance",true},{"--alpha",true},{"--alpha-band",true},
            {"--max-dimension",true},{"--no-smooth",false},{"--foreground",true},{"--background",true},
            {"--histogram-bins",true},{"--histogram-blur",true}};
        std::map<std::string,std::string> args;
        for (int i=1;i<argc;++i) {
            std::string key=argv[i];
            if (key=="--help"||key=="-h") {help();return 0;}
            auto found=valid.find(key);
            if (found==valid.end()) throw std::invalid_argument("unknown option: "+key);
            if (args.count(key)) throw std::invalid_argument("duplicate option: "+key);
            if (found->second) {
                if (i+1>=argc) throw std::invalid_argument("missing value for "+key);
                args[key]=argv[++i];
            } else args[key]="true";
        }
        if (!args.count("--image")||!args.count("--output-dir")) {help();return 2;}
        const bool json=args.count("--scribbles");
        if (json == bool(args.count("--foreground")||args.count("--background")))
            throw std::invalid_argument("choose --scribbles OR both --foreground and --background");
        if (!json && (!args.count("--foreground")||!args.count("--background")))
            throw std::invalid_argument("both PNG scribble masks are required");
        cutout::Options options;
        auto choice=[&](const char* key,const char* first,const char* second,bool& flag) {
            if (!args.count(key)) return;
            if (args[key]!=first && args[key]!=second) throw std::invalid_argument(std::string(key)+" expects "+first+" or "+second);
            flag=args[key]==first;
        };
        choice("--density","figtree","histogram",options.use_figtree);
        choice("--distance","levelset","pixel",options.use_level_set);
        choice("--alpha","refined","hard",options.refine_alpha);
        if (args.count("--max-dimension")) options.max_dimension=integer(args["--max-dimension"],"max dimension");
        if (args.count("--histogram-bins")) options.histogram_bins=integer(args["--histogram-bins"],"histogram bins");
        if (args.count("--histogram-blur")) options.histogram_blur=integer(args["--histogram-blur"],"histogram blur");
        if (args.count("--alpha-band")) options.alpha_band=integer(args["--alpha-band"],"alpha band");
        options.smooth_mask=!args.count("--no-smooth");
        cv::Mat image=cv::imread(args["--image"],cv::IMREAD_COLOR);
        if (image.empty()) throw std::invalid_argument("could not decode input image");
        cv::Mat fg,bg;
        if (json) {
            cv::FileStorage file(args["--scribbles"],cv::FileStorage::READ|cv::FileStorage::FORMAT_JSON);
            if (!file.isOpened()) throw std::invalid_argument("could not read scribbles JSON");
            int radius=2;
            if (!file["brush_radius"].empty()) {
                if (!file["brush_radius"].isInt() && !file["brush_radius"].isReal())
                    throw std::invalid_argument("brush_radius must be numeric");
                const double value=double(file["brush_radius"]);
                if (!std::isfinite(value)||value!=std::floor(value)||value<0||value>64)
                    throw std::invalid_argument("brush_radius must be an integer between 0 and 64");
                radius=int(value);
            }
            fg=cutout::scribble_mask(image.size(),strokes(file["foreground"],image.size()),radius);
            bg=cutout::scribble_mask(image.size(),strokes(file["background"],image.size()),radius);
        } else {
            fg=cv::imread(args["--foreground"],cv::IMREAD_GRAYSCALE);
            bg=cv::imread(args["--background"],cv::IMREAD_GRAYSCALE);
            if (fg.empty()) throw std::invalid_argument("could not decode foreground scribble mask");
            if (bg.empty()) throw std::invalid_argument("could not decode background scribble mask");
        }
        auto result=cutout::segment(image,fg,bg,options);
        const std::filesystem::path output=args["--output-dir"];
        std::filesystem::create_directories(output);
        save(output/"mask.png",result.mask);save(output/"cutout.png",result.rgba);save(output/"outline.png",result.outline);
        cv::Mat likelihood,alpha;result.foreground_likelihood.convertTo(likelihood,CV_8UC1,255);
        result.alpha.convertTo(alpha,CV_8UC1,255);save(output/"alpha.png",alpha);save(output/"foreground_likelihood.png",likelihood);
        cv::FileStorage report((output/"result.json").string(),cv::FileStorage::WRITE|cv::FileStorage::FORMAT_JSON);
        report<<"implementation"<<"Updated 2026 port of recovered CutOut Android native operators"
            <<"original_implementation_author"<<"Elad Richardson"
            <<"density"<<(options.use_figtree?"figtree":"histogram")
            <<"distance"<<(options.use_level_set?"levelset":"pixel")
            <<"alpha"<<(options.refine_alpha?"refined":"hard")
            <<"width"<<image.cols<<"height"<<image.rows
            <<"working_width"<<result.working_size.width<<"working_height"<<result.working_size.height
            <<"foreground_pixels"<<cv::countNonZero(result.mask)
            <<"elapsed_seconds"<<result.elapsed_seconds<<"opencv_version"<<cv::getVersionString()
            <<"opencv_header_version"<<CV_VERSION;
        report.release();
        std::cout<<(output/"result.json").string()<<'\n';return 0;
    } catch (const std::exception& error) {
        std::cerr<<"CutOut: "<<error.what()<<'\n';return 2;
    }
}
