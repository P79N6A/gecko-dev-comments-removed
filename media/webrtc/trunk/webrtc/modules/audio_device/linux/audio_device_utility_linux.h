









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_LINUX_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_LINUX_H

#include "audio_device_utility.h"
#include "audio_device.h"

namespace webrtc
{
class CriticalSectionWrapper;

class AudioDeviceUtilityLinux: public AudioDeviceUtility
{
public:
    AudioDeviceUtilityLinux(const int32_t id);
    ~AudioDeviceUtilityLinux();

    virtual int32_t Init();

private:
    CriticalSectionWrapper& _critSect;
    int32_t _id;
};

} 

#endif  
