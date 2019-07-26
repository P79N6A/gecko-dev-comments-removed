









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
    AudioDeviceUtilityLinux(const WebRtc_Word32 id);
    ~AudioDeviceUtilityLinux();

    virtual WebRtc_Word32 Init();

private:
    CriticalSectionWrapper& _critSect;
    WebRtc_Word32 _id;
};

} 

#endif  
