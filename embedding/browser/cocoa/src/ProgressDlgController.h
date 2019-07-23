




































#import <AppKit/AppKit.h>

#import "CHDownloadProgressDisplay.h"

#include "nscore.h"

class nsIWebBrowserPersist;
class nsISupports;
class nsIInputStream;
class nsDownloadListener;


@interface ChimeraDownloadControllerFactory : DownloadControllerFactory
@end


@interface ProgressDlgController : NSWindowController <CHDownloadProgressDisplay>
{
    IBOutlet NSTextField *mElapsedTimeLabel;
    IBOutlet NSTextField *mFromField;
    IBOutlet NSTextField *mStatusLabel;
    IBOutlet NSTextField *mTimeLeftLabel;
    IBOutlet NSTextField *mToField;
    IBOutlet NSProgressIndicator *mProgressBar;

    NSToolbarItem *leaveOpenToggleToolbarItem;

    BOOL      mSaveFileDialogShouldStayOpen;
    BOOL      mIsFileSave;
    BOOL      mDownloadIsComplete;
    long      mCurrentProgress; 
        
    CHDownloader        *mDownloader;   
    NSTimer             *mDownloadTimer;
}

-(void)autosaveWindowFrame;

-(void) setupDownloadTimer;
-(void) killDownloadTimer;
-(void) setDownloadProgress:(NSTimer *)aTimer;
-(NSString *) formatTime:(int)aSeconds;
-(NSString *) formatFuzzyTime:(int)aSeconds;
-(NSString *) formatBytes:(float)aBytes;

@end
