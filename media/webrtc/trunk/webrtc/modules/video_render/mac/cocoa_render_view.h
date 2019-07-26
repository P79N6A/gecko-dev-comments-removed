













#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_MAC_COCOA_RENDER_VIEW_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_MAC_COCOA_RENDER_VIEW_H_

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#import <OpenGL/OpenGL.h>

@interface CocoaRenderView : NSOpenGLView {
  NSOpenGLContext* _nsOpenGLContext;
}

-(void)initCocoaRenderView:(NSOpenGLPixelFormat*)fmt;
-(void)initCocoaRenderViewFullScreen:(NSOpenGLPixelFormat*)fmt;
-(NSOpenGLContext*)nsOpenGLContext;
@end

#endif  
