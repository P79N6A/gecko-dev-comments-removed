









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_H_

#import <QTKit/QTKit.h>

#include <stdio.h>

#include "../../video_capture_impl.h"
#include "video_capture_qtkit_utility.h"
#include "../../device_info_impl.h"



@class VideoCaptureMacQTKitObjC;
@class VideoCaptureMacQTKitInfoObjC;

namespace webrtc
{
namespace videocapturemodule
{

class VideoCaptureMacQTKit : public VideoCaptureImpl
{
public:
    VideoCaptureMacQTKit(const WebRtc_Word32 id);
    virtual ~VideoCaptureMacQTKit();

    







    static void Destroy(VideoCaptureModule* module);

    WebRtc_Word32 Init(const WebRtc_Word32 id,
                       const char* deviceUniqueIdUTF8);


    
    virtual WebRtc_Word32 StartCapture(
        const VideoCaptureCapability& capability);
    virtual WebRtc_Word32 StopCapture();

    

    virtual bool CaptureStarted();

    WebRtc_Word32 CaptureSettings(VideoCaptureCapability& settings);

protected:
    
    WebRtc_Word32 SetCameraOutput();

private:
    VideoCaptureMacQTKitObjC*        _captureDevice;
    VideoCaptureMacQTKitInfoObjC*    _captureInfo;
    bool                    _isCapturing;
    WebRtc_Word32            _id;
    WebRtc_Word32            _captureWidth;
    WebRtc_Word32            _captureHeight;
    WebRtc_Word32            _captureFrameRate;
    char                     _currentDeviceNameUTF8[MAX_NAME_LENGTH];
    char                     _currentDeviceUniqueIdUTF8[MAX_NAME_LENGTH];
    char                     _currentDeviceProductUniqueIDUTF8[MAX_NAME_LENGTH];
    WebRtc_Word32            _frameCount;
};
}  
}  

#endif  
