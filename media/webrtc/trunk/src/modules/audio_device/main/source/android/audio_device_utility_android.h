













#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_ANDROID_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_ANDROID_H

#include "audio_device_utility.h"
#include "audio_device.h"

namespace webrtc
{
class CriticalSectionWrapper;

class AudioDeviceUtilityAndroid: public AudioDeviceUtility
{
public:
    AudioDeviceUtilityAndroid(const WebRtc_Word32 id);
    ~AudioDeviceUtilityAndroid();

    virtual WebRtc_Word32 Init();

private:
    CriticalSectionWrapper& _critSect;
    WebRtc_Word32 _id;
    AudioDeviceModule::ErrorCode _lastError;
};

} 

#endif  
