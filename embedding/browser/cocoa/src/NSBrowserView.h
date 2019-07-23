




































#ifndef __nsCocoaBrowserView_h__
#define __nsCocoaBrowserView_h__

#import <Cocoa/Cocoa.h>

@class NSBrowserView;
class nsCocoaBrowserListener;
class nsIDOMWindow;
class nsIWebBrowser;





@protocol NSBrowserListener
- (void)onLoadingStarted;
- (void)onLoadingCompleted:(BOOL)succeeded;



- (void)onProgressChange:(int)currentBytes outOf:(int)maxBytes;
- (void)onLocationChange:(NSURL*)url;
@end

typedef enum {
  NSStatusTypeScript            = 0x0001,
  NSStatusTypeScriptDefault     = 0x0002,
  NSStatusTypeLink              = 0x0003,
} NSStatusType;

@protocol NSBrowserContainer
- (void)setStatus:(NSString *)statusString ofType:(NSStatusType)type;
- (NSString *)title;
- (void)setTitle:(NSString *)title;


- (void)sizeBrowserTo:(NSSize)dimensions;

- (NSBrowserView*)createBrowserWindow:(unsigned int)mask;
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

@interface NSBrowserView : NSView 
{
  nsIWebBrowser* _webBrowser;
  nsCocoaBrowserListener* _listener;
}


- (id)initWithFrame:(NSRect)frame;
- (void)dealloc;
- (void)setFrame:(NSRect)frameRect;


- (void)addListener:(id <NSBrowserListener>)listener;
- (void)removeListener:(id <NSBrowserListener>)listener;
- (void)setContainer:(id <NSBrowserContainer>)container;
- (nsIDOMWindow*)getContentWindow;


- (void)loadURI:(NSURL *)url flags:(unsigned int)flags;
- (void)reload:(unsigned int)flags;
- (BOOL)canGoBack;
- (BOOL)canGoForward;
- (void)goBack;
- (void)goForward;
- (void)gotoIndex:(int)index;
- (void)stop:(unsigned int)flags;
- (NSURL*)getCurrentURI;

- (nsIWebBrowser*)getWebBrowser;
- (void)setWebBrowser:(nsIWebBrowser*)browser;
@end

#endif
