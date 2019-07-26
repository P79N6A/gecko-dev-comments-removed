









#include "webrtc/modules/audio_device/mac/audio_device_utility_mac.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc
{

AudioDeviceUtilityMac::AudioDeviceUtilityMac(const int32_t id) :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}





AudioDeviceUtilityMac::~AudioDeviceUtilityMac()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    {
        CriticalSectionScoped lock(&_critSect);

        
    }

    delete &_critSect;
}

int32_t AudioDeviceUtilityMac::Init()
{

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "  OS info: %s", "OS X");

    return 0;
}

}  
