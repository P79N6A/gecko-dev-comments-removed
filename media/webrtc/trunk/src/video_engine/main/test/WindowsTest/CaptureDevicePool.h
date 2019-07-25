









#pragma once

#include "common_types.h"

#include "vie_base.h"
#include "vie_capture.h"
#include "vie_file.h"
#include "map_wrapper.h"

namespace webrtc {
class CriticalSectionWrapper;
}
using namespace webrtc;
class CaptureDevicePool
{
public:
    CaptureDevicePool(VideoEngine* videoEngine);
    ~CaptureDevicePool(void);
    WebRtc_Word32 GetCaptureDevice(int& captureId, const char uniqeDeviceName[256]);
    WebRtc_Word32 ReturnCaptureDevice(int captureId);

    private: 
        struct DeviceItem
        {
            int captureId;
            WebRtc_Word32 refCount;
            char uniqeDeviceName[256];
            DeviceItem()
            {
                captureId=-1;
                refCount=0;
            }
        };
        CriticalSectionWrapper& _critSect;
        ViECapture* _vieCapture;
        ViEFile*    _vieFile;
        MapWrapper _deviceMap;

};
