









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_WIN_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_WIN_H

#include "audio_device_utility.h"
#include "audio_device.h"
#include <windows.h>

namespace webrtc
{
class CriticalSectionWrapper;

class AudioDeviceUtilityWindows : public AudioDeviceUtility
{
public:
    AudioDeviceUtilityWindows(const WebRtc_Word32 id);
    ~AudioDeviceUtilityWindows();

    virtual WebRtc_Word32 Init();

private:
    BOOL GetOSDisplayString(LPTSTR pszOS);

private:
    CriticalSectionWrapper&         _critSect;
    WebRtc_Word32                   _id;
    AudioDeviceModule::ErrorCode    _lastError;
};

}  

#endif  
