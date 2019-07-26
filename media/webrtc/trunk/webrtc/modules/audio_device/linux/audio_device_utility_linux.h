









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_LINUX_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_LINUX_H

#include "webrtc/modules/audio_device/audio_device_utility.h"
#include "webrtc/modules/audio_device/include/audio_device.h"

namespace webrtc
{
class CriticalSectionWrapper;

class AudioDeviceUtilityLinux: public AudioDeviceUtility
{
public:
    AudioDeviceUtilityLinux(const int32_t id);
    virtual ~AudioDeviceUtilityLinux();

    virtual int32_t Init() OVERRIDE;

private:
    CriticalSectionWrapper& _critSect;
    int32_t _id;
};

}  

#endif  
