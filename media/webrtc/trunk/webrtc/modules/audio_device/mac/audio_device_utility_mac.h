









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_MAC_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_MAC_H

#include "webrtc/modules/audio_device/audio_device_utility.h"
#include "webrtc/modules/audio_device/include/audio_device.h"

namespace webrtc
{
class CriticalSectionWrapper;

class AudioDeviceUtilityMac: public AudioDeviceUtility
{
public:
    AudioDeviceUtilityMac(const int32_t id);
    ~AudioDeviceUtilityMac();

    virtual int32_t Init();

private:
    CriticalSectionWrapper& _critSect;
    int32_t _id;
};

}  

#endif  
