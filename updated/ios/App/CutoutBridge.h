#import <UIKit/UIKit.h>
NS_ASSUME_NONNULL_BEGIN
@interface CutoutRun : NSObject
// Exact opaque sRGB pixel buffer used by native segmentation, returned as a UIImage.
@property(nonatomic,strong) UIImage *input;
@property(nonatomic,strong) UIImage *cutout;
@property(nonatomic,strong) UIImage *mask;
@property(nonatomic,strong) UIImage *foreground;
@property(nonatomic,strong) UIImage *background;
@property(nonatomic,strong) NSDictionary *metrics;
@end
@interface CutoutBridge : NSObject
+ (nullable CutoutRun *)segment:(UIImage *)image strokes:(NSArray<NSDictionary *> *)strokes options:(NSDictionary *)options error:(NSError **)error;
@end
NS_ASSUME_NONNULL_END
