









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_INFO_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_INFO_H_

#include "../../video_capture_impl.h"
#include "../../device_info_impl.h"
#include "video_capture_qtkit_utility.h"

#include "map_wrapper.h"


@class VideoCaptureMacQTKitInfoObjC;

namespace webrtc
{
namespace videocapturemodule
{

class VideoCaptureMacQTKitInfo: public DeviceInfoImpl
{
public:

   VideoCaptureMacQTKitInfo(const WebRtc_Word32 id);
    virtual ~VideoCaptureMacQTKitInfo();

    WebRtc_Word32 Init();

    virtual WebRtc_UWord32 NumberOfDevices();

    








    virtual WebRtc_Word32 GetDeviceName(
        WebRtc_UWord32 deviceNumber, char* deviceNameUTF8,
        WebRtc_UWord32 deviceNameLength, char* deviceUniqueIdUTF8,
        WebRtc_UWord32 deviceUniqueIdUTF8Length,
        char* productUniqueIdUTF8 = 0,
        WebRtc_UWord32 productUniqueIdUTF8Length = 0);

    


    virtual WebRtc_Word32 NumberOfCapabilities(
        const char* deviceUniqueIdUTF8);

    


    virtual WebRtc_Word32 GetCapability(
        const char* deviceUniqueIdUTF8,
        const WebRtc_UWord32 deviceCapabilityNumber,
        VideoCaptureCapability& capability);

    



    virtual WebRtc_Word32 GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8,
        const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting);

    


    virtual WebRtc_Word32 DisplayCaptureSettingsDialogBox(
        const char* deviceUniqueIdUTF8,
        const char* dialogTitleUTF8, void* parentWindow,
        WebRtc_UWord32 positionX, WebRtc_UWord32 positionY);

protected:
    virtual WebRtc_Word32 CreateCapabilityMap(
        const char* deviceUniqueIdUTF8);

    VideoCaptureMacQTKitInfoObjC*    _captureInfo;
};
}  
}  

#endif  
