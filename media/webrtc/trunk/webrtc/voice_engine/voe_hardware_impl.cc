









#include "voe_hardware_impl.h"

#include <cassert>

#include "cpu_wrapper.h"
#include "critical_section_wrapper.h"
#include "trace.h"
#include "voe_errors.h"
#include "voice_engine_impl.h"

namespace webrtc
{

VoEHardware* VoEHardware::GetInterface(VoiceEngine* voiceEngine)
{
#ifndef WEBRTC_VOICE_ENGINE_HARDWARE_API
    return NULL;
#else
    if (NULL == voiceEngine)
    {
        return NULL;
    }
    VoiceEngineImpl* s = reinterpret_cast<VoiceEngineImpl*>(voiceEngine);
    s->AddRef();
    return s;
#endif
}

#ifdef WEBRTC_VOICE_ENGINE_HARDWARE_API

VoEHardwareImpl::VoEHardwareImpl(voe::SharedData* shared) :
    _cpu(NULL), _shared(shared)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEHardwareImpl() - ctor");

    _cpu = CpuWrapper::CreateCpu();
    if (_cpu)
    {
        _cpu->CpuUsage(); 
    }
}

VoEHardwareImpl::~VoEHardwareImpl()
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "~VoEHardwareImpl() - dtor");

    if (_cpu)
    {
        delete _cpu;
        _cpu = NULL;
    }
}

int VoEHardwareImpl::SetAudioDeviceLayer(AudioLayers audioLayer)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetAudioDeviceLayer(audioLayer=%d)", audioLayer);

    
    if (_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_ALREADY_INITED, kTraceError);
        return -1;
    }

    
    AudioDeviceModule::AudioLayer
        wantedLayer(AudioDeviceModule::kPlatformDefaultAudio);
    switch (audioLayer)
    {
        case kAudioPlatformDefault:
            
            break;
        case kAudioWindowsCore:
            wantedLayer = AudioDeviceModule::kWindowsCoreAudio;
            break;
        case kAudioWindowsWave:
            wantedLayer = AudioDeviceModule::kWindowsWaveAudio;
            break;
        case kAudioLinuxAlsa:
            wantedLayer = AudioDeviceModule::kLinuxAlsaAudio;
            break;
        case kAudioLinuxPulse:
            wantedLayer = AudioDeviceModule::kLinuxPulseAudio;
            break;
    }

    
    _shared->set_audio_device_layer(wantedLayer);

    return 0;
}

int VoEHardwareImpl::GetAudioDeviceLayer(AudioLayers& audioLayer)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
               "GetAudioDeviceLayer(devices=?)");

    

    AudioDeviceModule::AudioLayer
        activeLayer(AudioDeviceModule::kPlatformDefaultAudio);

    if (_shared->audio_device())
    {
        
        if (_shared->audio_device()->ActiveAudioLayer(&activeLayer) != 0)
        {
            _shared->SetLastError(VE_UNDEFINED_SC_ERR, kTraceError,
                "  Audio Device error");
            return -1;
        }
    }
    else
    {
        
        activeLayer = _shared->audio_device_layer();
    }

    
    switch (activeLayer)
    {
        case AudioDeviceModule::kPlatformDefaultAudio:
            audioLayer = kAudioPlatformDefault;
            break;
        case AudioDeviceModule::kWindowsCoreAudio:
            audioLayer = kAudioWindowsCore;
            break;
        case AudioDeviceModule::kWindowsWaveAudio:
            audioLayer = kAudioWindowsWave;
            break;
        case AudioDeviceModule::kLinuxAlsaAudio:
            audioLayer = kAudioLinuxAlsa;
            break;
        case AudioDeviceModule::kLinuxPulseAudio:
            audioLayer = kAudioLinuxPulse;
            break;
        default:
            _shared->SetLastError(VE_UNDEFINED_SC_ERR, kTraceError,
                "  unknown audio layer");
    }

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "  Output: audioLayer=%d", audioLayer);

    return 0;
}
int VoEHardwareImpl::GetNumOfRecordingDevices(int& devices)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetNumOfRecordingDevices(devices=?)");
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    devices = static_cast<int> (_shared->audio_device()->RecordingDevices());

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1), "  Output: devices=%d", devices);

    return 0;
}

int VoEHardwareImpl::GetNumOfPlayoutDevices(int& devices)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetNumOfPlayoutDevices(devices=?)");
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    devices = static_cast<int> (_shared->audio_device()->PlayoutDevices());

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "  Output: devices=%d", devices);

    return 0;
}

int VoEHardwareImpl::GetRecordingDeviceName(int index,
                                            char strNameUTF8[128],
                                            char strGuidUTF8[128])
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRecordingDeviceName(index=%d)", index);
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    if (strNameUTF8 == NULL)
    {
        _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
            "GetRecordingDeviceName() invalid argument");
        return -1;
    }

    

    
    const WebRtc_UWord16 strLen = 128;

    
    assert(strLen == kAdmMaxDeviceNameSize);
    assert(strLen == kAdmMaxGuidSize);

    char name[strLen];
    char guid[strLen];

    
    if (_shared->audio_device()->RecordingDeviceName(index, name, guid) != 0)
    {
        _shared->SetLastError(VE_CANNOT_RETRIEVE_DEVICE_NAME, kTraceError,
            "GetRecordingDeviceName() failed to get device name");
        return -1;
    }

    
    strncpy(strNameUTF8, name, strLen);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "  Output: strNameUTF8=%s", strNameUTF8);

    if (strGuidUTF8 != NULL)
    {
        strncpy(strGuidUTF8, guid, strLen);
        WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
            VoEId(_shared->instance_id(), -1),
            "  Output: strGuidUTF8=%s", strGuidUTF8);
    }

    return 0;
}

int VoEHardwareImpl::GetPlayoutDeviceName(int index,
                                          char strNameUTF8[128],
                                          char strGuidUTF8[128])
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetPlayoutDeviceName(index=%d)", index);
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    if (strNameUTF8 == NULL)
    {
        _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
            "GetPlayoutDeviceName() invalid argument");
        return -1;
    }

    

    
    const WebRtc_UWord16 strLen = 128;

    
    assert(strLen == kAdmMaxDeviceNameSize);
    assert(strLen == kAdmMaxGuidSize);

    char name[strLen];
    char guid[strLen];

    
    if (_shared->audio_device()->PlayoutDeviceName(index, name, guid) != 0)
    {
        _shared->SetLastError(VE_CANNOT_RETRIEVE_DEVICE_NAME, kTraceError,
            "GetPlayoutDeviceName() failed to get device name");
        return -1;
    }

    
    strncpy(strNameUTF8, name, strLen);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "  Output: strNameUTF8=%s", strNameUTF8);

    if (strGuidUTF8 != NULL)
    {
        strncpy(strGuidUTF8, guid, strLen);
        WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
            VoEId(_shared->instance_id(), -1),
            "  Output: strGuidUTF8=%s", strGuidUTF8);
    }

    return 0;
}

int VoEHardwareImpl::SetRecordingDevice(int index,
                                        StereoChannel recordingChannel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetRecordingDevice(index=%d, recordingChannel=%d)",
                 index, (int) recordingChannel);
    CriticalSectionScoped cs(_shared->crit_sec());
    IPHONE_NOT_SUPPORTED(_shared->statistics());
    

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    bool isRecording(false);

    
    
    if (_shared->audio_device()->Recording())
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_shared->instance_id(), -1),
                     "SetRecordingDevice() device is modified while recording"
                     " is active...");
        isRecording = true;
        if (_shared->audio_device()->StopRecording() == -1)
        {
            _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceError,
                "SetRecordingDevice() unable to stop recording");
            return -1;
        }
    }

    

    
    AudioDeviceModule::ChannelType recCh =
        AudioDeviceModule::kChannelBoth;
    switch (recordingChannel)
    {
        case kStereoLeft:
            recCh = AudioDeviceModule::kChannelLeft;
            break;
        case kStereoRight:
            recCh = AudioDeviceModule::kChannelRight;
            break;
        case kStereoBoth:
            
            break;
    }

    if (_shared->audio_device()->SetRecordingChannel(recCh) != 0) {
      _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceWarning,
          "SetRecordingChannel() unable to set the recording channel");
    }

    
    WebRtc_UWord16 indexU = static_cast<WebRtc_UWord16> (index);

    WebRtc_Word32 res(0);

    if (index == -1)
    {
        res = _shared->audio_device()->SetRecordingDevice(
            AudioDeviceModule::kDefaultCommunicationDevice);
    }
    else if (index == -2)
    {
        res = _shared->audio_device()->SetRecordingDevice(
            AudioDeviceModule::kDefaultDevice);
    }
    else
    {
        res = _shared->audio_device()->SetRecordingDevice(indexU);
    }

    if (res != 0)
    {
        _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceError,
            "SetRecordingDevice() unable to set the recording device");
        return -1;
    }

    
    if (_shared->audio_device()->InitMicrophone() == -1)
    {
        _shared->SetLastError(VE_CANNOT_ACCESS_MIC_VOL, kTraceWarning,
            "SetRecordingDevice() cannot access microphone");
    }

    
    bool available = false;
    if (_shared->audio_device()->StereoRecordingIsAvailable(&available) != 0) {
      _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceWarning,
          "StereoRecordingIsAvailable() failed to query stereo recording");
    }

    if (_shared->audio_device()->SetStereoRecording(available) != 0)
    {
        _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceWarning,
            "SetRecordingDevice() failed to set mono recording mode");
    }

    
    if (isRecording)
    {
        if (!_shared->ext_recording())
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                VoEId(_shared->instance_id(), -1),
                "SetRecordingDevice() recording is now being restored...");
            if (_shared->audio_device()->InitRecording() != 0)
            {
                WEBRTC_TRACE(kTraceError, kTraceVoice,
                    VoEId(_shared->instance_id(), -1),
                    "SetRecordingDevice() failed to initialize recording");
                return -1;
            }
            if (_shared->audio_device()->StartRecording() != 0)
            {
                WEBRTC_TRACE(kTraceError, kTraceVoice,
                             VoEId(_shared->instance_id(), -1),
                             "SetRecordingDevice() failed to start recording");
                return -1;
            }
        }
    }

    return 0;
}

int VoEHardwareImpl::SetPlayoutDevice(int index)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetPlayoutDevice(index=%d)", index);
    CriticalSectionScoped cs(_shared->crit_sec());
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    bool isPlaying(false);

    
    
    if (_shared->audio_device()->Playing())
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_shared->instance_id(), -1),
                     "SetPlayoutDevice() device is modified while playout is "
                     "active...");
        isPlaying = true;
        if (_shared->audio_device()->StopPlayout() == -1)
        {
            _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceError,
                "SetPlayoutDevice() unable to stop playout");
            return -1;
        }
    }

    

    
    WebRtc_UWord16 indexU = static_cast<WebRtc_UWord16> (index);

    WebRtc_Word32 res(0);

    if (index == -1)
    {
        res = _shared->audio_device()->SetPlayoutDevice(
            AudioDeviceModule::kDefaultCommunicationDevice);
    }
    else if (index == -2)
    {
        res = _shared->audio_device()->SetPlayoutDevice(
            AudioDeviceModule::kDefaultDevice);
    }
    else
    {
        res = _shared->audio_device()->SetPlayoutDevice(indexU);
    }

    if (res != 0)
    {
        _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceError,
            "SetPlayoutDevice() unable to set the playout device");
        return -1;
    }

    
    if (_shared->audio_device()->InitSpeaker() == -1)
    {
        _shared->SetLastError(VE_CANNOT_ACCESS_SPEAKER_VOL, kTraceWarning,
            "SetPlayoutDevice() cannot access speaker");
    }

    
    bool available = false;
    _shared->audio_device()->StereoPlayoutIsAvailable(&available);
    if (_shared->audio_device()->SetStereoPlayout(available) != 0)
    {
        _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceWarning,
            "SetPlayoutDevice() failed to set stereo playout mode");
    }

    
    if (isPlaying)
    {
        if (!_shared->ext_playout())
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                VoEId(_shared->instance_id(), -1),
                "SetPlayoutDevice() playout is now being restored...");
            if (_shared->audio_device()->InitPlayout() != 0)
            {
                WEBRTC_TRACE(kTraceError, kTraceVoice,
                  VoEId(_shared->instance_id(), -1),
                  "SetPlayoutDevice() failed to initialize playout");
                return -1;
            }
            if (_shared->audio_device()->StartPlayout() != 0)
            {
                WEBRTC_TRACE(kTraceError, kTraceVoice,
                             VoEId(_shared->instance_id(), -1),
                             "SetPlayoutDevice() failed to start playout");
                return -1;
            }
        }
    }

    return 0;
}

int VoEHardwareImpl::GetRecordingDeviceStatus(bool& isAvailable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetRecordingDeviceStatus()");
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    

    bool available(false);

    
    if (_shared->audio_device()->RecordingIsAvailable(&available) != 0)
    {
        _shared->SetLastError(VE_UNDEFINED_SC_REC_ERR, kTraceError,
            "  Audio Device error");
        return -1;
    }

    isAvailable = available;

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "  Output: isAvailable = %d)", (int) isAvailable);

    return 0;
}

int VoEHardwareImpl::GetPlayoutDeviceStatus(bool& isAvailable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetPlayoutDeviceStatus()");
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    

    bool available(false);

    
    if (_shared->audio_device()->PlayoutIsAvailable(&available) != 0)
    {
        _shared->SetLastError(VE_PLAY_UNDEFINED_SC_ERR, kTraceError,
            "  Audio Device error");
        return -1;
    }

    isAvailable = available;

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "  Output: isAvailable = %d)", (int) isAvailable);

    return 0;
}

int VoEHardwareImpl::ResetAudioDevice()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "ResetAudioDevice()");
    ANDROID_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

#if defined(WEBRTC_IOS)
    if (_shared->audio_device()->ResetAudioDevice() < 0)
    {
        _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceError,
            "  Failed to reset sound device");
        return -1;
    }
#else
    _shared->SetLastError(VE_FUNC_NOT_SUPPORTED, kTraceError,
        "  no support for resetting sound device");
    return -1;
#endif

    return 0;
}

int VoEHardwareImpl::AudioDeviceControl(unsigned int par1, unsigned int par2,
                                        unsigned int par3)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "AudioDeviceControl(%i, %i, %i)", par1, par2, par3);
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    _shared->SetLastError(VE_FUNC_NOT_SUPPORTED, kTraceError,
        "  no support for resetting sound device");
    return -1;
}

int VoEHardwareImpl::SetLoudspeakerStatus(bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetLoudspeakerStatus(enable=%i)", (int) enable);
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
#if defined(WEBRTC_ANDROID)
    if (_shared->audio_device()->SetLoudspeakerStatus(enable) < 0)
    {
        _shared->SetLastError(VE_IGNORED_FUNCTION, kTraceError,
            "  Failed to set loudspeaker status");
        return -1;
    }

    return 0;
#else
    _shared->SetLastError(VE_FUNC_NOT_SUPPORTED, kTraceError,
        "  no support for setting loudspeaker status");
    return -1;
#endif
}

int VoEHardwareImpl::GetLoudspeakerStatus(bool& enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetLoudspeakerStatus()");
    IPHONE_NOT_SUPPORTED(_shared->statistics());

#if defined(WEBRTC_ANDROID)
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    if (_shared->audio_device()->GetLoudspeakerStatus(&enabled) < 0)
    {
        _shared->SetLastError(VE_IGNORED_FUNCTION, kTraceError,
            "  Failed to get loudspeaker status");
        return -1;
    }

    return 0;
#else
    _shared->SetLastError(VE_FUNC_NOT_SUPPORTED, kTraceError,
      "  no support for setting loudspeaker status");
    return -1;
#endif
}

int VoEHardwareImpl::GetCPULoad(int& loadPercent)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetCPULoad()");
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    
    WebRtc_UWord16 load(0);
    if (_shared->audio_device()->CPULoad(&load) != 0)
    {
        _shared->SetLastError(VE_CPU_INFO_ERROR, kTraceError,
            "  error getting system CPU load");
        return -1;
    }

    loadPercent = static_cast<int> (load);

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "  Output: loadPercent = %d", loadPercent);

    return 0;
}

int VoEHardwareImpl::GetSystemCPULoad(int& loadPercent)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetSystemCPULoad(loadPercent=?)");
    ANDROID_NOT_SUPPORTED(_shared->statistics());
    IPHONE_NOT_SUPPORTED(_shared->statistics());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    
    if (!_cpu)
    {
        _shared->SetLastError(VE_FUNC_NOT_SUPPORTED, kTraceError,
            "  no support for getting system CPU load");
        return -1;
    }

    
    WebRtc_Word32 load = _cpu->CpuUsage();
    if (load < 0)
    {
        _shared->SetLastError(VE_CPU_INFO_ERROR, kTraceError,
            "  error getting system CPU load");
        return -1;
    }

    loadPercent = static_cast<int> (load);

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "  Output: loadPercent = %d", loadPercent);

    return 0;
}

int VoEHardwareImpl::EnableBuiltInAEC(bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
        "%s", __FUNCTION__);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    return _shared->audio_device()->EnableBuiltInAEC(enable);
}

bool VoEHardwareImpl::BuiltInAECIsEnabled() const
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
        "%s", __FUNCTION__);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return false;
    }

    return _shared->audio_device()->BuiltInAECIsEnabled();
}

int VoEHardwareImpl::SetRecordingSampleRate(unsigned int samples_per_sec) {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
               "%s", __FUNCTION__);
  if (!_shared->statistics().Initialized()) {
    _shared->SetLastError(VE_NOT_INITED, kTraceError);
    return false;
  }
  return _shared->audio_device()->SetRecordingSampleRate(samples_per_sec);
}

int VoEHardwareImpl::RecordingSampleRate(unsigned int* samples_per_sec) const {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
               "%s", __FUNCTION__);
  if (!_shared->statistics().Initialized()) {
    _shared->SetLastError(VE_NOT_INITED, kTraceError);
    return false;
  }
  return _shared->audio_device()->RecordingSampleRate(samples_per_sec);
}

int VoEHardwareImpl::SetPlayoutSampleRate(unsigned int samples_per_sec) {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
               "%s", __FUNCTION__);
  if (!_shared->statistics().Initialized()) {
    _shared->SetLastError(VE_NOT_INITED, kTraceError);
    return false;
  }
  return _shared->audio_device()->SetPlayoutSampleRate(samples_per_sec);
}

int VoEHardwareImpl::PlayoutSampleRate(unsigned int* samples_per_sec) const {
  WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
               "%s", __FUNCTION__);
  if (!_shared->statistics().Initialized()) {
    _shared->SetLastError(VE_NOT_INITED, kTraceError);
    return false;
  }
  return _shared->audio_device()->PlayoutSampleRate(samples_per_sec);
}

#endif  

} 
