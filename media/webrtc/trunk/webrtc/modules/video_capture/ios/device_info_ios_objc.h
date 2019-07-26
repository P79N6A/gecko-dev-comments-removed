









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_IOS_DEVICE_INFO_IOS_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_IOS_DEVICE_INFO_IOS_OBJC_H_

#import <AVFoundation/AVFoundation.h>

@interface DeviceInfoIosObjC : NSObject
+ (int)captureDeviceCount;
+ (AVCaptureDevice*)captureDeviceForIndex:(int)index;
+ (AVCaptureDevice*)captureDeviceForUniqueId:(NSString*)uniqueId;
+ (NSString*)deviceNameForIndex:(int)index;
+ (NSString*)deviceUniqueIdForIndex:(int)index;
+ (NSString*)deviceNameForUniqueId:(NSString*)uniqueId;
@end

#endif  
