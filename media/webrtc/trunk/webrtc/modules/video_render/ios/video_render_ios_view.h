









#ifndef WEBRTC_MODULES_VIDEO_RENDER_IOS_RENDER_VIEW_H_
#define WEBRTC_MODULES_VIDEO_RENDER_IOS_RENDER_VIEW_H_

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

#include "webrtc/modules/video_render/ios/open_gles20.h"

@interface VideoRenderIosView : UIView

- (BOOL)createContext;
- (BOOL)presentFramebuffer;
- (BOOL)renderFrame:(webrtc::I420VideoFrame*)frameToRender;
- (BOOL)setCoordinatesForZOrder:(const float)zOrder
                           Left:(const float)left
                            Top:(const float)top
                          Right:(const float)right
                         Bottom:(const float)bottom;

@property(nonatomic, retain) EAGLContext* context;

@end

#endif  
