




























#import <Cocoa/Cocoa.h>

#import <Breakpad/Breakpad.h>

enum BreakpadForkBehavior {
  DONOTHING = 0,
  UNINSTALL,
  RESETEXCEPTIONPORT
};

enum BreakpadForkTestCrashPoint {
  DURINGLAUNCH = 5,
  AFTERLAUNCH = 6,
  BETWEENFORKEXEC = 7
};

@interface Controller : NSObject {
  IBOutlet NSWindow *window_;
  IBOutlet NSWindow *forkTestOptions_;

  BreakpadRef breakpad_;

  enum BreakpadForkBehavior bpForkOption;

  BOOL useVFork;
  enum BreakpadForkTestCrashPoint progCrashPoint;
}

- (IBAction)crash:(id)sender;
- (IBAction)forkTestOptions:(id)sender;
- (IBAction)forkTestGo:(id)sender;
- (IBAction)showForkTestWindow:(id) sender;
- (void)generateReportWithoutCrash:(id)sender;
- (void)awakeFromNib;

@end
