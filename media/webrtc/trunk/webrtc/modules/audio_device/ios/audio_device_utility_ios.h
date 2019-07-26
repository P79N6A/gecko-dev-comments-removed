









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_IPHONE_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_IPHONE_H

#include "audio_device_utility.h"
#include "audio_device.h"

namespace webrtc {
class CriticalSectionWrapper;

class AudioDeviceUtilityIPhone: public AudioDeviceUtility {
public:
    AudioDeviceUtilityIPhone(const int32_t id);
    AudioDeviceUtilityIPhone();
    virtual ~AudioDeviceUtilityIPhone();

    virtual int32_t Init();

private:
    CriticalSectionWrapper& _critSect;
    int32_t _id;
    AudioDeviceModule::ErrorCode _lastError;
};

}  

#endif  
