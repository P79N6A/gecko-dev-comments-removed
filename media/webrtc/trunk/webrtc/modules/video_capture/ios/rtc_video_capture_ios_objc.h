









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_OBJC_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "webrtc/modules/video_capture/ios/video_capture_ios.h"





@interface RTCVideoCaptureIosObjC
    : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>

@property webrtc::VideoCaptureRotation frameRotation;




- (id)initWithOwner:(webrtc::videocapturemodule::VideoCaptureIos*)owner
          captureId:(int)captureId;
- (BOOL)setCaptureDeviceByUniqueId:(NSString*)uniqueId;
- (BOOL)startCaptureWithCapability:
        (const webrtc::VideoCaptureCapability&)capability;
- (BOOL)stopCapture;

@end
#endif  
