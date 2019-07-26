









#include "audio_device_generic.h"
#include "trace.h"

namespace webrtc {

WebRtc_Word32 AudioDeviceGeneric::SetRecordingSampleRate(
    const WebRtc_UWord32 samplesPerSec)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
        "Set recording sample rate not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceGeneric::SetPlayoutSampleRate(
    const WebRtc_UWord32 samplesPerSec)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
        "Set playout sample rate not supported on this platform");
    return -1;
}
	
WebRtc_Word32 AudioDeviceGeneric::SetLoudspeakerStatus(bool enable)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
        "Set loudspeaker status not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceGeneric::GetLoudspeakerStatus(bool& enable) const
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
        "Get loudspeaker status not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceGeneric::ResetAudioDevice()
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
        "Reset audio device not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceGeneric::SoundDeviceControl(unsigned int par1,
    unsigned int par2, unsigned int par3, unsigned int par4)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
        "Sound device control not supported on this platform");
    return -1;
}

int32_t AudioDeviceGeneric::EnableBuiltInAEC(bool enable)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
        "Windows AEC not supported on this platform");
    return -1;
}

bool AudioDeviceGeneric::BuiltInAECIsEnabled() const
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
        "Windows AEC not supported on this platform");
    return false;
}

}  

