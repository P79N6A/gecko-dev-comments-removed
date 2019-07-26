









#include "webrtc/modules/audio_device/linux/audio_device_utility_linux.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc
{

AudioDeviceUtilityLinux::AudioDeviceUtilityLinux(const int32_t id) :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()), _id(id)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}

AudioDeviceUtilityLinux::~AudioDeviceUtilityLinux()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    {
        CriticalSectionScoped lock(&_critSect);

        
    }

    delete &_critSect;
}






int32_t AudioDeviceUtilityLinux::Init()
{

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "  OS info: %s", "Linux");

    return 0;
}


}  
