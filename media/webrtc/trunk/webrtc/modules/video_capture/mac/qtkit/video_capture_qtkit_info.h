









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_INFO_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_INFO_H_

#include "webrtc/modules/video_capture/device_info_impl.h"
#include "webrtc/modules/video_capture/mac/qtkit/video_capture_qtkit_utility.h"
#include "webrtc/modules/video_capture/video_capture_impl.h"


@class VideoCaptureMacQTKitInfoObjC;

namespace webrtc
{
namespace videocapturemodule
{

class VideoCaptureMacQTKitInfo: public DeviceInfoImpl
{
public:

   VideoCaptureMacQTKitInfo(const int32_t id);
    virtual ~VideoCaptureMacQTKitInfo();

    int32_t Init();

    virtual uint32_t NumberOfDevices();

    








    virtual int32_t GetDeviceName(
        uint32_t deviceNumber, char* deviceNameUTF8,
        uint32_t deviceNameLength, char* deviceUniqueIdUTF8,
        uint32_t deviceUniqueIdUTF8Length,
        char* productUniqueIdUTF8 = 0,
        uint32_t productUniqueIdUTF8Length = 0);

    


    virtual int32_t NumberOfCapabilities(
        const char* deviceUniqueIdUTF8);

    


    virtual int32_t GetCapability(
        const char* deviceUniqueIdUTF8,
        const uint32_t deviceCapabilityNumber,
        VideoCaptureCapability& capability);

    



    virtual int32_t GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8,
        const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting);

    


    virtual int32_t DisplayCaptureSettingsDialogBox(
        const char* deviceUniqueIdUTF8,
        const char* dialogTitleUTF8, void* parentWindow,
        uint32_t positionX, uint32_t positionY);

protected:
    virtual int32_t CreateCapabilityMap(
        const char* deviceUniqueIdUTF8);

    VideoCaptureMacQTKitInfoObjC*    _captureInfo;
};
}  
}  

#endif  
