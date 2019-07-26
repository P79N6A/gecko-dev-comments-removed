









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_DUMMY_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_DUMMY_H

#include "webrtc/modules/audio_device/audio_device_utility.h"
#include "webrtc/modules/audio_device/include/audio_device.h"

namespace webrtc
{
class CriticalSectionWrapper;

class AudioDeviceUtilityDummy: public AudioDeviceUtility
{
public:
    AudioDeviceUtilityDummy(const int32_t id) {}
    virtual ~AudioDeviceUtilityDummy() {}

    virtual int32_t Init() OVERRIDE;
};
}  

#endif  
