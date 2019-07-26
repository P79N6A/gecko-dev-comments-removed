









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_DEVICE_INFO_DS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_DEVICE_INFO_DS_H_

#include "../video_capture_impl.h"
#include "../device_info_impl.h"

#include <Dshow.h>
#include "map_wrapper.h"


namespace webrtc
{
namespace videocapturemodule
{
struct VideoCaptureCapabilityWindows: public VideoCaptureCapability
{
    WebRtc_UWord32 directShowCapabilityIndex;
    bool supportFrameRateControl;
    VideoCaptureCapabilityWindows()
    {
        directShowCapabilityIndex = 0;
        supportFrameRateControl = false;
    }

};
class DeviceInfoDS: public DeviceInfoImpl
{
public:
    
    static DeviceInfoDS* Create(const WebRtc_Word32 id);

    DeviceInfoDS(const WebRtc_Word32 id);
    virtual ~DeviceInfoDS();

    WebRtc_Word32 Init();
    virtual WebRtc_UWord32 NumberOfDevices();

    


    virtual WebRtc_Word32
        GetDeviceName(WebRtc_UWord32 deviceNumber,
                      char* deviceNameUTF8,
                      WebRtc_UWord32 deviceNameLength,
                      char* deviceUniqueIdUTF8,
                      WebRtc_UWord32 deviceUniqueIdUTF8Length,
                      char* productUniqueIdUTF8,
                      WebRtc_UWord32 productUniqueIdUTF8Length);

    


    virtual WebRtc_Word32
        DisplayCaptureSettingsDialogBox(
                                        const char* deviceUniqueIdUTF8,
                                        const char* dialogTitleUTF8,
                                        void* parentWindow,
                                        WebRtc_UWord32 positionX,
                                        WebRtc_UWord32 positionY);

    

    


    IBaseFilter * GetDeviceFilter(const char* deviceUniqueIdUTF8,
                                  char* productUniqueIdUTF8 = NULL,
                                  WebRtc_UWord32 productUniqueIdUTF8Length = 0);

    WebRtc_Word32
        GetWindowsCapability(const WebRtc_Word32 capabilityIndex,
                             VideoCaptureCapabilityWindows& windowsCapability);

    static void GetProductId(const char* devicePath,
                             char* productUniqueIdUTF8,
                             WebRtc_UWord32 productUniqueIdUTF8Length);

protected:
    WebRtc_Word32 GetDeviceInfo(WebRtc_UWord32 deviceNumber,
                                char* deviceNameUTF8,
                                WebRtc_UWord32 deviceNameLength,
                                char* deviceUniqueIdUTF8,
                                WebRtc_UWord32 deviceUniqueIdUTF8Length,
                                char* productUniqueIdUTF8,
                                WebRtc_UWord32 productUniqueIdUTF8Length);

    virtual WebRtc_Word32
        CreateCapabilityMap(const char* deviceUniqueIdUTF8);

private:
    ICreateDevEnum* _dsDevEnum;
    IEnumMoniker* _dsMonikerDevEnum;
    bool _CoUninitializeIsRequired;

};
} 
} 
#endif 
