









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_IOS_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_IOS_H

#include "webrtc/modules/audio_device/audio_device_utility.h"
#include "webrtc/modules/audio_device/include/audio_device.h"

namespace webrtc {
class CriticalSectionWrapper;

class AudioDeviceUtilityIOS: public AudioDeviceUtility {
 public:
    AudioDeviceUtilityIOS(const int32_t id);
    AudioDeviceUtilityIOS();
    virtual ~AudioDeviceUtilityIOS();

    virtual int32_t Init();

 private:
    CriticalSectionWrapper& _critSect;
    int32_t _id;
    AudioDeviceModule::ErrorCode _lastError;
};

}  

#endif  
