









#include "audio_device_utility_ios.h"
#include "audio_device_config.h"

#include "critical_section_wrapper.h"
#include "trace.h"

namespace webrtc {
AudioDeviceUtilityIPhone::AudioDeviceUtilityIPhone(const WebRtc_Word32 id)
:
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _lastError(AudioDeviceModule::kAdmErrNone) {
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}

AudioDeviceUtilityIPhone::~AudioDeviceUtilityIPhone() {
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    CriticalSectionScoped lock(&_critSect);

    delete &_critSect;
}

WebRtc_Word32 AudioDeviceUtilityIPhone::Init() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "  OS info: %s", "iOS");

    return 0;
}

}  
