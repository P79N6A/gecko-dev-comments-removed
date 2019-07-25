









#include "ref_count.h"
#include "video_capture_windows.h"
#include "trace.h"

namespace webrtc
{
namespace videocapturemodule
{
VideoCaptureModule* VideoCaptureImpl::Create(
    const WebRtc_Word32 id,
    const char* deviceUniqueIdUTF8)
{

    if (deviceUniqueIdUTF8 == NULL)
    {
        return NULL;
    }

    char productId[kVideoCaptureProductIdLength];
    videocapturemodule::DeviceInfoWindows::GetProductId(deviceUniqueIdUTF8,
                                                        productId,
                                                        sizeof(productId));
    
    RefCountImpl<videocapturemodule::VideoCaptureDS>* newCaptureModule =
        new RefCountImpl<videocapturemodule::VideoCaptureDS>(id);

    if (newCaptureModule->Init(id, deviceUniqueIdUTF8) != 0)
    {
        delete newCaptureModule;
        newCaptureModule = NULL;
    }
    return newCaptureModule;
}
} 
} 
