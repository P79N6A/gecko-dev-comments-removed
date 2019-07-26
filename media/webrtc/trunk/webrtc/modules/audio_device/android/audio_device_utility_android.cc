













#include "webrtc/modules/audio_device/android/audio_device_utility_android.h"

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc
{

AudioDeviceUtilityAndroid::AudioDeviceUtilityAndroid(const int32_t id) :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()), _id(id)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}

AudioDeviceUtilityAndroid::~AudioDeviceUtilityAndroid()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    {
        CriticalSectionScoped lock(&_critSect);
    }

    delete &_critSect;
}

int32_t AudioDeviceUtilityAndroid::Init()
{

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "  OS info: %s", "Android");

    return 0;
}

}  
