









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_OBJC_H_

#import <UIKit/UIKit.h>

#include "webrtc/modules/video_capture/ios/video_capture_ios.h"

@interface VideoCaptureIosObjC
    : UIViewController<AVCaptureVideoDataOutputSampleBufferDelegate> {
 @private
  webrtc::videocapturemodule::VideoCaptureIos* _owner;
  webrtc::VideoCaptureCapability _capability;
  AVCaptureSession* _captureSession;
  int _captureId;
}

@property webrtc::VideoCaptureRotation frameRotation;




- (id)initWithOwner:(webrtc::videocapturemodule::VideoCaptureIos*)owner
          captureId:(int)captureId;
- (BOOL)setCaptureDeviceByUniqueId:(NSString*)uniequeId;
- (BOOL)startCaptureWithCapability:
        (const webrtc::VideoCaptureCapability&)capability;
- (BOOL)stopCapture;

@end
#endif  
