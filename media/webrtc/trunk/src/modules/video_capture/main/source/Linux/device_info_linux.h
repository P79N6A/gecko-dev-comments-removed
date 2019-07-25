









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_LINUX_DEVICE_INFO_LINUX_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_LINUX_DEVICE_INFO_LINUX_H_

#include "../video_capture_impl.h"
#include "../device_info_impl.h"

namespace webrtc
{
namespace videocapturemodule
{
class DeviceInfoLinux: public DeviceInfoImpl
{
public:
    DeviceInfoLinux(const WebRtc_Word32 id);
    virtual ~DeviceInfoLinux();
    virtual WebRtc_UWord32 NumberOfDevices();
    virtual WebRtc_Word32 GetDeviceName(
        WebRtc_UWord32 deviceNumber,
        char* deviceNameUTF8,
        WebRtc_UWord32 deviceNameLength,
        char* deviceUniqueIdUTF8,
        WebRtc_UWord32 deviceUniqueIdUTF8Length,
        char* productUniqueIdUTF8=0,
        WebRtc_UWord32 productUniqueIdUTF8Length=0);
    


    virtual WebRtc_Word32 CreateCapabilityMap (const char* deviceUniqueIdUTF8);
    virtual WebRtc_Word32 DisplayCaptureSettingsDialogBox(
        const char* ,
        const char* ,
        void* ,
        WebRtc_UWord32 ,
        WebRtc_UWord32 ) { return -1;}
    WebRtc_Word32 FillCapabilityMap(int fd);
    WebRtc_Word32 Init();
private:

    bool IsDeviceNameMatches(const char* name, const char* deviceUniqueIdUTF8);
};
} 
} 
#endif 
