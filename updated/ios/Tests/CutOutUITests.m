#import <XCTest/XCTest.h>

@interface CutOutUITests : XCTestCase
@property(nonatomic,strong) XCUIApplication *app;
@end
@implementation CutOutUITests
- (void)setUp {self.continueAfterFailure=NO;self.app=[XCUIApplication new];self.app.launchArguments=@[@"-run_fixture",@"YES",@"-example",@"coffee",@"-figtree",@"YES",@"-level_set",@"YES",@"-refine_alpha",@"YES",@"-max_dimension",@"400"];[self.app launch];[self waitForStatus:@"PNG saved"];}
- (void)waitForStatus:(NSString *)text {XCUIElement *status=self.app.staticTexts[@"cutout.status"];NSPredicate *predicate=[NSPredicate predicateWithFormat:@"label CONTAINS[c] %@",text];[self expectationForPredicate:predicate evaluatedWithObject:status handler:nil];[self waitForExpectationsWithTimeout:90 handler:nil];}
- (XCUICoordinate *)photoPoint:(CGPoint)point canvas:(XCUIElement *)canvas {CGSize size=canvas.frame.size;CGFloat scale=MIN((size.width-8)/600,(size.height-8)/400);return [canvas coordinateWithNormalizedOffset:CGVectorMake(((size.width-600*scale)/2+point.x*scale)/size.width,((size.height-400*scale)/2+point.y*scale)/size.height)];}
- (void)testTouchErrorRecoveryAndStaleResult {
    [self.app.buttons[@"cutout.clear"] tap];[self.app.buttons[@"cutout.segment"] tap];[self waitForStatus:@"Segmentation failed: Both foreground and background scribbles are required"];
    XCUIElement *canvas=self.app.otherElements[@"cutout.canvas"];XCTAssertTrue(canvas.exists);
    [self.app.segmentedControls[@"cutout.seedMode"].buttons[@"Foreground"] tap];
    [[self photoPoint:CGPointMake(250,100) canvas:canvas] pressForDuration:.05 thenDragToCoordinate:[self photoPoint:CGPointMake(300,130) canvas:canvas]];
    [self.app.segmentedControls[@"cutout.seedMode"].buttons[@"Background"] tap];
    [[self photoPoint:CGPointMake(550,50) canvas:canvas] pressForDuration:.05 thenDragToCoordinate:[self photoPoint:CGPointMake(550,200) canvas:canvas]];
    [self.app.segmentedControls[@"cutout.working"].buttons[@"Full"] tap];[self.app.buttons[@"cutout.segment"] tap];[self waitForStatus:@"PNG saved"];
    XCTAssertTrue([((NSString *)canvas.value) containsString:@"working 600x400"]);
    XCTAssertTrue([((NSString *)canvas.value) containsString:@"saved PNG yes"]);
    [self.app.segmentedControls[@"cutout.propagation"].buttons[@"Pixel grid"] tap];[self.app.buttons[@"cutout.toggle"] tap];[self waitForStatus:@"No current result"];
    [self.app.buttons[@"cutout.segment"] tap];[self waitForStatus:@"PNG saved"];
    [self.app.buttons[@"cutout.clear"] tap];[self.app.buttons[@"cutout.toggle"] tap];[self waitForStatus:@"No current result"];
}
- (void)testCreditsAndLicenseViewer {
    [self.app.buttons[@"cutout.examples"] tap];[self.app.buttons[@"Credits and terms"] tap];XCUIElement *text=self.app.textViews[@"cutout.termsText"];XCTAssertTrue([text waitForExistenceWithTimeout:5]);NSString *body=(NSString *)text.value;XCTAssertTrue([body containsString:@"Elad Richardson"]);XCTAssertTrue([body containsString:@"CMLA"]);XCTAssertTrue([body containsString:@"61 avenue"]);[self.app.navigationBars.buttons[@"Licenses"] tap];XCUIElement *license=self.app.tables.staticTexts[@"OpenCV-LICENSE.txt"];for(int i=0;i<6&&!license.hittable;i++)[self.app.tables.firstMatch swipeUp];XCTAssertTrue(license.hittable);[license tap];XCTAssertTrue([self.app.textViews[@"cutout.termsText"] waitForExistenceWithTimeout:5]);XCTAssertTrue([((NSString *)self.app.textViews[@"cutout.termsText"].value) containsString:@"Apache License"]);
}
// Picker interaction is opt-in: it can display the user's photo library.
- (void)testPhotoPickerPresentationWhenExplicitlyEnabled {
    if(![NSProcessInfo.processInfo.environment[@"CUTOUT_TEST_PHOTO_PICKER"] isEqualToString:@"YES"]){XCTSkip(@"Photo-library presentation requires an explicit opt-in.");}
    [self.app.buttons[@"cutout.open"] tap];XCUIElement *cancel=self.app.buttons[@"Cancel"];XCTAssertTrue([cancel waitForExistenceWithTimeout:10]);[cancel tap];XCTAssertTrue(self.app.buttons[@"cutout.segment"].exists);
}
@end
