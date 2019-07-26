














#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_INFO_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_INFO_OBJC_H_

#import <QTKit/QTKit.h>
#import <Foundation/Foundation.h>
#include "video_capture_qtkit_utility.h"
#include "video_capture_qtkit_info.h"

@interface VideoCaptureMacQTKitInfoObjC : NSObject{
    bool                                _OSSupportedInfo;
    NSArray*                            _captureDevicesInfo;
    int                                    _captureDeviceCountInfo;

}







- (NSNumber*)getCaptureDevices;
- (NSNumber*)initializeVariables;
- (void)checkOSSupported;








- (NSNumber*)getCaptureDeviceCount;

- (NSNumber*)getDeviceNamesFromIndex:(WebRtc_UWord32)index
    DefaultName:(char*)deviceName
    WithLength:(WebRtc_UWord32)deviceNameLength
    AndUniqueID:(char*)deviceUniqueID
    WithLength:(WebRtc_UWord32)deviceUniqueIDLength
    AndProductID:(char*)deviceProductID
    WithLength:(WebRtc_UWord32)deviceProductIDLength;

- (NSNumber*)displayCaptureSettingsDialogBoxWithDevice:
        (const char*)deviceUniqueIdUTF8
    AndTitle:(const char*)dialogTitleUTF8
    AndParentWindow:(void*) parentWindow AtX:(WebRtc_UWord32)positionX
    AndY:(WebRtc_UWord32) positionY;
@end

#endif  
