














#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_OBJC_H_

#import <AppKit/AppKit.h>
#import <CoreData/CoreData.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <QTKit/QTKit.h>

#include "webrtc/modules/video_capture/mac/qtkit/video_capture_qtkit.h"

@interface VideoCaptureMacQTKitObjC : NSObject {
  bool _capturing;
  int _frameRate;
  int _frameWidth;
  int _frameHeight;
  int _framesDelivered;
  int _framesRendered;
  bool _captureInitialized;

  webrtc::videocapturemodule::VideoCaptureMacQTKit* _owner;
  NSLock* lock_;

  QTCaptureSession* _captureSession;
  QTCaptureDeviceInput* _captureVideoDeviceInput;
  QTCaptureDecompressedVideoOutput* _captureDecompressedVideoOutput;
  NSArray* _captureDevices;
  int _captureDeviceCount;
  char _captureDeviceNameUTF8[1024];
  char _captureDeviceNameUniqueID[1024];
}

- (void)getCaptureDevices;
- (BOOL)initializeVideoCapture;
- (BOOL)initializeVariables;

- (void)registerOwner:(webrtc::videocapturemodule::VideoCaptureMacQTKit*)owner;
- (BOOL)setCaptureDeviceById:(char*)uniqueId;
- (void)setCaptureHeight:(int)height width:(int)width frameRate:(int)frameRate;
- (void)startCapture;
- (void)stopCapture;

@end

#endif  
