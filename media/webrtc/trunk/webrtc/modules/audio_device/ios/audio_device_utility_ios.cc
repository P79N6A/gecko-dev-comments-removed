









#include "webrtc/modules/audio_device/audio_device_config.h"
#include "webrtc/modules/audio_device/ios/audio_device_utility_ios.h"

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {
AudioDeviceUtilityIPhone::AudioDeviceUtilityIPhone(const int32_t id)
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

int32_t AudioDeviceUtilityIPhone::Init() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "  OS info: %s", "iOS");

    return 0;
}

}  
