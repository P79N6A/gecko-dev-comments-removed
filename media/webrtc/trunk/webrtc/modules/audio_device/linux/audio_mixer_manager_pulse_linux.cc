









#include <cassert>

#include "audio_mixer_manager_pulse_linux.h"
#include "trace.h"

extern webrtc_adm_linux_pulse::PulseAudioSymbolTable PaSymbolTable;




#define LATE(sym) \
  LATESYM_GET(webrtc_adm_linux_pulse::PulseAudioSymbolTable, &PaSymbolTable, sym)

namespace webrtc
{

enum { kMaxRetryOnFailure = 2 };

AudioMixerManagerLinuxPulse::AudioMixerManagerLinuxPulse(const WebRtc_Word32 id) :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _paOutputDeviceIndex(-1),
    _paInputDeviceIndex(-1),
    _paPlayStream(NULL),
    _paRecStream(NULL),
    _paMainloop(NULL),
    _paContext(NULL),
    _paVolume(0),
    _paMute(0),
    _paVolSteps(0),
    _paSpeakerMute(false),
    _paSpeakerVolume(PA_VOLUME_NORM),
    _paChannels(0),
    _paObjectsSet(false),
    _callbackValues(false)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s constructed", __FUNCTION__);
}

AudioMixerManagerLinuxPulse::~AudioMixerManagerLinuxPulse()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destructed", __FUNCTION__);

    Close();

    delete &_critSect;
}





WebRtc_Word32 AudioMixerManagerLinuxPulse::SetPulseAudioObjects(
    pa_threaded_mainloop* mainloop,
    pa_context* context)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!mainloop || !context)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  could not set PulseAudio objects for mixer");
        return -1;
    }

    _paMainloop = mainloop;
    _paContext = context;
    _paObjectsSet = true;

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  the PulseAudio objects for the mixer has been set");

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::Close()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    CloseSpeaker();
    CloseMicrophone();

    _paMainloop = NULL;
    _paContext = NULL;
    _paObjectsSet = false;

    return 0;

}

WebRtc_Word32 AudioMixerManagerLinuxPulse::CloseSpeaker()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    
    _paOutputDeviceIndex = -1;
    _paPlayStream = NULL;

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::CloseMicrophone()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    
    _paInputDeviceIndex = -1;
    _paRecStream = NULL;

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::SetPlayStream(pa_stream* playStream)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::SetPlayStream(playStream)");

    CriticalSectionScoped lock(&_critSect);
    _paPlayStream = playStream;
    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::SetRecStream(pa_stream* recStream)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::SetRecStream(recStream)");

    CriticalSectionScoped lock(&_critSect);
    _paRecStream = recStream;
    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::OpenSpeaker(
    WebRtc_UWord16 deviceIndex)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::OpenSpeaker(deviceIndex=%d)",
                 deviceIndex);

    CriticalSectionScoped lock(&_critSect);

    
    
    if (!_paObjectsSet)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PulseAudio objects has not been set");
        return -1;
    }

    
    
    _paOutputDeviceIndex = deviceIndex;

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  the output mixer device is now open");

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::OpenMicrophone(
    WebRtc_UWord16 deviceIndex)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::OpenMicrophone(deviceIndex=%d)",
                 deviceIndex);

    CriticalSectionScoped lock(&_critSect);

    
    
    if (!_paObjectsSet)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PulseAudio objects have not been set");
        return -1;
    }

    
    
    _paInputDeviceIndex = deviceIndex;

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  the input mixer device is now open");

    return 0;
}

bool AudioMixerManagerLinuxPulse::SpeakerIsInitialized() const
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    return (_paOutputDeviceIndex != -1);
}

bool AudioMixerManagerLinuxPulse::MicrophoneIsInitialized() const
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    return (_paInputDeviceIndex != -1);
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::SetSpeakerVolume(
    WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::SetSpeakerVolume(volume=%u)",
                 volume);

    CriticalSectionScoped lock(&_critSect);

    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    bool setFailed(false);

    if (_paPlayStream && (LATE(pa_stream_get_state)(_paPlayStream)
        != PA_STREAM_UNCONNECTED))
    {
        
        PaLock();

        
        const pa_sample_spec *spec =
            LATE(pa_stream_get_sample_spec)(_paPlayStream);
        if (!spec)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  could not get sample specification");
            PaUnLock();
            return -1;
        }

        
        pa_cvolume cVolumes;
        LATE(pa_cvolume_set)(&cVolumes, spec->channels, volume);

        pa_operation* paOperation = NULL;
        paOperation = LATE(pa_context_set_sink_input_volume)(
            _paContext,
            LATE(pa_stream_get_index)(_paPlayStream),
            &cVolumes,
            PaSetVolumeCallback, NULL);
        if (!paOperation)
        {
            setFailed = true;
        }

        
        LATE(pa_operation_unref)(paOperation);

        PaUnLock();
    } else
    {
        
        
        _paSpeakerVolume = volume;
    }

    if (setFailed)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " could not set speaker volume, error%d",
                     LATE(pa_context_errno)(_paContext));

        return -1;
    }

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::SpeakerVolume(WebRtc_UWord32& volume) const
{

    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    if (_paPlayStream && (LATE(pa_stream_get_state)(_paPlayStream)
        != PA_STREAM_UNCONNECTED))
    {
        
        if (!GetSinkInputInfo())
          return -1;

        volume = static_cast<WebRtc_UWord32> (_paVolume);
        ResetCallbackVariables();
    } else
    {
        volume = _paSpeakerVolume;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerLinuxPulse::SpeakerVolume() => vol=%i",
                 volume);

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const
{

    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    
    
    maxVolume = static_cast<WebRtc_UWord32> (PA_VOLUME_NORM);

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::MinSpeakerVolume(WebRtc_UWord32& minVolume) const
{

    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    minVolume = static_cast<WebRtc_UWord32> (PA_VOLUME_MUTED);

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const
{

    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    
    
    stepSize = 1;

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerLinuxPulse::SpeakerVolumeStepSize() => "
                 "size=%i, stepSize");

    
    ResetCallbackVariables();

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::SpeakerVolumeIsAvailable(bool& available)
{
    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    
    available = true;

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::SpeakerMuteIsAvailable(bool& available)
{
    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    
    available = true;

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::SetSpeakerMute(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::SetSpeakerMute(enable=%u)",
                 enable);

    CriticalSectionScoped lock(&_critSect);

    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    bool setFailed(false);

    if (_paPlayStream && (LATE(pa_stream_get_state)(_paPlayStream)
        != PA_STREAM_UNCONNECTED))
    {
        
        PaLock();

        pa_operation* paOperation = NULL;
        paOperation = LATE(pa_context_set_sink_input_mute)(
            _paContext,
            LATE(pa_stream_get_index)(_paPlayStream),
            (int) enable,
            PaSetVolumeCallback,
            NULL);
        if (!paOperation)
        {
            setFailed = true;
        }

        
        LATE(pa_operation_unref)(paOperation);

        PaUnLock();
    } else
    {
        
        
        _paSpeakerMute = enable;
    }

    if (setFailed)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " could not mute speaker, error%d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::SpeakerMute(bool& enabled) const
{

    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    if (_paPlayStream && (LATE(pa_stream_get_state)(_paPlayStream)
        != PA_STREAM_UNCONNECTED))
    {
        
        if (!GetSinkInputInfo())
          return -1;

        enabled = static_cast<bool> (_paMute);
        ResetCallbackVariables();
    } else
    {
        enabled = _paSpeakerMute;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerLinuxPulse::SpeakerMute() => "
                 "enabled=%i, enabled");

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::StereoPlayoutIsAvailable(bool& available)
{
    if (_paOutputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  output device index has not been set");
        return -1;
    }

    uint32_t deviceIndex = (uint32_t) _paOutputDeviceIndex;

    PaLock();

    
    
    
    if (_paPlayStream && (LATE(pa_stream_get_state)(_paPlayStream)
        != PA_STREAM_UNCONNECTED))
    {
        deviceIndex = LATE(pa_stream_get_device_index)(_paPlayStream);
    }

    PaUnLock();

    if (!GetSinkInfoByIndex(deviceIndex))
      return -1;

    available = static_cast<bool> (_paChannels == 2);

    
    ResetCallbackVariables();

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::StereoRecordingIsAvailable(bool& available)
{
    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    uint32_t deviceIndex = (uint32_t) _paInputDeviceIndex;

    PaLock();

    
    
    
    if (_paRecStream && (LATE(pa_stream_get_state)(_paRecStream)
        != PA_STREAM_UNCONNECTED))
    {
        deviceIndex = LATE(pa_stream_get_device_index)(_paRecStream);
    }

    pa_operation* paOperation = NULL;
    ResetCallbackVariables();

    
    
    paOperation = LATE(pa_context_get_source_info_by_index)(
        _paContext, deviceIndex,
        PaSourceInfoCallback,
        (void*) this);

    WaitForOperationCompletion(paOperation);
    PaUnLock();

    if (!_callbackValues)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "Error getting number of input channels: %d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    available = static_cast<bool> (_paChannels == 2);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerLinuxPulse::StereoRecordingIsAvailable()"
                 " => available=%i, available");

    
    ResetCallbackVariables();

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::MicrophoneMuteIsAvailable(
    bool& available)
{
    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    
    available = true;

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::SetMicrophoneMute(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::SetMicrophoneMute(enable=%u)",
                 enable);

    CriticalSectionScoped lock(&_critSect);

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    bool setFailed(false);
    pa_operation* paOperation = NULL;
    ResetCallbackVariables();

    uint32_t deviceIndex = (uint32_t) _paInputDeviceIndex;

    PaLock();

    
    
    
    if (_paRecStream && (LATE(pa_stream_get_state)(_paRecStream)
        != PA_STREAM_UNCONNECTED))
    {
        deviceIndex = LATE(pa_stream_get_device_index)(_paRecStream);
    }

    
    paOperation = LATE(pa_context_set_source_mute_by_index)(
        _paContext, deviceIndex,
        enable,
        PaSetVolumeCallback, NULL);

    if (!paOperation)
    {
        setFailed = true;
    }

    
    LATE(pa_operation_unref)(paOperation);

    PaUnLock();

    
    ResetCallbackVariables();

    if (setFailed)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " could not mute microphone, error%d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::MicrophoneMute(bool& enabled) const
{

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    uint32_t deviceIndex = (uint32_t) _paInputDeviceIndex;

    PaLock();

    
    
    
    if (_paRecStream && (LATE(pa_stream_get_state)(_paRecStream)
        != PA_STREAM_UNCONNECTED))
    {
        deviceIndex = LATE(pa_stream_get_device_index)(_paRecStream);
    }

    PaUnLock();

    if (!GetSourceInfoByIndex(deviceIndex))
      return -1;

    enabled = static_cast<bool> (_paMute);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerLinuxPulse::MicrophoneMute() =>"
                 " enabled=%i, enabled");

    
    ResetCallbackVariables();

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::MicrophoneBoostIsAvailable(bool& available)
{
    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    
    
    
    available = false;

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::SetMicrophoneBoost(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::SetMicrophoneBoost(enable=%u)",
                 enable);

    CriticalSectionScoped lock(&_critSect);

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    
    bool available(false);
    MicrophoneBoostIsAvailable(available);
    if (!available)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  it is not possible to enable microphone boost");
        return -1;
    }

    

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::MicrophoneBoost(bool& enabled) const
{

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    
    enabled = false;

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::MicrophoneVolumeIsAvailable(
    bool& available)
{
    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    
    available = true;

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::SetMicrophoneVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerLinuxPulse::SetMicrophoneVolume(volume=%u)",
                 volume);

    CriticalSectionScoped lock(&_critSect);

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    
    
    

    
    
    
    
    
    
    

    uint32_t deviceIndex = (uint32_t) _paInputDeviceIndex;

    PaLock();

    
    
    
    if (_paRecStream && (LATE(pa_stream_get_state)(_paRecStream)
        != PA_STREAM_UNCONNECTED))
    {
        deviceIndex = LATE(pa_stream_get_device_index)(_paRecStream);
    }

    bool setFailed(false);
    pa_operation* paOperation = NULL;
    ResetCallbackVariables();

    
    paOperation
        = LATE(pa_context_get_source_info_by_index)(_paContext, deviceIndex,
                                                    PaSourceInfoCallback,
                                                    (void*) this);

    WaitForOperationCompletion(paOperation);

    if (!_callbackValues)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "Error getting input channels: %d",
                     LATE(pa_context_errno)(_paContext));
        PaUnLock();
        return -1;
    }

    WebRtc_UWord8 channels = _paChannels;
    ResetCallbackVariables();

    pa_cvolume cVolumes;
    LATE(pa_cvolume_set)(&cVolumes, channels, volume);

    
    paOperation
        = LATE(pa_context_set_source_volume_by_index)(_paContext, deviceIndex,
                                                      &cVolumes,
                                                      PaSetVolumeCallback, NULL);

    if (!paOperation)
    {
        setFailed = true;
    }

    
    LATE(pa_operation_unref)(paOperation);

    PaUnLock();

    
    ResetCallbackVariables();

    if (setFailed)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " could not set microphone volume, error%d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::MicrophoneVolume(WebRtc_UWord32& volume) const
{

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    uint32_t deviceIndex = (uint32_t) _paInputDeviceIndex;

    PaLock();

    
    
    
    if (_paRecStream && (LATE(pa_stream_get_state)(_paRecStream)
        != PA_STREAM_UNCONNECTED))
    {
        deviceIndex = LATE(pa_stream_get_device_index)(_paRecStream);
    }

    PaUnLock();

    if (!GetSourceInfoByIndex(deviceIndex))
      return -1;

    volume = static_cast<WebRtc_UWord32> (_paVolume);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerLinuxPulse::MicrophoneVolume() => vol=%i, volume");

    
    ResetCallbackVariables();

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const
{

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    
    
    
    maxVolume = static_cast<WebRtc_UWord32> (PA_VOLUME_NORM);

    return 0;
}

WebRtc_Word32
AudioMixerManagerLinuxPulse::MinMicrophoneVolume(WebRtc_UWord32& minVolume) const
{

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    minVolume = static_cast<WebRtc_UWord32> (PA_VOLUME_MUTED);

    return 0;
}

WebRtc_Word32 AudioMixerManagerLinuxPulse::MicrophoneVolumeStepSize(
    WebRtc_UWord16& stepSize) const
{

    if (_paInputDeviceIndex == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  input device index has not been set");
        return -1;
    }

    uint32_t deviceIndex = (uint32_t) _paInputDeviceIndex;

    PaLock();

    
    
    
    if (_paRecStream && (LATE(pa_stream_get_state)(_paRecStream)
        != PA_STREAM_UNCONNECTED))
    {
        deviceIndex = LATE(pa_stream_get_device_index)(_paRecStream);
    }

    pa_operation* paOperation = NULL;
    ResetCallbackVariables();

    
    paOperation
        = LATE(pa_context_get_source_info_by_index)(_paContext, deviceIndex,
                                                    PaSourceInfoCallback,
                                                    (void*) this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    if (!_callbackValues)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "Error getting step size: %d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    stepSize = static_cast<WebRtc_UWord16> ((PA_VOLUME_NORM + 1) / _paVolSteps);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerLinuxPulse::MicrophoneVolumeStepSize()"
                 " => size=%i, stepSize");

    
    ResetCallbackVariables();

    return 0;
}





void AudioMixerManagerLinuxPulse::PaSinkInfoCallback(pa_context *,
                                                     const pa_sink_info *i,
                                                     int eol, void *pThis)
{
    static_cast<AudioMixerManagerLinuxPulse*> (pThis)-> PaSinkInfoCallbackHandler(
        i, eol);
}

void AudioMixerManagerLinuxPulse::PaSinkInputInfoCallback(
    pa_context *,
    const pa_sink_input_info *i,
    int eol, void *pThis)
{
    static_cast<AudioMixerManagerLinuxPulse*> (pThis)->
        PaSinkInputInfoCallbackHandler(i, eol);
}


void AudioMixerManagerLinuxPulse::PaSourceInfoCallback(pa_context *,
                                                       const pa_source_info *i,
                                                       int eol, void *pThis)
{
    static_cast<AudioMixerManagerLinuxPulse*> (pThis)->
        PaSourceInfoCallbackHandler(i, eol);
}

void AudioMixerManagerLinuxPulse::PaSetVolumeCallback(pa_context * c,
                                                      int success, void *)
{
    if (!success)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
                     " failed to set volume");
    }
}

void AudioMixerManagerLinuxPulse::PaSinkInfoCallbackHandler(
    const pa_sink_info *i,
    int eol)
{
    if (eol)
    {
        
        LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
        return;
    }

    _callbackValues = true;
    _paChannels = i->channel_map.channels; 
    pa_volume_t paVolume = PA_VOLUME_MUTED; 
    for (int j = 0; j < _paChannels; ++j)
    {
        if (paVolume < i->volume.values[j])
        {
            paVolume = i->volume.values[j];
        }
    }
    _paVolume = paVolume; 
    _paMute = i->mute; 

    
    
    
    _paVolSteps = PA_VOLUME_NORM + 1;
}

void AudioMixerManagerLinuxPulse::PaSinkInputInfoCallbackHandler(
    const pa_sink_input_info *i,
    int eol)
{
    if (eol)
    {
        
        LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
        return;
    }

    _callbackValues = true;
    _paChannels = i->channel_map.channels; 
    pa_volume_t paVolume = PA_VOLUME_MUTED; 
    for (int j = 0; j < _paChannels; ++j)
    {
        if (paVolume < i->volume.values[j])
        {
            paVolume = i->volume.values[j];
        }
    }
    _paVolume = paVolume; 
    _paMute = i->mute; 
}

void AudioMixerManagerLinuxPulse::PaSourceInfoCallbackHandler(
    const pa_source_info *i,
    int eol)
{
    if (eol)
    {
        
        LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
        return;
    }

    _callbackValues = true;
    _paChannels = i->channel_map.channels; 
    pa_volume_t paVolume = PA_VOLUME_MUTED; 
    for (int j = 0; j < _paChannels; ++j)
    {
        if (paVolume < i->volume.values[j])
        {
            paVolume = i->volume.values[j];
        }
    }
    _paVolume = paVolume; 
    _paMute = i->mute; 

    
    
    
    _paVolSteps = PA_VOLUME_NORM + 1;
}

void AudioMixerManagerLinuxPulse::ResetCallbackVariables() const
{
    _paVolume = 0;
    _paMute = 0;
    _paVolSteps = 0;
    _paChannels = 0;
    _callbackValues = false;
}

void AudioMixerManagerLinuxPulse::WaitForOperationCompletion(
    pa_operation* paOperation) const
{
    while (LATE(pa_operation_get_state)(paOperation) == PA_OPERATION_RUNNING)
    {
        LATE(pa_threaded_mainloop_wait)(_paMainloop);
    }

    LATE(pa_operation_unref)(paOperation);
}

void AudioMixerManagerLinuxPulse::PaLock() const
{
    LATE(pa_threaded_mainloop_lock)(_paMainloop);
}

void AudioMixerManagerLinuxPulse::PaUnLock() const
{
    LATE(pa_threaded_mainloop_unlock)(_paMainloop);
}

bool AudioMixerManagerLinuxPulse::GetSinkInputInfo() const {
  pa_operation* paOperation = NULL;
  ResetCallbackVariables();

  PaLock();
  for (int retries = 0; retries < kMaxRetryOnFailure && !_callbackValues;
       retries ++) {
    
    paOperation = LATE(pa_context_get_sink_input_info)(
        _paContext,
        LATE(pa_stream_get_index)(_paPlayStream),
        PaSinkInputInfoCallback,
        (void*) this);

    WaitForOperationCompletion(paOperation);
  }
  PaUnLock();

  if (!_callbackValues) {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "GetSinkInputInfo failed to get volume info : %d",
                 LATE(pa_context_errno)(_paContext));
    return false;
  }

  return true;
}

bool AudioMixerManagerLinuxPulse::GetSinkInfoByIndex(
    int device_index) const {
  pa_operation* paOperation = NULL;
  ResetCallbackVariables();

  PaLock();
  for (int retries = 0; retries < kMaxRetryOnFailure && !_callbackValues;
       retries ++) {
    paOperation = LATE(pa_context_get_sink_info_by_index)(_paContext,
        device_index, PaSinkInfoCallback, (void*) this);

    WaitForOperationCompletion(paOperation);
  }
  PaUnLock();

  if (!_callbackValues) {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "GetSinkInfoByIndex failed to get volume info: %d",
                 LATE(pa_context_errno)(_paContext));
    return false;
  }

  return true;
}

bool AudioMixerManagerLinuxPulse::GetSourceInfoByIndex(
    int device_index) const {
  pa_operation* paOperation = NULL;
  ResetCallbackVariables();

  PaLock();
  for (int retries = 0; retries < kMaxRetryOnFailure && !_callbackValues;
       retries ++) {
  paOperation  = LATE(pa_context_get_source_info_by_index)(
      _paContext, device_index, PaSourceInfoCallback, (void*) this);

  WaitForOperationCompletion(paOperation);
  }

  PaUnLock();

  if (!_callbackValues) {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "GetSourceInfoByIndex error: %d",
                 LATE(pa_context_errno)(_paContext));
    return false;
  }

  return true;
}

}

