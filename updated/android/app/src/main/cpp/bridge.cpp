// New Android adapter, 2026. Recovered algorithms live in updated/native.
#include <jni.h>
#include <cutout/core.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdint>
#include <stdexcept>
#include <vector>

extern "C" JNIEXPORT jintArray JNICALL
Java_org_awetzler_cutout_updated_NativeBridge_segment(JNIEnv* env, jclass,
        jintArray pixels, jint width, jint height, jbyteArray fg, jbyteArray bg,
        jboolean figtree, jboolean level_set, jboolean refine_alpha, jint max_dimension) {
    try {
        if (width < 1 || height < 1 || width > 1600 || height > 1600)
            throw std::invalid_argument("Image dimensions must be 1–1600 pixels");
        const int count = width * height;
        if (!pixels || !fg || !bg || env->GetArrayLength(pixels) != count ||
                env->GetArrayLength(fg) != count || env->GetArrayLength(bg) != count)
            throw std::invalid_argument("Input image and scribble lengths disagree");
        std::vector<jint> argb(count);
        env->GetIntArrayRegion(pixels, 0, count, argb.data());
        cv::Mat bgr(height, width, CV_8UC3), foreground(height, width, CV_8UC1), background(height, width, CV_8UC1);
        env->GetByteArrayRegion(fg, 0, count, reinterpret_cast<jbyte*>(foreground.data));
        env->GetByteArrayRegion(bg, 0, count, reinterpret_cast<jbyte*>(background.data));
        if (env->ExceptionCheck()) return nullptr;
        for (int i = 0; i < count; ++i) {
            const uint32_t value = static_cast<uint32_t>(argb[i]);
            bgr.ptr<cv::Vec3b>()[i] = cv::Vec3b(value & 255, (value >> 8) & 255, (value >> 16) & 255);
        }
        cutout::Options options;
        options.use_figtree = figtree;
        options.use_level_set = level_set;
        options.refine_alpha = refine_alpha;
        options.max_dimension = max_dimension;
        const auto result = cutout::segment(bgr, foreground, background, options);
        argb.resize(2 * count);
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            const int i = y * width + x;
            const auto pixel = result.rgba.at<cv::Vec4b>(y, x);
            argb[count + i] = result.mask.at<uint8_t>(y, x);
            argb[i] = static_cast<jint>((static_cast<uint32_t>(pixel[3]) << 24) |
                        (static_cast<uint32_t>(pixel[2]) << 16) |
                        (static_cast<uint32_t>(pixel[1]) << 8) | pixel[0]);
        }
        auto out = env->NewIntArray(2 * count);
        if (out) env->SetIntArrayRegion(out, 0, 2 * count, argb.data());
        return out;
    } catch (const std::invalid_argument& error) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), error.what());
        return nullptr;
    } catch (const std::exception& error) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), error.what());
        return nullptr;
    } catch (...) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), "Unexpected native segmentation failure");
        return nullptr;
    }
}
