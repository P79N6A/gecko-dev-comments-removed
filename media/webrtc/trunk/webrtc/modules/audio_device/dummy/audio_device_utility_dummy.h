









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_DUMMY_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_DUMMY_H

#include "audio_device_utility.h"
#include "audio_device.h"

namespace webrtc
{
class CriticalSectionWrapper;

class AudioDeviceUtilityDummy: public AudioDeviceUtility
{
public:
    AudioDeviceUtilityDummy(const int32_t id) {}
    ~AudioDeviceUtilityDummy() {}

    virtual int32_t Init() { return 0; }
};

} 

#endif  
