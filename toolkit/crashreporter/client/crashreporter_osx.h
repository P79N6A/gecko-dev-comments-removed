





































#ifndef CRASHREPORTER_OSX_H__
#define CRASHREPORTER_OSX_H__

#include <Cocoa/Cocoa.h>
#include "HTTPMultipartUpload.h"
#include "crashreporter.h"

@interface CrashReporterUI : NSObject
{
    IBOutlet NSWindow* window;

    
    IBOutlet NSView* enableView;

    IBOutlet NSTextField* descriptionLabel;
    IBOutlet NSButton* disableReportingButton;
    IBOutlet NSButton* dontSendButton;
    IBOutlet NSButton* sendButton;

    
    IBOutlet NSView* uploadingView;

    IBOutlet NSTextField* progressLabel;
    IBOutlet NSProgressIndicator* progressBar;
    IBOutlet NSButton* closeButton;

    
    IBOutlet NSView* errorView;
    IBOutlet NSTextField* errorLabel;
    IBOutlet NSButton* errorCloseButton;

    HTTPMultipartUpload *mPost;
}

- (void)showDefaultUI;
- (void)showCrashUI:(const std::string&)dumpfile
    queryParameters:(const StringTable&)queryParameters
            sendURL:(const std::string&)sendURL;
- (void)showErrorUI:(const std::string&)dumpfile;

- (IBAction)closeClicked:(id)sender;
- (IBAction)sendClicked:(id)sender;

- (void)setView:(NSWindow *)w newView: (NSView *)v animate: (BOOL) animate;
- (bool)setupPost;
- (void)uploadThread:(id)post;
- (void)uploadComplete:(id)data;

@end

#endif
