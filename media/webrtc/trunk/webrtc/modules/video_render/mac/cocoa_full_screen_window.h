














#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_MAC_COCOA_FULL_SCREEN_WINDOW_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_MAC_COCOA_FULL_SCREEN_WINDOW_H_

#import <Cocoa/Cocoa.h>


@interface CocoaFullScreenWindow : NSObject {
	NSWindow*			_window;
}

-(id)init;
-(void)grabFullScreen;
-(void)releaseFullScreen;
-(NSWindow*)window;

@end

#endif  
