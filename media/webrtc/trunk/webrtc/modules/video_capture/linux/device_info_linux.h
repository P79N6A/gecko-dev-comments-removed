









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_LINUX_DEVICE_INFO_LINUX_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_LINUX_DEVICE_INFO_LINUX_H_

#include "webrtc/modules/video_capture/device_info_impl.h"
#include "webrtc/modules/video_capture/video_capture_impl.h"

namespace webrtc
{
namespace videocapturemodule
{
class DeviceInfoLinux: public DeviceInfoImpl
{
public:
    DeviceInfoLinux(const int32_t id);
    virtual ~DeviceInfoLinux();
    virtual uint32_t NumberOfDevices();
    virtual int32_t GetDeviceName(
        uint32_t deviceNumber,
        char* deviceNameUTF8,
        uint32_t deviceNameLength,
        char* deviceUniqueIdUTF8,
        uint32_t deviceUniqueIdUTF8Length,
        char* productUniqueIdUTF8=0,
        uint32_t productUniqueIdUTF8Length=0);
    


    virtual int32_t CreateCapabilityMap (const char* deviceUniqueIdUTF8);
    virtual int32_t DisplayCaptureSettingsDialogBox(
        const char* ,
        const char* ,
        void* ,
        uint32_t ,
        uint32_t ) { return -1;}
    int32_t FillCapabilities(int fd);
    int32_t Init();
private:

    bool IsDeviceNameMatches(const char* name, const char* deviceUniqueIdUTF8);
};
}  
}  
#endif 
