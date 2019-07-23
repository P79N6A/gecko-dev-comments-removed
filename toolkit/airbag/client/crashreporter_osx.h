





































#ifndef CRASHREPORTER_OSX_H__
#define CRASHREPORTER_OSX_H__

#include <Cocoa/Cocoa.h>
#include "HTTPMultipartUpload.h"
#include "crashreporter.h"

@interface CrashReporterUI : NSObject
{
    IBOutlet NSWindow* window;

    
    IBOutlet NSTextField* headerLabel;
    IBOutlet NSTextField* descriptionLabel;
    IBOutlet NSButton* viewReportButton;
    IBOutlet NSTextField* viewReportLabel;
    IBOutlet NSScrollView* viewReportScrollView;
    IBOutlet NSTextView* viewReportTextView;
    IBOutlet NSButton* submitReportButton;
    IBOutlet NSButton* emailMeButton;
    IBOutlet NSTextField* emailText;
    IBOutlet NSButton* closeButton;
    IBOutlet NSButton* restartButton;

    
    IBOutlet NSView* errorView;
    IBOutlet NSTextField* errorHeaderLabel;
    IBOutlet NSTextField* errorLabel;
    IBOutlet NSButton* errorCloseButton;

    HTTPMultipartUpload *mPost;
}

- (void)showCrashUI:(const std::string&)dumpfile
    queryParameters:(const StringTable&)queryParameters
            sendURL:(const std::string&)sendURL;
- (void)showErrorUI:(const std::string&)dumpfile;
- (void)showReportInfo;

- (IBAction)viewReportClicked:(id)sender;
- (IBAction)closeClicked:(id)sender;
- (IBAction)closeAndSendClicked:(id)sender;
- (IBAction)restartClicked:(id)sender;
- (IBAction)emailMeClicked:(id)sender;

- (void)controlTextDidChange:(NSNotification *)note;

- (float)setStringFitVertically:(NSControl*)control
                         string:(NSString*)str
                   resizeWindow:(BOOL)resizeWindow;
- (void)setView:(NSView*)v animate: (BOOL) animate;
- (void)updateEmail;
- (void)sendReport;
- (bool)setupPost;
- (void)uploadThread:(id)post;
- (void)uploadComplete:(id)data;

@end

#endif
