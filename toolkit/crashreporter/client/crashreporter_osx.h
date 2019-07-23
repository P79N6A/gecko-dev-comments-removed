





































#ifndef CRASHREPORTER_OSX_H__
#define CRASHREPORTER_OSX_H__

#include <Cocoa/Cocoa.h>
#include "HTTPMultipartUpload.h"

@interface CrashReporterUI : NSObject
{
    
    IBOutlet NSView *enableView;

    IBOutlet NSTextField *descriptionLabel;
    IBOutlet NSButton *disableReportingButton;
    IBOutlet NSButton *dontSendButton;
    IBOutlet NSButton *sendButton;

    
    IBOutlet NSView *uploadingView;

    IBOutlet NSTextField *progressLabel;
    IBOutlet NSProgressIndicator *progressBar;
    IBOutlet NSButton *closeButton;

    HTTPMultipartUpload *mPost;
}

- (IBAction)closeClicked:(id)sender;
- (IBAction)sendClicked:(id)sender;

- (void)setView:(NSWindow *)w newView: (NSView *)v animate: (BOOL) animate;
- (void)setupPost;
- (void)uploadThread:(id)post;
- (void)uploadComplete:(id)error;

@end

#endif
