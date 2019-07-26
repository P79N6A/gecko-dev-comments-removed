









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_H_

#import <QTKit/QTKit.h>
#include <stdio.h>

#include "webrtc/modules/video_capture/device_info_impl.h"
#include "webrtc/modules/video_capture/mac/qtkit/video_capture_qtkit_utility.h"
#include "webrtc/modules/video_capture/video_capture_impl.h"

@class VideoCaptureMacQTKitObjC;
@class VideoCaptureMacQTKitInfoObjC;

namespace webrtc
{
namespace videocapturemodule
{

class VideoCaptureMacQTKit : public VideoCaptureImpl
{
public:
    VideoCaptureMacQTKit(const int32_t id);
    virtual ~VideoCaptureMacQTKit();

    







    static void Destroy(VideoCaptureModule* module);

    int32_t Init(const int32_t id, const char* deviceUniqueIdUTF8);


    
    virtual int32_t StartCapture(
        const VideoCaptureCapability& capability);
    virtual int32_t StopCapture();

    

    virtual bool CaptureStarted();

    int32_t CaptureSettings(VideoCaptureCapability& settings);

protected:
    
    int32_t SetCameraOutput();

private:
    VideoCaptureMacQTKitObjC*        _captureDevice;
    VideoCaptureMacQTKitInfoObjC*    _captureInfo;
    bool                    _isCapturing;
    int32_t            _id;
    int32_t            _captureWidth;
    int32_t            _captureHeight;
    int32_t            _captureFrameRate;
    char                     _currentDeviceNameUTF8[MAX_NAME_LENGTH];
    char                     _currentDeviceUniqueIdUTF8[MAX_NAME_LENGTH];
    char                     _currentDeviceProductUniqueIDUTF8[MAX_NAME_LENGTH];
    int32_t            _frameCount;
};
}  
}  

#endif  
