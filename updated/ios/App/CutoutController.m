// New UIKit interface, 2026. Recovered and third-party authorship is in Credits.
#import "CutoutController.h"
#import "CutoutBridge.h"
#import <PhotosUI/PhotosUI.h>
#import <ImageIO/ImageIO.h>
#include <sys/stat.h>
#include <errno.h>

static NSError *CutoutError(NSInteger code, NSString *message) {
    return [NSError errorWithDomain:@"CutOut.Interface" code:code userInfo:@{NSLocalizedDescriptionKey:message}];
}
static NSString *ReadLicenseText(NSString *path) {
    NSString *text=[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
    if(!text)text=[NSString stringWithContentsOfFile:path usedEncoding:NULL error:nil];
    if(!text)text=[NSString stringWithContentsOfFile:path encoding:NSISOLatin1StringEncoding error:nil];
    return text?:@"Cannot read license text.";
}

// devicectl may pass a literal "--" through to argv. Parse only this app's
// documented options instead of relying on NSUserDefaults' argument domain.
static NSDictionary<NSString *,NSString *> *CutoutLaunchOptions(void) {
    NSSet *keys=[NSSet setWithArray:@[@"run_batch",@"run_fixture",@"example",@"figtree",@"level_set",@"refine_alpha",@"max_dimension"]];
    NSArray<NSString *> *argv=NSProcessInfo.processInfo.arguments;
    NSMutableDictionary *values=[NSMutableDictionary new];
    for(NSUInteger i=1;i<argv.count;i++) {
        NSString *argument=argv[i];
        if([argument isEqualToString:@"--"]||![argument hasPrefix:@"-"])continue;
        NSString *key=[[argument stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"-"]] stringByReplacingOccurrencesOfString:@"-" withString:@"_"];
        if(![keys containsObject:key])continue;
        NSString *value=@"YES";
        if(i+1<argv.count&&(![argv[i+1] hasPrefix:@"-"]||[key isEqualToString:@"max_dimension"]))value=argv[++i];
        values[key]=value;
    }
    return values;
}

@interface TextController : UIViewController
@property(nonatomic,copy) NSString *body;
@end
@implementation TextController
- (void)viewDidLoad { [super viewDidLoad];self.view.backgroundColor=UIColor.systemBackgroundColor;UITextView *text=[UITextView new];text.editable=NO;text.text=self.body;text.font=[UIFont systemFontOfSize:15];text.accessibilityIdentifier=@"cutout.termsText";text.translatesAutoresizingMaskIntoConstraints=NO;[self.view addSubview:text];[NSLayoutConstraint activateConstraints:@[[text.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:12],[text.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-12],[text.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],[text.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor]]]; }
@end
@interface LicenseController : UITableViewController
@property(nonatomic,strong) NSArray<NSString *> *paths;
@end
@implementation LicenseController
- (void)viewDidLoad { [super viewDidLoad];self.title=@"License texts";NSString *base=[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"Licenses"];NSMutableArray *files=[NSMutableArray new];for(NSString *path in [[NSFileManager defaultManager] enumeratorAtPath:base]){BOOL directory=NO;[[NSFileManager defaultManager] fileExistsAtPath:[base stringByAppendingPathComponent:path] isDirectory:&directory];if(!directory)[files addObject:path];}self.paths=[files sortedArrayUsingSelector:@selector(compare:)]; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section{return self.paths.count;}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {UITableViewCell *cell=[[UITableViewCell alloc]initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];cell.textLabel.text=self.paths[path.row];cell.textLabel.numberOfLines=2;return cell;}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)index {TextController *text=[TextController new];text.title=self.paths[index.row];NSString *path=[[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"Licenses"] stringByAppendingPathComponent:self.paths[index.row]];text.body=ReadLicenseText(path);[self.navigationController pushViewController:text animated:YES];}
@end

@interface ImageCanvas : UIView
@property(nonatomic,strong) UIImage *image,*result;
@property(nonatomic,strong) NSMutableArray<NSMutableDictionary *> *strokes;
@property(nonatomic,strong) NSMutableDictionary *activeStroke;
@property(nonatomic) BOOL foreground,showResult,savedResult;
@property(nonatomic) CGSize workingSize;
@property(nonatomic,copy) void (^changed)(void);
- (void)setSource:(UIImage *)image strokes:(NSArray *)strokes;
- (void)invalidateResult;
@end
@implementation ImageCanvas
- (instancetype)init {if((self=[super init])){self.backgroundColor=UIColor.secondarySystemBackgroundColor;self.foreground=YES;self.strokes=[NSMutableArray new];self.isAccessibilityElement=YES;self.accessibilityTraits|=UIAccessibilityTraitAllowsDirectInteraction;self.contentMode=UIViewContentModeRedraw;self.accessibilityIdentifier=@"cutout.canvas";self.accessibilityLabel=@"Image and foreground/background scribbles";}return self;}
- (CGRect)imageRect {if(!self.image)return CGRectZero;CGFloat scale=MIN((self.bounds.size.width-8)/self.image.size.width,(self.bounds.size.height-8)/self.image.size.height);return CGRectMake((self.bounds.size.width-self.image.size.width*scale)/2,(self.bounds.size.height-self.image.size.height*scale)/2,self.image.size.width*scale,self.image.size.height*scale);}
- (void)setSource:(UIImage *)image strokes:(NSArray *)strokes {self.image=image;NSData *data=[NSJSONSerialization dataWithJSONObject:strokes options:0 error:nil];self.strokes=[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:nil]?:[NSMutableArray new];self.activeStroke=nil;[self invalidateResult];}
- (void)invalidateResult {self.result=nil;self.showResult=NO;self.savedResult=NO;self.workingSize=CGSizeZero;[self setNeedsDisplay];}
- (NSString *)accessibilityValue {
    NSUInteger foreground=0,background=0;for(NSDictionary *stroke in self.strokes){if([stroke[@"foreground"] boolValue])foreground++;else background++;}
    NSString *state=self.result?[NSString stringWithFormat:@"current result; working %.0fx%.0f; saved PNG %@; displaying %@",self.workingSize.width,self.workingSize.height,self.savedResult?@"yes":@"no",self.showResult?@"result":@"image"]:@"no current result";
    return [NSString stringWithFormat:@"image %.0fx%.0f; foreground strokes %lu; background strokes %lu; %@",self.image.size.width,self.image.size.height,(unsigned long)foreground,(unsigned long)background,state];
}
- (void)drawRect:(CGRect)rect {if(!self.image)return;CGRect box=[self imageRect];CGContextRef ctx=UIGraphicsGetCurrentContext();CGContextSaveGState(ctx);CGContextClipToRect(ctx,box);for(CGFloat y=box.origin.y;y<CGRectGetMaxY(box);y+=14)for(CGFloat x=box.origin.x;x<CGRectGetMaxX(box);x+=14){NSInteger col=(NSInteger)((x-box.origin.x)/14),row=(NSInteger)((y-box.origin.y)/14);[((col+row)%2)?UIColor.whiteColor:[UIColor colorWithWhite:.85 alpha:1] setFill];UIRectFill(CGRectMake(x,y,14,14));}[(self.showResult&&self.result?self.result:self.image) drawInRect:box];if(!self.showResult){CGFloat scale=box.size.width/self.image.size.width;CGContextTranslateCTM(ctx,box.origin.x,box.origin.y);CGContextScaleCTM(ctx,scale,scale);for(NSDictionary *stroke in self.strokes){NSArray *points=stroke[@"points"];if(!points.count)continue;UIBezierPath *path=[UIBezierPath bezierPath];path.lineWidth=5;path.lineCapStyle=kCGLineCapRound;path.lineJoinStyle=kCGLineJoinRound;NSArray *first=points[0];[path moveToPoint:CGPointMake([first[0] doubleValue],[first[1] doubleValue])];if(points.count==1)[path addLineToPoint:CGPointMake([first[0] doubleValue]+.01,[first[1] doubleValue])];for(NSUInteger i=1;i<points.count;i++){NSArray *xy=points[i];[path addLineToPoint:CGPointMake([xy[0] doubleValue],[xy[1] doubleValue])];}[[stroke[@"foreground"] boolValue]?[UIColor colorWithRed:.12 green:.75 blue:.28 alpha:1]:UIColor.systemRedColor setStroke];[path stroke];}}CGContextRestoreGState(ctx);}
- (void)recordTouches:(NSSet<UITouch *> *)touches ending:(BOOL)ending {UITouch *touch=touches.anyObject;CGPoint p=[touch locationInView:self];CGRect box=[self imageRect];if(!CGRectContainsPoint(box,p)){self.activeStroke=nil;return;}CGFloat x=MAX(0,MIN(self.image.size.width-.001,(p.x-box.origin.x)*self.image.size.width/box.size.width)),y=MAX(0,MIN(self.image.size.height-.001,(p.y-box.origin.y)*self.image.size.height/box.size.height));if(!self.activeStroke){self.activeStroke=[@{@"foreground":@(self.foreground),@"points":[NSMutableArray new]} mutableCopy];[self.strokes addObject:self.activeStroke];}NSMutableArray *points=self.activeStroke[@"points"];[points addObject:@[@(x),@(y)]];[self invalidateResult];if(self.changed)self.changed();if(ending)self.activeStroke=nil;}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {self.activeStroke=nil;[self recordTouches:touches ending:NO];}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {[self recordTouches:touches ending:NO];}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {[self recordTouches:touches ending:YES];}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {self.activeStroke=nil;}
@end

@interface CutoutController () <PHPickerViewControllerDelegate>
@property(nonatomic,strong) ImageCanvas *canvas;
@property(nonatomic,strong) UILabel *status,*credit;
@property(nonatomic,strong) UISegmentedControl *density,*propagation,*working,*seedMode;
@property(nonatomic,strong) UISwitch *alpha;
@property(nonatomic,strong) NSMutableArray<UIControl *> *controls;
@property(nonatomic,strong) NSURL *exportURL;
@property(nonatomic,copy) NSString *exampleName;
@property(nonatomic) BOOL busy,batchActive;
@property(nonatomic) NSUInteger inputGeneration;
@property(nonatomic,strong) NSArray<NSDictionary *> *batchCases;
@property(nonatomic,strong) NSMutableArray *batchResults;
@property(nonatomic) NSUInteger batchIndex;
@property(nonatomic,copy) NSString *batchIdentifier;
@property(nonatomic,strong) dispatch_queue_t worker;
- (BOOL)loadExample:(NSString *)name;
- (BOOL)loadExample:(NSString *)name forBatch:(BOOL)forBatch error:(NSError **)error;
- (void)abortBatch:(NSError *)error;
- (void)stopBatchAfterIndexError:(NSError *)error;
@end
@implementation CutoutController
@synthesize busy=_busy;
- (UIButton *)button:(NSString *)title identifier:(NSString *)identifier action:(SEL)action {UIButton *b=[UIButton buttonWithType:UIButtonTypeSystem];[b setTitle:title forState:UIControlStateNormal];b.accessibilityIdentifier=identifier;[b addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];[self.controls addObject:b];return b;}
- (UIStackView *)row:(NSArray<UIView *> *)views {UIStackView *row=[[UIStackView alloc]initWithArrangedSubviews:views];row.axis=UILayoutConstraintAxisHorizontal;row.distribution=UIStackViewDistributionFillEqually;row.spacing=5;[row.heightAnchor constraintEqualToConstant:34].active=YES;return row;}
- (UISegmentedControl *)segments:(NSArray *)items identifier:(NSString *)identifier {UISegmentedControl *s=[[UISegmentedControl alloc]initWithItems:items];s.selectedSegmentIndex=0;s.accessibilityIdentifier=identifier;s.accessibilityLabel=@{@"cutout.seedMode":@"Seed class",@"cutout.density":@"Density estimation",@"cutout.propagation":@"Distance propagation",@"cutout.working":@"Working-size limit"}[identifier];[s addTarget:self action:@selector(optionsChanged) forControlEvents:UIControlEventValueChanged];[self.controls addObject:s];return s;}
- (void)viewDidLoad { [super viewDidLoad];self.view.backgroundColor=UIColor.systemBackgroundColor;self.controls=[NSMutableArray new];self.worker=dispatch_queue_create("org.example.cutout.serial",DISPATCH_QUEUE_SERIAL);UIStackView *stack=[UIStackView new];stack.axis=UILayoutConstraintAxisVertical;stack.spacing=6;stack.translatesAutoresizingMaskIntoConstraints=NO;[self.view addSubview:stack];UILayoutGuide *safe=self.view.safeAreaLayoutGuide;[NSLayoutConstraint activateConstraints:@[[stack.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:12],[stack.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-12],[stack.topAnchor constraintEqualToAnchor:safe.topAnchor constant:6],[stack.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-6]]];UILabel *title=[UILabel new];title.text=@"CutOut · updated iOS interface";title.font=[UIFont boldSystemFontOfSize:20];[stack addArrangedSubview:title];self.credit=[UILabel new];self.credit.font=[UIFont systemFontOfSize:11];self.credit.numberOfLines=0;[stack addArrangedSubview:self.credit];self.canvas=[ImageCanvas new];[stack addArrangedSubview:self.canvas];[self.canvas.heightAnchor constraintGreaterThanOrEqualToConstant:150].active=YES;[self.canvas setContentHuggingPriority:1 forAxis:UILayoutConstraintAxisVertical];[self.canvas setContentCompressionResistancePriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisVertical];__weak CutoutController *weak=self;self.canvas.changed=^{[weak invalidateCurrentResult:@"Scribbles changed. Segment again to update the result."];};[stack addArrangedSubview:[self row:@[[self button:@"Open image" identifier:@"cutout.open" action:@selector(openImage)],[self button:@"Examples" identifier:@"cutout.examples" action:@selector(examples)],[self button:@"Clear" identifier:@"cutout.clear" action:@selector(clear)]]]];self.seedMode=[self segments:@[@"Foreground",@"Background"] identifier:@"cutout.seedMode"];[self.seedMode removeTarget:self action:@selector(optionsChanged) forControlEvents:UIControlEventValueChanged];[self.seedMode addTarget:self action:@selector(seedChanged) forControlEvents:UIControlEventValueChanged];[stack addArrangedSubview:self.seedMode];self.density=[self segments:@[@"FIGTree",@"Histogram"] identifier:@"cutout.density"];self.propagation=[self segments:@[@"Pixel grid",@"Level-set tree"] identifier:@"cutout.propagation"];[stack addArrangedSubview:self.density];[stack addArrangedSubview:self.propagation];self.alpha=[UISwitch new];self.alpha.accessibilityIdentifier=@"cutout.alpha";self.alpha.accessibilityLabel=@"Refine alpha";[self.alpha addTarget:self action:@selector(optionsChanged) forControlEvents:UIControlEventValueChanged];[self.controls addObject:self.alpha];UILabel *alphaLabel=[UILabel new];alphaLabel.text=@"Refine alpha";alphaLabel.font=[UIFont systemFontOfSize:13];self.working=[self segments:@[@"400",@"800",@"Full"] identifier:@"cutout.working"];UIStackView *settings=[[UIStackView alloc]initWithArrangedSubviews:@[alphaLabel,self.alpha,self.working]];settings.spacing=8;[settings.heightAnchor constraintEqualToConstant:34].active=YES;[self.working setContentHuggingPriority:1 forAxis:UILayoutConstraintAxisHorizontal];[stack addArrangedSubview:settings];[stack addArrangedSubview:[self row:@[[self button:@"Segment" identifier:@"cutout.segment" action:@selector(segment)],[self button:@"Image / result" identifier:@"cutout.toggle" action:@selector(toggle)],[self button:@"Share PNG" identifier:@"cutout.share" action:@selector(share)]]]];self.status=[UILabel new];self.status.font=[UIFont systemFontOfSize:12];self.status.numberOfLines=3;self.status.accessibilityIdentifier=@"cutout.status";[self.status.heightAnchor constraintGreaterThanOrEqualToConstant:44].active=YES;[stack addArrangedSubview:self.status];[self loadExample:@"ellipse"];NSDictionary *args=CutoutLaunchOptions();if([args[@"run_batch"] boolValue]){[self beginBatch];}else if([args[@"run_fixture"] boolValue]){if(![self loadExample:args[@"example"]?:@"ellipse"])return;if(args[@"figtree"])self.density.selectedSegmentIndex=[args[@"figtree"] boolValue]?0:1;self.propagation.selectedSegmentIndex=[args[@"level_set"] boolValue]?1:0;self.alpha.on=[args[@"refine_alpha"] boolValue];NSInteger limit=args[@"max_dimension"]?[args[@"max_dimension"] integerValue]:400;self.working.selectedSegmentIndex=limit==0?2:limit==800?1:0;NSUInteger generation=self.inputGeneration;dispatch_after(dispatch_time(DISPATCH_TIME_NOW,400*NSEC_PER_MSEC),dispatch_get_main_queue(),^{if(generation==self.inputGeneration&&!self.batchActive)[self segment];});}}
- (void)setBusy:(BOOL)busy {_busy=busy;BOOL enabled=!busy&&!self.batchActive;self.canvas.userInteractionEnabled=enabled;for(UIControl *control in self.controls)control.enabled=enabled;}
- (void)invalidateCurrentResult:(NSString *)message {self.inputGeneration++;[self.canvas invalidateResult];self.exportURL=nil;self.status.text=message;}
- (void)optionsChanged {[self invalidateCurrentResult:@"Options changed. Segment again to update the result."];}
- (void)seedChanged {self.canvas.foreground=self.seedMode.selectedSegmentIndex==0;}
- (void)clear {if(self.busy||self.batchActive)return;[self.canvas setSource:self.canvas.image strokes:@[]];[self invalidateCurrentResult:@"Scribbles cleared. Draw both foreground and background."];}
- (BOOL)loadExample:(NSString *)name {
    NSError *error=nil;BOOL loaded=[self loadExample:name forBatch:NO error:&error];
    if(!loaded)self.status.text=[@"Example load failed: " stringByAppendingString:error.localizedDescription?:@"Unknown error"];
    return loaded;
}
- (BOOL)loadExample:(NSString *)name forBatch:(BOOL)forBatch error:(NSError **)error {
    if(self.busy||(self.batchActive&&!forBatch)){if(error)*error=CutoutError(10,@"Inputs are locked while segmentation or a fixture batch is active.");return NO;}
    NSString *example=[name isEqualToString:@"coffee"]?@"coffee":@"ellipse";
    UIImage *image=nil;NSArray *strokes=nil;NSString *credit=nil,*message=nil;
    if([example isEqualToString:@"coffee"]) {
        NSString *imagePath=[NSBundle.mainBundle pathForResource:@"coffee" ofType:@"png"];
        NSString *strokePath=[NSBundle.mainBundle pathForResource:@"coffee-scribbles" ofType:@"json"];
        image=imagePath?[UIImage imageWithContentsOfFile:imagePath]:nil;
        NSData *data=strokePath?[NSData dataWithContentsOfFile:strokePath]:nil;
        NSDictionary *input=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:error]:nil;
        if(!image.CGImage||![input isKindOfClass:NSDictionary.class]||![input[@"foreground"] isKindOfClass:NSArray.class]||![input[@"background"] isKindOfClass:NSArray.class]) {
            if(error&&!*error)*error=CutoutError(11,@"Bundled coffee image or scribbles are unavailable.");return NO;
        }
        NSMutableArray *ordered=[NSMutableArray new];
        for(NSString *key in @[@"foreground",@"background"])for(NSArray *points in input[key]) {
            if(![points isKindOfClass:NSArray.class]){if(error)*error=CutoutError(12,@"Bundled coffee scribbles have an invalid stroke.");return NO;}
            [ordered addObject:@{@"foreground":@([key isEqualToString:@"foreground"]),@"points":points}];
        }
        strokes=ordered;
        credit=@"Updated build of Elad Richardson's recovered CutOut implementation; new UIKit/ObjC++ runner (2026). Coffee: Rachel Michetti / Pikolo Espresso Bar, CC0 via scikit-image.";
        message=@"Coffee fixture, 600 × 400. Demonstration scribbles are inputs, not independent reference annotations or an accuracy benchmark.";
    } else {
        UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];format.scale=1;format.opaque=YES;format.preferredRange=UIGraphicsImageRendererFormatRangeStandard;
        image=[[[UIGraphicsImageRenderer alloc]initWithSize:CGSizeMake(128,96) format:format] imageWithActions:^(UIGraphicsImageRendererContext *ctx){[[UIColor colorWithRed:42/255. green:72/255. blue:130/255. alpha:1] setFill];UIRectFill(CGRectMake(0,0,128,96));[[UIColor colorWithRed:220/255. green:112/255. blue:42/255. alpha:1] setFill];[[UIBezierPath bezierPathWithOvalInRect:CGRectMake(30,18,70,64)] fill];}];
        strokes=@[@{@"foreground":@YES,@"points":@[@[@57,@40],@[@68,@60]]},@{@"foreground":@NO,@"points":@[@[@10,@10],@[@115,@10]]}];
        credit=@"Updated build of Elad Richardson's recovered CutOut implementation; new UIKit/ObjC++ runner (2026). Original credits and dependency terms: Examples → Credits and terms.";
        message=@"Procedural ellipse fixture for execution checks; not a trained model or an accuracy benchmark. Draw strokes or choose the photograph.";
    }
    if(!image.CGImage){if(error)*error=CutoutError(13,@"Example image could not be created.");return NO;}
    [self.canvas setSource:image strokes:strokes];self.exampleName=example;self.credit.text=credit;[self invalidateCurrentResult:message];return YES;
}
- (void)examples {if(self.busy||self.batchActive)return;UIAlertController *sheet=[UIAlertController alertControllerWithTitle:@"Examples and project records" message:nil preferredStyle:UIAlertControllerStyleActionSheet];[sheet addAction:[UIAlertAction actionWithTitle:@"Ellipse run check" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){[self loadExample:@"ellipse"];}]];[sheet addAction:[UIAlertAction actionWithTitle:@"Coffee photograph" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){[self loadExample:@"coffee"];}]];[sheet addAction:[UIAlertAction actionWithTitle:@"Credits and terms" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){[self credits];}]];[sheet addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];sheet.popoverPresentationController.sourceView=self.view;sheet.popoverPresentationController.sourceRect=CGRectMake(self.view.bounds.size.width/2,self.view.bounds.size.height/2,1,1);[self presentViewController:sheet animated:YES completion:nil];}
- (void)credits {TextController *text=[TextController new];text.title=@"Credits and terms";text.body=[NSString stringWithContentsOfFile:[NSBundle.mainBundle pathForResource:@"credits" ofType:@"txt"] encoding:NSUTF8StringEncoding error:nil]?:@"Credit file unavailable.";UINavigationController *nav=[[UINavigationController alloc]initWithRootViewController:text];text.navigationItem.leftBarButtonItem=[[UIBarButtonItem alloc]initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(closeModal)];text.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc]initWithTitle:@"Licenses" style:UIBarButtonItemStylePlain target:self action:@selector(licenses)];[self presentViewController:nav animated:YES completion:nil];}
- (void)licenses {[(UINavigationController *)self.presentedViewController pushViewController:[LicenseController new] animated:YES];}
- (void)closeModal {[self dismissViewControllerAnimated:YES completion:nil];}
- (void)openImage {if(self.busy||self.batchActive)return;PHPickerConfiguration *configuration=[PHPickerConfiguration new];configuration.selectionLimit=1;configuration.filter=PHPickerFilter.imagesFilter;PHPickerViewController *picker=[[PHPickerViewController alloc]initWithConfiguration:configuration];picker.delegate=self;[self presentViewController:picker animated:YES completion:nil];}
- (void)picker:(PHPickerViewController *)picker didFinishPicking:(NSArray<PHPickerResult *> *)results {[picker dismissViewControllerAnimated:YES completion:nil];if(!results.count)return;if(self.busy||self.batchActive){self.status.text=@"Image import was not started while segmentation or a fixture batch is active.";return;}[self setBusy:YES];self.status.text=@"Loading the selected image…";[results[0].itemProvider loadDataRepresentationForTypeIdentifier:@"public.image" completionHandler:^(NSData *data,NSError *error){UIImage *image=nil;if(data.length&&data.length<=100*1024*1024){CGImageSourceRef source=CGImageSourceCreateWithData((__bridge CFDataRef)data,NULL);if(source){NSDictionary *settings=@{(__bridge NSString *)kCGImageSourceCreateThumbnailFromImageAlways:@YES,(__bridge NSString *)kCGImageSourceCreateThumbnailWithTransform:@YES,(__bridge NSString *)kCGImageSourceThumbnailMaxPixelSize:@800};CGImageRef thumb=CGImageSourceCreateThumbnailAtIndex(source,0,(__bridge CFDictionaryRef)settings);if(thumb){UIImage *decoded=[UIImage imageWithCGImage:thumb scale:1 orientation:UIImageOrientationUp];UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];format.scale=1;format.opaque=YES;format.preferredRange=UIGraphicsImageRendererFormatRangeStandard;image=[[[UIGraphicsImageRenderer alloc]initWithSize:decoded.size format:format] imageWithActions:^(UIGraphicsImageRendererContext *ctx){[UIColor.whiteColor setFill];UIRectFill(CGRectMake(0,0,decoded.size.width,decoded.size.height));[decoded drawAtPoint:CGPointZero];}];CGImageRelease(thumb);}CFRelease(source);}}dispatch_async(dispatch_get_main_queue(),^{[self setBusy:NO];if(!image){self.status.text=error.localizedDescription?:@"The selected image could not be decoded within the image-size limit.";return;}self.exampleName=@"imported";[self.canvas setSource:image strokes:@[]];self.credit.text=@"Selected image; updated build of Elad Richardson's recovered CutOut implementation with a new UIKit/ObjC++ runner. Processing stays on this device.";[self invalidateCurrentResult:@"Image loaded at up to 800 pixels. Draw both foreground and background strokes."];});}];}
- (NSDictionary *)options {return @{@"figtree":@(self.density.selectedSegmentIndex==0),@"level_set":@(self.propagation.selectedSegmentIndex==1),@"refine_alpha":@(self.alpha.on),@"max_dimension":@(self.working.selectedSegmentIndex==2?0:self.working.selectedSegmentIndex==1?800:400)};}
- (NSURL *)documents {return [[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;}
- (BOOL)writeJSON:(id)value url:(NSURL *)url error:(NSError **)error {NSData *data=[NSJSONSerialization dataWithJSONObject:value options:NSJSONWritingPrettyPrinted error:error];return data&&[data writeToURL:url options:NSDataWritingAtomic error:error];}
- (NSDictionary *)saveRun:(CutoutRun *)run strokes:(NSArray *)strokes scenario:(NSString *)scenario batchIdentifier:(NSString *)batchIdentifier error:(NSError **)error {
    NSURL *documents=[self documents];
    if(!documents){if(error)*error=CutoutError(20,@"The app Documents directory is unavailable.");return nil;}
    NSURL *runs=[documents URLByAppendingPathComponent:@"runs" isDirectory:YES];
    if(![[NSFileManager defaultManager] createDirectoryAtURL:runs withIntermediateDirectories:YES attributes:nil error:error])return nil;
    NSString *name=[NSString stringWithFormat:@"%@-%@",scenario,NSUUID.UUID.UUIDString];
    NSString *relative=[@"runs" stringByAppendingPathComponent:name];NSURL *directory=[documents URLByAppendingPathComponent:relative isDirectory:YES];
    // mkdir is exclusive: an existing leaf is an error, never a completed-run overwrite.
    if(mkdir(directory.fileSystemRepresentation,0700)!=0){int failure=errno;if(error)*error=[NSError errorWithDomain:NSPOSIXErrorDomain code:failure userInfo:@{NSLocalizedDescriptionKey:@"Cannot create a new exclusive run directory."}];return nil;}
    NSDictionary *images=@{@"input.png":run.input,@"foreground.png":run.foreground,@"background.png":run.background,@"mask.png":run.mask,@"cutout.png":run.cutout};
    for(NSString *filename in images){NSData *png=UIImagePNGRepresentation(images[filename]);if(!png){if(error)*error=CutoutError(21,[@"PNG encoding failed for " stringByAppendingString:filename]);return nil;}if(![png writeToURL:[directory URLByAppendingPathComponent:filename] options:NSDataWritingAtomic error:error])return nil;}
    NSMutableDictionary *record=[run.metrics mutableCopy];record[@"scenario"]=scenario;if(batchIdentifier)record[@"batch_id"]=batchIdentifier;record[@"directory"]=relative;record[@"implementation"]=@"updated/native via new UIKit/ObjC++ runner";record[@"timing_scope"]=@"Image/stroke conversion, native segmentation and UIImage creation; excludes PNG persistence";
    if(![self writeJSON:strokes url:[directory URLByAppendingPathComponent:@"scribbles.json"] error:error])return nil;
    if(![self writeJSON:record url:[directory URLByAppendingPathComponent:@"result.json"] error:error])return nil;
    NSError *pointerError=nil;BOOL pointerSaved=[self writeJSON:@{@"directory":relative,@"completion_marker":@"result.json"} url:[documents URLByAppendingPathComponent:@"last-run.json"] error:&pointerError];
    record[@"last_run_pointer_updated"]=@(pointerSaved);if(!pointerSaved)record[@"persistence_warning"]=[@"The completed run is saved, but last-run.json could not be updated: " stringByAppendingString:pointerError.localizedDescription?:@"Unknown write error"];
    return record;
}
- (void)segment {
    if(self.busy){if(self.batchActive)[self abortBatch:CutoutError(30,@"A batch case could not start because another operation was active.")];return;}
    [self invalidateCurrentResult:@"Running segmentation…"];NSUInteger generation=self.inputGeneration;[self setBusy:YES];
    UIImage *image=self.canvas.image;NSData *snapshot=[NSJSONSerialization dataWithJSONObject:self.canvas.strokes options:0 error:nil];NSArray *strokes=snapshot?[NSJSONSerialization JSONObjectWithData:snapshot options:0 error:nil]:nil;
    NSDictionary *options=[self options];NSString *batchIdentifier=self.batchActive?[self.batchIdentifier copy]:nil;
    NSString *scenario=[NSString stringWithFormat:@"%@-figtree-%d-levelset-%d-alpha-%d%@",self.exampleName,[options[@"figtree"] boolValue],[options[@"level_set"] boolValue],[options[@"refine_alpha"] boolValue],[options[@"max_dimension"] intValue]==0?@"-full":@""];
    dispatch_async(self.worker,^{
        NSError *error=nil;CutoutRun *run=[CutoutBridge segment:image strokes:strokes options:options error:&error];NSDictionary *saved=nil;NSError *saveError=nil;
        if(run)saved=[self saveRun:run strokes:strokes scenario:scenario batchIdentifier:batchIdentifier error:&saveError];
        dispatch_async(dispatch_get_main_queue(),^{
            [self setBusy:NO];BOOL current=generation==self.inputGeneration;
            if(!run){if(current)self.status.text=[@"Segmentation failed: " stringByAppendingString:error.localizedDescription?:@"Unknown error"];NSLog(@"CUTOUT_ERROR %@",error.localizedDescription);[self finishBatchCase:nil error:error];return;}
            if(current){self.canvas.result=run.cutout;self.canvas.showResult=YES;self.canvas.savedResult=saved!=nil;self.canvas.workingSize=CGSizeMake([run.metrics[@"working_width"] doubleValue],[run.metrics[@"working_height"] doubleValue]);[self.canvas setNeedsDisplay];self.exportURL=saved?[[[self documents] URLByAppendingPathComponent:saved[@"directory"]] URLByAppendingPathComponent:@"cutout.png"]:nil;
                NSString *persistence=saved?@"PNG saved in app Documents.":[@"Result displayed; saving failed: " stringByAppendingString:saveError.localizedDescription?:@"Unknown error"];
                if(saved[@"persistence_warning"])persistence=[persistence stringByAppendingFormat:@" %@",saved[@"persistence_warning"]];
                self.status.text=[NSString stringWithFormat:@"Segmented %@ / %@ pixels in %.1f ms; working %@ × %@. %@",run.metrics[@"selected_pixels"],@([run.metrics[@"width"] longValue]*[run.metrics[@"height"] longValue]),[run.metrics[@"elapsed_ms"] doubleValue],run.metrics[@"working_width"],run.metrics[@"working_height"],persistence];
            }
            NSLog(@"CUTOUT_RESULT %@",run.metrics);[self finishBatchCase:saved error:saveError];
        });
    });
}
- (void)toggle {if(!self.canvas.result){self.status.text=@"No current result. Segment the current inputs first.";return;}self.canvas.showResult=!self.canvas.showResult;[self.canvas setNeedsDisplay];}
- (void)share {if(!self.canvas.result||!self.exportURL){self.status.text=@"No current saved result. Segment the current inputs before sharing.";return;}UIActivityViewController *share=[[UIActivityViewController alloc]initWithActivityItems:@[self.exportURL] applicationActivities:nil];share.excludedActivityTypes=@[UIActivityTypeSaveToCameraRoll];share.popoverPresentationController.sourceView=self.view;share.popoverPresentationController.sourceRect=CGRectMake(self.view.bounds.size.width/2,self.view.bounds.size.height-60,1,1);[self presentViewController:share animated:YES completion:nil];}
- (void)beginBatch {
    if(self.busy||self.batchActive){self.status.text=@"Cannot start a fixture batch while an operation is active.";return;}
    NSMutableArray *cases=[NSMutableArray new];for(NSNumber *fig in @[@NO,@YES])for(NSNumber *tree in @[@NO,@YES])for(NSNumber *alpha in @[@NO,@YES])[cases addObject:@{@"example":@"ellipse",@"figtree":fig,@"level_set":tree,@"refine_alpha":alpha,@"max_dimension":@400}];for(NSNumber *tree in @[@NO,@YES])for(NSNumber *alpha in @[@NO,@YES])[cases addObject:@{@"example":@"coffee",@"figtree":@YES,@"level_set":tree,@"refine_alpha":alpha,@"max_dimension":@400}];for(NSNumber *tree in @[@NO,@YES])[cases addObject:@{@"example":@"coffee",@"figtree":@YES,@"level_set":tree,@"refine_alpha":@YES,@"max_dimension":@0}];
    self.batchCases=cases;self.batchResults=[NSMutableArray new];self.batchIndex=0;self.batchIdentifier=NSUUID.UUID.UUIDString;self.batchActive=YES;[self setBusy:NO];
    NSError *resetError=nil;NSURL *index=[[self documents] URLByAppendingPathComponent:@"batch-results.json"];
    if([[NSFileManager defaultManager] fileExistsAtPath:index.path]&&![[NSFileManager defaultManager] removeItemAtURL:index error:&resetError]){[self stopBatchAfterIndexError:resetError];return;}
    if(![self writeJSON:@{@"batch_id":self.batchIdentifier,@"batch_complete":@NO,@"passed":@NO,@"expected_runs":@(cases.count),@"completed_runs":@0,@"results":@[]} url:index error:&resetError]){[self stopBatchAfterIndexError:resetError];return;}
    NSLog(@"CUTOUT_BATCH_START %@",self.batchIdentifier);dispatch_after(dispatch_time(DISPATCH_TIME_NOW,400*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self nextBatchCase];});
}
- (void)nextBatchCase {
    if(!self.batchActive||self.batchIndex>=self.batchCases.count)return;
    if(self.busy){[self abortBatch:CutoutError(31,@"A batch case encountered an unexpected active operation.")];return;}
    NSDictionary *item=self.batchCases[self.batchIndex];NSError *loadError=nil;
    if(![self loadExample:item[@"example"] forBatch:YES error:&loadError]){[self finishBatchCase:nil error:loadError];return;}
    self.density.selectedSegmentIndex=[item[@"figtree"] boolValue]?0:1;self.propagation.selectedSegmentIndex=[item[@"level_set"] boolValue]?1:0;self.alpha.on=[item[@"refine_alpha"] boolValue];self.working.selectedSegmentIndex=[item[@"max_dimension"] intValue]==0?2:0;[self segment];
}
- (void)logBatchEndPassed:(BOOL)passed aborted:(BOOL)aborted error:(NSError *)error {
    NSMutableDictionary *record=[@{@"id":self.batchIdentifier?:@"none",@"passed":@(passed),@"aborted":@(aborted),@"completed_runs":@(self.batchIndex)} mutableCopy];
    if(error)record[@"error"]=error.localizedDescription;
    NSData *data=[NSJSONSerialization dataWithJSONObject:record options:0 error:nil];NSLog(@"CUTOUT_BATCH_END %@",data?[[NSString alloc]initWithData:data encoding:NSUTF8StringEncoding]:record.description);
}
- (void)stopBatchAfterIndexError:(NSError *)error {
    NSString *prior=[self.status.text hasPrefix:@"Batch aborted:"]?self.status.text:nil;
    self.batchCases=nil;self.batchActive=NO;[self setBusy:self.busy];self.status.text=[@"Batch record write failed; the batch was stopped: " stringByAppendingString:error.localizedDescription?:@"Unknown write error"];
    if(prior)self.status.text=[prior stringByAppendingFormat:@" %@",self.status.text];
    [self logBatchEndPassed:NO aborted:YES error:error];
}
- (void)abortBatch:(NSError *)error {
    if(!self.batchActive)return;[self invalidateCurrentResult:[@"Batch aborted: " stringByAppendingString:error.localizedDescription]];
    NSDictionary *requested=self.batchIndex<self.batchCases.count?self.batchCases[self.batchIndex]:@{};
    [self.batchResults addObject:@{@"success":@NO,@"error":error.localizedDescription,@"case_index":@(self.batchIndex),@"requested_case":requested}];
    NSError *indexError=nil;BOOL wrote=[self writeJSON:@{@"batch_id":self.batchIdentifier,@"batch_complete":@NO,@"passed":@NO,@"aborted":@YES,@"aborted_case_index":@(self.batchIndex),@"expected_runs":@(self.batchCases.count),@"completed_runs":@(self.batchIndex),@"results":self.batchResults} url:[[self documents] URLByAppendingPathComponent:@"batch-results.json"] error:&indexError];
    self.batchCases=nil;self.batchActive=NO;[self setBusy:self.busy];if(!wrote){[self stopBatchAfterIndexError:indexError];return;}[self logBatchEndPassed:NO aborted:YES error:error];
}
- (void)finishBatchCase:(NSDictionary *)saved error:(NSError *)error {
    if(!self.batchActive)return;NSMutableDictionary *entry=saved?[saved mutableCopy]:[NSMutableDictionary new];entry[@"case_index"]=@(self.batchIndex);entry[@"success"]=@(saved!=nil);if(error)entry[@"error"]=error.localizedDescription;
    if(!saved){NSDictionary *item=self.batchCases[self.batchIndex];entry[@"requested_case"]=item;}
    [self.batchResults addObject:entry];self.batchIndex++;BOOL complete=self.batchIndex==self.batchCases.count;BOOL passed=YES;for(NSDictionary *row in self.batchResults)if(![row[@"success"] boolValue])passed=NO;
    NSError *indexError=nil;BOOL wrote=[self writeJSON:@{@"batch_id":self.batchIdentifier,@"batch_complete":@(complete),@"passed":@(passed&&complete),@"expected_runs":@(self.batchCases.count),@"completed_runs":@(self.batchIndex),@"results":self.batchResults} url:[[self documents] URLByAppendingPathComponent:@"batch-results.json"] error:&indexError];
    if(!wrote){[self stopBatchAfterIndexError:indexError];return;}
    if(complete){self.batchCases=nil;self.batchActive=NO;[self setBusy:NO];self.status.text=[NSString stringWithFormat:@"Batch complete: %lu runs; %@. %@",(unsigned long)self.batchIndex,passed?@"all saved":@"some failed",self.status.text];[self logBatchEndPassed:passed aborted:NO error:nil];}
    else dispatch_async(dispatch_get_main_queue(),^{[self nextBatchCase];});
}
@end
