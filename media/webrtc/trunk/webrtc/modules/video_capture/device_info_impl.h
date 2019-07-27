









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_DEVICE_INFO_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_DEVICE_INFO_IMPL_H_

#include <vector>

#include "webrtc/modules/video_capture/include/video_capture.h"
#include "webrtc/modules/video_capture/video_capture_delay.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"

namespace webrtc
{
namespace videocapturemodule
{
class DeviceInfoImpl: public VideoCaptureModule::DeviceInfo
{
public:
    DeviceInfoImpl(const int32_t id);
    virtual ~DeviceInfoImpl(void);
    virtual int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8);
    virtual int32_t GetCapability(
        const char* deviceUniqueIdUTF8,
        const uint32_t deviceCapabilityNumber,
        VideoCaptureCapability& capability);

    virtual int32_t GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8,
        const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting);
    virtual int32_t GetOrientation(
        const char* deviceUniqueIdUTF8,
        VideoCaptureRotation& orientation);

protected:
    

    virtual int32_t Init()=0;
    virtual int32_t Refresh() { return 0; }
    


    virtual int32_t CreateCapabilityMap(const char* deviceUniqueIdUTF8)=0;

    
    int32_t GetExpectedCaptureDelay(const DelayValues delayValues[],
                                    const uint32_t sizeOfDelayValues,
                                    const char* productId,
                                    const uint32_t width,
                                    const uint32_t height);
protected:
    
    int32_t _id;
    typedef std::vector<VideoCaptureCapability> VideoCaptureCapabilities;
    VideoCaptureCapabilities _captureCapabilities;
    RWLockWrapper& _apiLock;
    char* _lastUsedDeviceName;
    uint32_t _lastUsedDeviceNameLength;
};
}  
}  
#endif 
