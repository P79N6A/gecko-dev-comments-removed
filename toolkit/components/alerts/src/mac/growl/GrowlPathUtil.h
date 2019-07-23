








#import <Cocoa/Cocoa.h>

@interface GrowlPathUtil : NSObject {

}






+ (NSBundle *) growlPrefPaneBundle;
+ (NSBundle *) helperAppBundle;
+ (NSString *) growlSupportDir;


+ (NSString *) screenshotsDirectory;

+ (NSString *) nextScreenshotName;

@end
