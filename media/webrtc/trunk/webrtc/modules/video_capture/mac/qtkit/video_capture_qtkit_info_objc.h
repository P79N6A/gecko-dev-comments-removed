














#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_INFO_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_INFO_OBJC_H_

#import <Foundation/Foundation.h>
#import <QTKit/QTKit.h>

#include "webrtc/modules/video_capture/mac/qtkit/video_capture_qtkit_info.h"
#include "webrtc/modules/video_capture/mac/qtkit/video_capture_qtkit_utility.h"

@interface VideoCaptureMacQTKitInfoObjC : NSObject{
    bool                                _OSSupportedInfo;
    NSArray*                            _captureDevicesInfo;
    int                                    _captureDeviceCountInfo;

}







- (NSNumber*)getCaptureDevices;
- (NSNumber*)initializeVariables;
- (void)checkOSSupported;








- (NSNumber*)getCaptureDeviceCount;

- (NSNumber*)getDeviceNamesFromIndex:(uint32_t)index
    DefaultName:(char*)deviceName
    WithLength:(uint32_t)deviceNameLength
    AndUniqueID:(char*)deviceUniqueID
    WithLength:(uint32_t)deviceUniqueIDLength
    AndProductID:(char*)deviceProductID
    WithLength:(uint32_t)deviceProductIDLength;

- (NSNumber*)displayCaptureSettingsDialogBoxWithDevice:
        (const char*)deviceUniqueIdUTF8
    AndTitle:(const char*)dialogTitleUTF8
    AndParentWindow:(void*) parentWindow AtX:(uint32_t)positionX
    AndY:(uint32_t) positionY;
@end

#endif  
