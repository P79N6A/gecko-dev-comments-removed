









#include "voe_base_impl.h"

#include "audio_coding_module.h"
#include "audio_processing.h"
#include "channel.h"
#include "critical_section_wrapper.h"
#include "file_wrapper.h"
#include "modules/audio_device/audio_device_impl.h"
#include "output_mixer.h"
#include "signal_processing_library.h"
#include "trace.h"
#include "transmit_mixer.h"
#include "utility.h"
#include "voe_errors.h"
#include "voice_engine_impl.h"

#if (defined(_WIN32) && defined(_DLL) && (_MSC_VER == 1400))

#include <stdio.h>
extern "C"
    { FILE _iob[3] = {   __iob_func()[0], __iob_func()[1], __iob_func()[2]}; }
#endif

namespace webrtc
{

VoEBase* VoEBase::GetInterface(VoiceEngine* voiceEngine)
{
    if (NULL == voiceEngine)
    {
        return NULL;
    }
    VoiceEngineImpl* s = static_cast<VoiceEngineImpl*>(voiceEngine);
    s->AddRef();
    return s;
}

VoEBaseImpl::VoEBaseImpl(voe::SharedData* shared) :
    _voiceEngineObserverPtr(NULL),
    _callbackCritSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _voiceEngineObserver(false), _oldVoEMicLevel(0), _oldMicLevel(0),
    _shared(shared)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEBaseImpl() - ctor");
}

VoEBaseImpl::~VoEBaseImpl()
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "~VoEBaseImpl() - dtor");

    TerminateInternal();

    delete &_callbackCritSect;
}

void VoEBaseImpl::OnErrorIsReported(const ErrorCode error)
{
    CriticalSectionScoped cs(&_callbackCritSect);
    if (_voiceEngineObserver)
    {
        if (_voiceEngineObserverPtr)
        {
            int errCode(0);
            if (error == AudioDeviceObserver::kRecordingError)
            {
                errCode = VE_RUNTIME_REC_ERROR;
                WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                    VoEId(_shared->instance_id(), -1),
                    "VoEBaseImpl::OnErrorIsReported() => VE_RUNTIME_REC_ERROR");
            }
            else if (error == AudioDeviceObserver::kPlayoutError)
            {
                errCode = VE_RUNTIME_PLAY_ERROR;
                WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                    VoEId(_shared->instance_id(), -1),
                    "VoEBaseImpl::OnErrorIsReported() => "
                    "VE_RUNTIME_PLAY_ERROR");
            }
            
            _voiceEngineObserverPtr->CallbackOnError(-1, errCode);
        }
    }
}

void VoEBaseImpl::OnWarningIsReported(const WarningCode warning)
{
    CriticalSectionScoped cs(&_callbackCritSect);
    if (_voiceEngineObserver)
    {
        if (_voiceEngineObserverPtr)
        {
            int warningCode(0);
            if (warning == AudioDeviceObserver::kRecordingWarning)
            {
                warningCode = VE_RUNTIME_REC_WARNING;
                WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                    VoEId(_shared->instance_id(), -1),
                    "VoEBaseImpl::OnErrorIsReported() => "
                    "VE_RUNTIME_REC_WARNING");
            }
            else if (warning == AudioDeviceObserver::kPlayoutWarning)
            {
                warningCode = VE_RUNTIME_PLAY_WARNING;
                WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                    VoEId(_shared->instance_id(), -1),
                    "VoEBaseImpl::OnErrorIsReported() => "
                    "VE_RUNTIME_PLAY_WARNING");
            }
            
            _voiceEngineObserverPtr->CallbackOnError(-1, warningCode);
        }
    }
}

int32_t VoEBaseImpl::RecordedDataIsAvailable(
        const void* audioSamples,
        const uint32_t nSamples,
        const uint8_t nBytesPerSample,
        const uint8_t nChannels,
        const uint32_t samplesPerSec,
        const uint32_t totalDelayMS,
        const int32_t clockDrift,
        const uint32_t currentMicLevel,
        uint32_t& newMicLevel)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEBaseImpl::RecordedDataIsAvailable(nSamples=%u, "
                     "nBytesPerSample=%u, nChannels=%u, samplesPerSec=%u, "
                     "totalDelayMS=%u, clockDrift=%d, currentMicLevel=%u)",
                 nSamples, nBytesPerSample, nChannels, samplesPerSec,
                 totalDelayMS, clockDrift, currentMicLevel);

    assert(_shared->transmit_mixer() != NULL);
    assert(_shared->audio_device() != NULL);

    bool isAnalogAGC(false);
    uint32_t maxVolume(0);
    uint16_t currentVoEMicLevel(0);
    uint32_t newVoEMicLevel(0);

    if (_shared->audio_processing() &&
        (_shared->audio_processing()->gain_control()->mode()
                    == GainControl::kAdaptiveAnalog))
    {
        isAnalogAGC = true;
    }

    
    if (isAnalogAGC)
    {
        
        if (_shared->audio_device()->MaxMicrophoneVolume(&maxVolume) == 0)
        {
            if (0 != maxVolume)
            {
                currentVoEMicLevel = (uint16_t) ((currentMicLevel
                        * kMaxVolumeLevel + (int) (maxVolume / 2))
                        / (maxVolume));
            }
        }
        
        
        
        
        
        if (currentVoEMicLevel > kMaxVolumeLevel)
        {
            currentVoEMicLevel = kMaxVolumeLevel;
            maxVolume = currentMicLevel;
        }
    }

    
    
    
    
    if (_oldMicLevel == currentMicLevel)
    {
        currentVoEMicLevel = (uint16_t) _oldVoEMicLevel;
    }

    
    
    _shared->transmit_mixer()->PrepareDemux(audioSamples, nSamples, nChannels,
        samplesPerSec, static_cast<uint16_t>(totalDelayMS), clockDrift,
        currentVoEMicLevel);

    
    
    
    _shared->transmit_mixer()->DemuxAndMix();
    
    
    _shared->transmit_mixer()->EncodeAndSend();

    
    if (isAnalogAGC)
    {
        
        newVoEMicLevel = _shared->transmit_mixer()->CaptureLevel();
        if (newVoEMicLevel != currentVoEMicLevel)
        {
            
            newMicLevel = (uint32_t) ((newVoEMicLevel * maxVolume
                    + (int) (kMaxVolumeLevel / 2)) / (kMaxVolumeLevel));
        }
        else
        {
            
            newMicLevel = 0;
        }

        
        _oldVoEMicLevel = newVoEMicLevel;
        _oldMicLevel = currentMicLevel;
    }

    return 0;
}

int32_t VoEBaseImpl::NeedMorePlayData(
        const uint32_t nSamples,
        const uint8_t nBytesPerSample,
        const uint8_t nChannels,
        const uint32_t samplesPerSec,
        void* audioSamples,
        uint32_t& nSamplesOut)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEBaseImpl::NeedMorePlayData(nSamples=%u, "
                     "nBytesPerSample=%d, nChannels=%d, samplesPerSec=%u)",
                 nSamples, nBytesPerSample, nChannels, samplesPerSec);

    assert(_shared->output_mixer() != NULL);

    
    
    
    _shared->output_mixer()->MixActiveChannels();

    
    _shared->output_mixer()->DoOperationsOnCombinedSignal();

    
    _shared->output_mixer()->GetMixedAudio(samplesPerSec, nChannels,
        &_audioFrame);

    assert(static_cast<int>(nSamples) == _audioFrame.samples_per_channel_);
    assert(samplesPerSec ==
        static_cast<uint32_t>(_audioFrame.sample_rate_hz_));

    
    memcpy(
           (int16_t*) audioSamples,
           (const int16_t*) _audioFrame.data_,
           sizeof(int16_t) * (_audioFrame.samples_per_channel_
                   * _audioFrame.num_channels_));

    nSamplesOut = _audioFrame.samples_per_channel_;

    return 0;
}

int VoEBaseImpl::RegisterVoiceEngineObserver(VoiceEngineObserver& observer)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "RegisterVoiceEngineObserver(observer=0x%d)", &observer);
    CriticalSectionScoped cs(&_callbackCritSect);
    if (_voiceEngineObserverPtr)
    {
        _shared->SetLastError(VE_INVALID_OPERATION, kTraceError,
            "RegisterVoiceEngineObserver() observer already enabled");
        return -1;
    }

    
    voe::ScopedChannel sc(_shared->channel_manager());
    void* iterator(NULL);
    voe::Channel* channelPtr = sc.GetFirstChannel(iterator);
    while (channelPtr != NULL)
    {
        channelPtr->RegisterVoiceEngineObserver(observer);
        channelPtr = sc.GetNextChannel(iterator);
    }
    _shared->transmit_mixer()->RegisterVoiceEngineObserver(observer);

    _voiceEngineObserverPtr = &observer;
    _voiceEngineObserver = true;

    return 0;
}

int VoEBaseImpl::DeRegisterVoiceEngineObserver()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "DeRegisterVoiceEngineObserver()");
    CriticalSectionScoped cs(&_callbackCritSect);
    if (!_voiceEngineObserverPtr)
    {
        _shared->SetLastError(VE_INVALID_OPERATION, kTraceError,
            "DeRegisterVoiceEngineObserver() observer already disabled");
        return 0;
    }

    _voiceEngineObserver = false;
    _voiceEngineObserverPtr = NULL;

    
    voe::ScopedChannel sc(_shared->channel_manager());
    void* iterator(NULL);
    voe::Channel* channelPtr = sc.GetFirstChannel(iterator);
    while (channelPtr != NULL)
    {
        channelPtr->DeRegisterVoiceEngineObserver();
        channelPtr = sc.GetNextChannel(iterator);
    }

    return 0;
}

int VoEBaseImpl::Init(AudioDeviceModule* external_adm,
                      AudioProcessing* audioproc)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
        "Init(external_adm=0x%p)", external_adm);
    CriticalSectionScoped cs(_shared->crit_sec());

    WebRtcSpl_Init();

    if (_shared->statistics().Initialized())
    {
        return 0;
    }

    if (_shared->process_thread())
    {
        if (_shared->process_thread()->Start() != 0)
        {
            _shared->SetLastError(VE_THREAD_ERROR, kTraceError,
                "Init() failed to start module process thread");
            return -1;
        }
    }

    
    
    if (external_adm == NULL)
    {
        
        _shared->set_audio_device(AudioDeviceModuleImpl::Create(
            VoEId(_shared->instance_id(), -1), _shared->audio_device_layer()));

        if (_shared->audio_device() == NULL)
        {
            _shared->SetLastError(VE_NO_MEMORY, kTraceCritical,
                "Init() failed to create the ADM");
            return -1;
        }
    }
    else
    {
        
        _shared->set_audio_device(external_adm);
        WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_shared->instance_id(), -1),
            "An external ADM implementation will be used in VoiceEngine");
    }

    
    
    if (_shared->process_thread() &&
        _shared->process_thread()->RegisterModule(_shared->audio_device()) != 0)
    {
        _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceError,
            "Init() failed to register the ADM");
        return -1;
    }

    bool available(false);

    
    

    
    if (_shared->audio_device()->RegisterEventObserver(this) != 0) {
      _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceWarning,
          "Init() failed to register event observer for the ADM");
    }

    
    if (_shared->audio_device()->RegisterAudioCallback(this) != 0) {
      _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceWarning,
          "Init() failed to register audio callback for the ADM");
    }

    
    if (_shared->audio_device()->Init() != 0)
    {
        _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceError,
            "Init() failed to initialize the ADM");
        return -1;
    }

    
    if (_shared->audio_device()->SetPlayoutDevice(
            WEBRTC_VOICE_ENGINE_DEFAULT_DEVICE) != 0)
    {
        _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceInfo,
            "Init() failed to set the default output device");
    }
    if (_shared->audio_device()->SpeakerIsAvailable(&available) != 0)
    {
        _shared->SetLastError(VE_CANNOT_ACCESS_SPEAKER_VOL, kTraceInfo,
            "Init() failed to check speaker availability, trying to "
            "initialize speaker anyway");
    }
    else if (!available)
    {
        _shared->SetLastError(VE_CANNOT_ACCESS_SPEAKER_VOL, kTraceInfo,
            "Init() speaker not available, trying to initialize speaker "
            "anyway");
    }
    if (_shared->audio_device()->InitSpeaker() != 0)
    {
        _shared->SetLastError(VE_CANNOT_ACCESS_SPEAKER_VOL, kTraceInfo,
            "Init() failed to initialize the speaker");
    }

    
    if (_shared->audio_device()->SetRecordingDevice(
            WEBRTC_VOICE_ENGINE_DEFAULT_DEVICE) != 0)
    {
        _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceInfo,
            "Init() failed to set the default input device");
    }
    if (_shared->audio_device()->MicrophoneIsAvailable(&available) != 0)
    {
        _shared->SetLastError(VE_CANNOT_ACCESS_MIC_VOL, kTraceInfo,
            "Init() failed to check microphone availability, trying to "
            "initialize microphone anyway");
    }
    else if (!available)
    {
        _shared->SetLastError(VE_CANNOT_ACCESS_MIC_VOL, kTraceInfo,
            "Init() microphone not available, trying to initialize "
            "microphone anyway");
    }
    if (_shared->audio_device()->InitMicrophone() != 0)
    {
        _shared->SetLastError(VE_CANNOT_ACCESS_MIC_VOL, kTraceInfo,
            "Init() failed to initialize the microphone");
    }

    
    if (_shared->audio_device()->StereoPlayoutIsAvailable(&available) != 0) {
      _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceWarning,
          "Init() failed to query stereo playout mode");
    }
    if (_shared->audio_device()->SetStereoPlayout(available) != 0)
    {
        _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceWarning,
            "Init() failed to set mono/stereo playout mode");
    }

    
    
    
    
    
    
    
    _shared->audio_device()->StereoRecordingIsAvailable(&available);
    if (_shared->audio_device()->SetStereoRecording(available) != 0)
    {
        _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceWarning,
            "Init() failed to set mono/stereo recording mode");
    }

    if (!audioproc) {
      audioproc = AudioProcessing::Create(VoEId(_shared->instance_id(), -1));
      if (!audioproc) {
        LOG(LS_ERROR) << "Failed to create AudioProcessing.";
        _shared->SetLastError(VE_NO_MEMORY);
        return -1;
      }
    }
    _shared->set_audio_processing(audioproc);

    
    _shared->SetLastError(VE_APM_ERROR);
    if (audioproc->echo_cancellation()->set_device_sample_rate_hz(48000)) {
      LOG_FERR1(LS_ERROR, set_device_sample_rate_hz, 48000);
      return -1;
    }
    
    
    if (audioproc->set_sample_rate_hz(16000)) {
      LOG_FERR1(LS_ERROR, set_sample_rate_hz, 16000);
      return -1;
    }
    if (audioproc->set_num_channels(1, 1) != 0) {
      LOG_FERR2(LS_ERROR, set_num_channels, 1, 1);
      return -1;
    }
    if (audioproc->set_num_reverse_channels(1) != 0) {
      LOG_FERR1(LS_ERROR, set_num_reverse_channels, 1);
      return -1;
    }

    
    if (audioproc->high_pass_filter()->Enable(true) != 0) {
      LOG_FERR1(LS_ERROR, high_pass_filter()->Enable, true);
      return -1;
    }
    if (audioproc->echo_cancellation()->enable_drift_compensation(false) != 0) {
      LOG_FERR1(LS_ERROR, enable_drift_compensation, false);
      return -1;
    }
    if (audioproc->noise_suppression()->set_level(kDefaultNsMode) != 0) {
      LOG_FERR1(LS_ERROR, noise_suppression()->set_level, kDefaultNsMode);
      return -1;
    }
    GainControl* agc = audioproc->gain_control();
    if (agc->set_analog_level_limits(kMinVolumeLevel, kMaxVolumeLevel) != 0) {
      LOG_FERR2(LS_ERROR, agc->set_analog_level_limits, kMinVolumeLevel,
                kMaxVolumeLevel);
      return -1;
    }
    if (agc->set_mode(kDefaultAgcMode) != 0) {
      LOG_FERR1(LS_ERROR, agc->set_mode, kDefaultAgcMode);
      return -1;
    }
    if (agc->Enable(kDefaultAgcState) != 0) {
      LOG_FERR1(LS_ERROR, agc->Enable, kDefaultAgcState);
      return -1;
    }
    _shared->SetLastError(0);  

#ifdef WEBRTC_VOICE_ENGINE_AGC
    bool agc_enabled = agc->mode() == GainControl::kAdaptiveAnalog &&
                       agc->is_enabled();
    if (_shared->audio_device()->SetAGC(agc_enabled) != 0) {
      LOG_FERR1(LS_ERROR, audio_device()->SetAGC, agc_enabled);
      _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR);
      
      
    }
#endif

    return _shared->statistics().SetInitialized();
}

int VoEBaseImpl::Terminate()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "Terminate()");
    CriticalSectionScoped cs(_shared->crit_sec());
    return TerminateInternal();
}

int VoEBaseImpl::MaxNumOfChannels()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "MaxNumOfChannels()");
    int32_t maxNumOfChannels =
        _shared->channel_manager().MaxNumOfChannels();
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "MaxNumOfChannels() => %d", maxNumOfChannels);
    return (maxNumOfChannels);
}

int VoEBaseImpl::CreateChannel()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "CreateChannel()");
    CriticalSectionScoped cs(_shared->crit_sec());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    int32_t channelId = -1;

    if (!_shared->channel_manager().CreateChannel(channelId))
    {
        _shared->SetLastError(VE_CHANNEL_NOT_CREATED, kTraceError,
            "CreateChannel() failed to allocate memory for channel");
        return -1;
    }

    bool destroyChannel(false);
    {
        voe::ScopedChannel sc(_shared->channel_manager(), channelId);
        voe::Channel* channelPtr = sc.ChannelPtr();
        if (channelPtr == NULL)
        {
            _shared->SetLastError(VE_CHANNEL_NOT_CREATED, kTraceError,
                "CreateChannel() failed to allocate memory for channel");
            return -1;
        }
        else if (channelPtr->SetEngineInformation(_shared->statistics(),
                                                  *_shared->output_mixer(),
                                                  *_shared->transmit_mixer(),
                                                  *_shared->process_thread(),
                                                  *_shared->audio_device(),
                                                  _voiceEngineObserverPtr,
                                                  &_callbackCritSect) != 0)
        {
            destroyChannel = true;
            _shared->SetLastError(VE_CHANNEL_NOT_CREATED, kTraceError,
                "CreateChannel() failed to associate engine and channel."
                " Destroying channel.");
        }
        else if (channelPtr->Init() != 0)
        {
            destroyChannel = true;
            _shared->SetLastError(VE_CHANNEL_NOT_CREATED, kTraceError,
                "CreateChannel() failed to initialize channel. Destroying"
                " channel.");
        }
    }
    if (destroyChannel)
    {
        _shared->channel_manager().DestroyChannel(channelId);
        return -1;
    }
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1),
        "CreateChannel() => %d", channelId);
    return channelId;
}

int VoEBaseImpl::DeleteChannel(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "DeleteChannel(channel=%d)", channel);
    CriticalSectionScoped cs(_shared->crit_sec());

    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }

    {
        voe::ScopedChannel sc(_shared->channel_manager(), channel);
        voe::Channel* channelPtr = sc.ChannelPtr();
        if (channelPtr == NULL)
        {
            _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
                "DeleteChannel() failed to locate channel");
            return -1;
        }
    }

    if (_shared->channel_manager().DestroyChannel(channel) != 0)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "DeleteChannel() failed to destroy channel");
        return -1;
    }

    if (StopSend() != 0)
    {
        return -1;
    }

    if (StopPlayout() != 0)
    {
        return -1;
    }

    return 0;
}

int VoEBaseImpl::StartReceive(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "StartReceive(channel=%d)", channel);
    CriticalSectionScoped cs(_shared->crit_sec());
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "StartReceive() failed to locate channel");
        return -1;
    }
    return channelPtr->StartReceiving();
}

int VoEBaseImpl::StopReceive(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "StopListen(channel=%d)", channel);
    CriticalSectionScoped cs(_shared->crit_sec());
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetLocalReceiver() failed to locate channel");
        return -1;
    }
    return channelPtr->StopReceiving();
}

int VoEBaseImpl::StartPlayout(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "StartPlayout(channel=%d)", channel);
    CriticalSectionScoped cs(_shared->crit_sec());
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "StartPlayout() failed to locate channel");
        return -1;
    }
    if (channelPtr->Playing())
    {
        return 0;
    }
    if (StartPlayout() != 0)
    {
        _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceError,
            "StartPlayout() failed to start playout");
        return -1;
    }
    return channelPtr->StartPlayout();
}

int VoEBaseImpl::StopPlayout(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "StopPlayout(channel=%d)", channel);
    CriticalSectionScoped cs(_shared->crit_sec());
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "StopPlayout() failed to locate channel");
        return -1;
    }
    if (channelPtr->StopPlayout() != 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
            VoEId(_shared->instance_id(), -1),
            "StopPlayout() failed to stop playout for channel %d", channel);
    }
    return StopPlayout();
}

int VoEBaseImpl::StartSend(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "StartSend(channel=%d)", channel);
    CriticalSectionScoped cs(_shared->crit_sec());
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "StartSend() failed to locate channel");
        return -1;
    }
    if (channelPtr->Sending())
    {
        return 0;
    }
    if (StartSend() != 0)
    {
        _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceError,
            "StartSend() failed to start recording");
        return -1;
    }
    return channelPtr->StartSend();
}

int VoEBaseImpl::StopSend(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "StopSend(channel=%d)", channel);
    CriticalSectionScoped cs(_shared->crit_sec());
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "StopSend() failed to locate channel");
        return -1;
    }
    if (channelPtr->StopSend() != 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
            VoEId(_shared->instance_id(), -1),
            "StopSend() failed to stop sending for channel %d", channel);
    }
    return StopSend();
}

int VoEBaseImpl::GetVersion(char version[1024])
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetVersion(version=?)");
    assert(kVoiceEngineVersionMaxMessageSize == 1024);

    if (version == NULL)
    {
        _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError);
        return (-1);
    }

    char versionBuf[kVoiceEngineVersionMaxMessageSize];
    char* versionPtr = versionBuf;

    int32_t len = 0;
    int32_t accLen = 0;

    len = AddVoEVersion(versionPtr);
    if (len == -1)
    {
        return -1;
    }
    versionPtr += len;
    accLen += len;
    assert(accLen < kVoiceEngineVersionMaxMessageSize);

    len = AddBuildInfo(versionPtr);
    if (len == -1)
    {
        return -1;
    }
    versionPtr += len;
    accLen += len;
    assert(accLen < kVoiceEngineVersionMaxMessageSize);

#ifdef WEBRTC_EXTERNAL_TRANSPORT
    len = AddExternalTransportBuild(versionPtr);
    if (len == -1)
    {
         return -1;
    }
    versionPtr += len;
    accLen += len;
    assert(accLen < kVoiceEngineVersionMaxMessageSize);
#endif
#ifdef WEBRTC_VOE_EXTERNAL_REC_AND_PLAYOUT
    len = AddExternalRecAndPlayoutBuild(versionPtr);
    if (len == -1)
    {
        return -1;
    }
    versionPtr += len;
    accLen += len;
    assert(accLen < kVoiceEngineVersionMaxMessageSize);
 #endif

    memcpy(version, versionBuf, accLen);
    version[accLen] = '\0';

    
    char partOfVersion[256];
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
        VoEId(_shared->instance_id(), -1), "GetVersion() =>");
    for (int partStart = 0; partStart < accLen;)
    {
        memset(partOfVersion, 0, sizeof(partOfVersion));
        int partEnd = partStart + 180;
        while (version[partEnd] != '\n' && version[partEnd] != '\0')
        {
            partEnd--;
        }
        if (partEnd < accLen)
        {
            memcpy(partOfVersion, &version[partStart], partEnd - partStart);
        }
        else
        {
            memcpy(partOfVersion, &version[partStart], accLen - partStart);
        }
        partStart = partEnd;
        WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
            VoEId(_shared->instance_id(), -1), "%s", partOfVersion);
    }

    return 0;
}

int32_t VoEBaseImpl::AddBuildInfo(char* str) const
{
    return sprintf(str, "Build: svn:%s %s\n", WEBRTC_SVNREVISION, BUILDINFO);
}

int32_t VoEBaseImpl::AddVoEVersion(char* str) const
{
    return sprintf(str, "VoiceEngine 4.1.0\n");
}

#ifdef WEBRTC_EXTERNAL_TRANSPORT
int32_t VoEBaseImpl::AddExternalTransportBuild(char* str) const
{
    return sprintf(str, "External transport build\n");
}
#endif

#ifdef WEBRTC_VOE_EXTERNAL_REC_AND_PLAYOUT
int32_t VoEBaseImpl::AddExternalRecAndPlayoutBuild(char* str) const
{
    return sprintf(str, "External recording and playout build\n");
}
#endif

int VoEBaseImpl::LastError()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "LastError()");
    return (_shared->statistics().LastError());
}


int VoEBaseImpl::SetNetEQPlayoutMode(int channel, NetEqModes mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetNetEQPlayoutMode(channel=%i, mode=%i)", channel, mode);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetNetEQPlayoutMode() failed to locate channel");
        return -1;
    }
    return channelPtr->SetNetEQPlayoutMode(mode);
}

int VoEBaseImpl::GetNetEQPlayoutMode(int channel, NetEqModes& mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetNetEQPlayoutMode(channel=%i, mode=?)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetNetEQPlayoutMode() failed to locate channel");
        return -1;
    }
    return channelPtr->GetNetEQPlayoutMode(mode);
}

int VoEBaseImpl::SetOnHoldStatus(int channel, bool enable, OnHoldModes mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetOnHoldStatus(channel=%d, enable=%d, mode=%d)", channel,
                 enable, mode);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetOnHoldStatus() failed to locate channel");
        return -1;
    }
    return channelPtr->SetOnHoldStatus(enable, mode);
}

int VoEBaseImpl::GetOnHoldStatus(int channel, bool& enabled, OnHoldModes& mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "GetOnHoldStatus(channel=%d, enabled=?, mode=?)", channel);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ScopedChannel sc(_shared->channel_manager(), channel);
    voe::Channel* channelPtr = sc.ChannelPtr();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "GetOnHoldStatus() failed to locate channel");
        return -1;
    }
    return channelPtr->GetOnHoldStatus(enabled, mode);
}

int32_t VoEBaseImpl::StartPlayout()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEBaseImpl::StartPlayout()");
    if (_shared->audio_device()->Playing())
    {
        return 0;
    }
    if (!_shared->ext_playout())
    {
        if (_shared->audio_device()->InitPlayout() != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceVoice,
                VoEId(_shared->instance_id(), -1),
                "StartPlayout() failed to initialize playout");
            return -1;
        }
        if (_shared->audio_device()->StartPlayout() != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceVoice,
                VoEId(_shared->instance_id(), -1),
                "StartPlayout() failed to start playout");
            return -1;
        }
    }
    return 0;
}

int32_t VoEBaseImpl::StopPlayout()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEBaseImpl::StopPlayout()");

    int32_t numOfChannels = _shared->channel_manager().NumOfChannels();
    if (numOfChannels <= 0)
    {
        return 0;
    }

    uint16_t nChannelsPlaying(0);
    int32_t* channelsArray = new int32_t[numOfChannels];

    
    _shared->channel_manager().GetChannelIds(channelsArray, numOfChannels);
    for (int i = 0; i < numOfChannels; i++)
    {
        voe::ScopedChannel sc(_shared->channel_manager(), channelsArray[i]);
        voe::Channel* chPtr = sc.ChannelPtr();
        if (chPtr)
        {
            if (chPtr->Playing())
            {
                nChannelsPlaying++;
            }
        }
    }
    delete[] channelsArray;

    
    if (nChannelsPlaying == 0)
    {
        if (_shared->audio_device()->StopPlayout() != 0)
        {
            _shared->SetLastError(VE_CANNOT_STOP_PLAYOUT, kTraceError,
                "StopPlayout() failed to stop playout");
            return -1;
        }
    }
    return 0;
}

int32_t VoEBaseImpl::StartSend()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEBaseImpl::StartSend()");
    if (_shared->audio_device()->Recording())
    {
        return 0;
    }
    if (!_shared->ext_recording())
    {
        if (_shared->audio_device()->InitRecording() != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceVoice,
                VoEId(_shared->instance_id(), -1),
                "StartSend() failed to initialize recording");
            return -1;
        }
        if (_shared->audio_device()->StartRecording() != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceVoice,
                VoEId(_shared->instance_id(), -1),
                "StartSend() failed to start recording");
            return -1;
        }
    }

    return 0;
}

int32_t VoEBaseImpl::StopSend()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEBaseImpl::StopSend()");

    if (_shared->NumOfSendingChannels() == 0 &&
        !_shared->transmit_mixer()->IsRecordingMic())
    {
        
        if (_shared->audio_device()->StopRecording() != 0)
        {
            _shared->SetLastError(VE_CANNOT_STOP_RECORDING, kTraceError,
                "StopSend() failed to stop recording");
            return -1;
        }
        _shared->transmit_mixer()->StopSend();
    }

    return 0;
}

int32_t VoEBaseImpl::TerminateInternal()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoEBaseImpl::TerminateInternal()");

    
    int32_t numOfChannels = _shared->channel_manager().NumOfChannels();
    if (numOfChannels > 0)
    {
        int32_t* channelsArray = new int32_t[numOfChannels];
        _shared->channel_manager().GetChannelIds(channelsArray, numOfChannels);
        for (int i = 0; i < numOfChannels; i++)
        {
            DeleteChannel(channelsArray[i]);
        }
        delete[] channelsArray;
    }

    if (_shared->process_thread())
    {
        if (_shared->audio_device())
        {
            if (_shared->process_thread()->
                    DeRegisterModule(_shared->audio_device()) != 0)
            {
                _shared->SetLastError(VE_THREAD_ERROR, kTraceError,
                    "TerminateInternal() failed to deregister ADM");
            }
        }
        if (_shared->process_thread()->Stop() != 0)
        {
            _shared->SetLastError(VE_THREAD_ERROR, kTraceError,
                "TerminateInternal() failed to stop module process thread");
        }
    }

    if (_shared->audio_device())
    {
        if (_shared->audio_device()->StopPlayout() != 0)
        {
            _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceWarning,
                "TerminateInternal() failed to stop playout");
        }
        if (_shared->audio_device()->StopRecording() != 0)
        {
            _shared->SetLastError(VE_SOUNDCARD_ERROR, kTraceWarning,
                "TerminateInternal() failed to stop recording");
        }
        if (_shared->audio_device()->RegisterEventObserver(NULL) != 0) {
          _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceWarning,
              "TerminateInternal() failed to de-register event observer "
              "for the ADM");
        }
        if (_shared->audio_device()->RegisterAudioCallback(NULL) != 0) {
          _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceWarning,
              "TerminateInternal() failed to de-register audio callback "
              "for the ADM");
        }
        if (_shared->audio_device()->Terminate() != 0)
        {
            _shared->SetLastError(VE_AUDIO_DEVICE_MODULE_ERROR, kTraceError,
                "TerminateInternal() failed to terminate the ADM");
        }
        _shared->set_audio_device(NULL);
    }

    if (_shared->audio_processing()) {
        _shared->set_audio_processing(NULL);
    }

    return _shared->statistics().SetUnInitialized();
}

} 
