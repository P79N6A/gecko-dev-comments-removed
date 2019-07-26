













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
    AudioDeviceUtilityAndroid(const int32_t id);
    ~AudioDeviceUtilityAndroid();

    virtual int32_t Init();

private:
    CriticalSectionWrapper& _critSect;
    int32_t _id;
    AudioDeviceModule::ErrorCode _lastError;
};

} 

#endif  
