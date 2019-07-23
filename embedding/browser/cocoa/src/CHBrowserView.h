




































#ifndef __nsCocoaBrowserView_h__
#define __nsCocoaBrowserView_h__

#undef DARWIN
#import <Cocoa/Cocoa.h>

@class CHBrowserView;
class CHBrowserListener;
class nsIDOMWindow;
class nsIWebBrowser;
class nsIDOMNode;
class nsIDOMEvent;
class nsIWebBrowserFind;
class nsIEventSink;
class nsIDragHelperService;






@protocol CHBrowserListener

- (void)onLoadingStarted;
- (void)onLoadingCompleted:(BOOL)succeeded;



- (void)onProgressChange:(int)currentBytes outOf:(int)maxBytes;
- (void)onLocationChange:(NSString*)urlSpec;
- (void)onStatusChange:(NSString*)aMessage;
- (void)onSecurityStateChange:(unsigned long)newState;

- (void)onShowContextMenu:(int)flags domEvent:(nsIDOMEvent*)aEvent domNode:(nsIDOMNode*)aNode;

- (void)onShowTooltip:(NSPoint)where withText:(NSString*)text;
- (void)onHideTooltip;

@end

typedef enum {
  NSStatusTypeScript            = 0x0001,
  NSStatusTypeScriptDefault     = 0x0002,
  NSStatusTypeLink              = 0x0003,
} NSStatusType;

@protocol CHBrowserContainer

- (void)setStatus:(NSString *)statusString ofType:(NSStatusType)type;
- (NSString *)title;
- (void)setTitle:(NSString *)title;


- (void)sizeBrowserTo:(NSSize)dimensions;

- (CHBrowserView*)createBrowserWindow:(unsigned int)mask;

- (NSMenu*)contextMenu;
- (NSWindow*)nativeWindow;




- (BOOL)shouldAcceptDragFromSource:(id)dragSource;

@end

enum {
  NSLoadFlagsNone                   = 0x0000,
  NSLoadFlagsDontPutInHistory       = 0x0010,
  NSLoadFlagsReplaceHistoryEntry    = 0x0020,
  NSLoadFlagsBypassCacheAndProxy    = 0x0040
}; 

enum {
  NSStopLoadNetwork   = 0x01,
  NSStopLoadContent   = 0x02,
  NSStopLoadAll       = 0x03  
};

@interface CHBrowserView : NSView 
{
  nsIWebBrowser* _webBrowser;
  CHBrowserListener* _listener;
  NSWindow* mWindow;
  
  nsIDragHelperService* mDragHelper;
  NSPoint               mLastTrackedLocation;
  NSWindow*             mLastTrackedWindow;
}


- (id)initWithFrame:(NSRect)frame;
- (id)initWithFrame:(NSRect)frame andWindow:(NSWindow*)aWindow;

- (void)dealloc;
- (void)setFrame:(NSRect)frameRect;


- (void)addListener:(id <CHBrowserListener>)listener;
- (void)removeListener:(id <CHBrowserListener>)listener;
- (void)setContainer:(id <CHBrowserContainer>)container;
- (nsIDOMWindow*)getContentWindow;


- (void)loadURI:(NSString *)urlSpec referrer:(NSString*)referrer flags:(unsigned int)flags;
- (void)reload:(unsigned int)flags;
- (BOOL)canGoBack;
- (BOOL)canGoForward;
- (void)goBack;
- (void)goForward;
- (void)gotoIndex:(int)index;
- (void)stop:(unsigned int)flags;
- (NSString*)getCurrentURI;

- (void)saveDocument: (NSView*)aFilterView filterList: (NSPopUpButton*)aFilterList;
- (void)saveURL: (NSView*)aFilterView filterList: (NSPopUpButton*)aFilterList
            url: (NSString*)aURLSpec suggestedFilename: (NSString*)aFilename;

- (void)printDocument;

- (BOOL)findInPageWithPattern:(NSString*)inText caseSensitive:(BOOL)inCaseSensitive
            wrap:(BOOL)inWrap backwards:(BOOL)inBackwards;

-(BOOL)validateMenuItem: (NSMenuItem*)aMenuItem;

-(IBAction)cut:(id)aSender;
-(BOOL)canCut;
-(IBAction)copy:(id)aSender;
-(BOOL)canCopy;
-(IBAction)paste:(id)aSender;
-(BOOL)canPaste;
-(IBAction)delete:(id)aSender;
-(BOOL)canDelete;
-(IBAction)selectAll:(id)aSender;

-(IBAction)undo:(id)aSender;
-(IBAction)redo:(id)aSender;

- (BOOL)canUndo;
- (BOOL)canRedo;

-(NSString*)getCurrentURLSpec;

- (void)setActive: (BOOL)aIsActive;

- (NSMenu*)contextMenu;
- (NSWindow*)nativeWindow;

- (void)destroyWebBrowser;
- (nsIWebBrowser*)getWebBrowser;
- (CHBrowserListener*)getCocoaBrowserListener;
- (void)setWebBrowser:(nsIWebBrowser*)browser;

- (NSString*)getFocusedURLString;

  
  
- (void) findEventSink:(nsIEventSink**)outSink forPoint:(NSPoint)inPoint inWindow:(NSWindow*)inWind;

@end

#endif
