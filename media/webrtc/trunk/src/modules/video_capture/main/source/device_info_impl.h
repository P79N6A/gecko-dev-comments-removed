









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_DEVICE_INFO_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_DEVICE_INFO_IMPL_H_

#include "video_capture.h"

#include "map_wrapper.h"
#include "rw_lock_wrapper.h"
#include "video_capture_delay.h"

namespace webrtc
{
namespace videocapturemodule
{
class DeviceInfoImpl: public VideoCaptureModule::DeviceInfo
{
public:
    DeviceInfoImpl(const WebRtc_Word32 id);
    virtual ~DeviceInfoImpl(void);
    virtual WebRtc_Word32 NumberOfCapabilities(const char* deviceUniqueIdUTF8);
    virtual WebRtc_Word32 GetCapability(
        const char* deviceUniqueIdUTF8,
        const WebRtc_UWord32 deviceCapabilityNumber,
        VideoCaptureCapability& capability);

    virtual WebRtc_Word32 GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8,
        const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting);
    virtual WebRtc_Word32 GetOrientation(
        const char* deviceUniqueIdUTF8,
        VideoCaptureRotation& orientation);

protected:
    

    virtual WebRtc_Word32 Init()=0;
    


    virtual WebRtc_Word32 CreateCapabilityMap(const char* deviceUniqueIdUTF8)=0;

    
    WebRtc_Word32 GetExpectedCaptureDelay(const DelayValues delayValues[],
                                          const WebRtc_UWord32 sizeOfDelayValues,
                                          const char* productId,
                                          const WebRtc_UWord32 width,
                                          const WebRtc_UWord32 height);
protected:
    
    WebRtc_Word32 _id;
    MapWrapper _captureCapabilities;
    RWLockWrapper& _apiLock;
    char* _lastUsedDeviceName;
    WebRtc_UWord32 _lastUsedDeviceNameLength;
};
} 
} 
#endif 
