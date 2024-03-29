









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_MIXER_MANAGER_PULSE_LINUX_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_MIXER_MANAGER_PULSE_LINUX_H

#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/modules/audio_device/linux/pulseaudiosymboltable_linux.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/typedefs.h"

#include <pulse/pulseaudio.h>
#include <stdint.h>

#ifndef UINT32_MAX
#define UINT32_MAX  ((uint32_t)-1)
#endif

namespace webrtc
{

class AudioMixerManagerLinuxPulse
{
public:
    int32_t SetPlayStream(pa_stream* playStream);
    int32_t SetRecStream(pa_stream* recStream);
    int32_t OpenSpeaker(uint16_t deviceIndex);
    int32_t OpenMicrophone(uint16_t deviceIndex);
    int32_t SetSpeakerVolume(uint32_t volume);
    int32_t SpeakerVolume(uint32_t& volume) const;
    int32_t MaxSpeakerVolume(uint32_t& maxVolume) const;
    int32_t MinSpeakerVolume(uint32_t& minVolume) const;
    int32_t SpeakerVolumeStepSize(uint16_t& stepSize) const;
    int32_t SpeakerVolumeIsAvailable(bool& available);
    int32_t SpeakerMuteIsAvailable(bool& available);
    int32_t SetSpeakerMute(bool enable);
    int32_t StereoPlayoutIsAvailable(bool& available);
    int32_t StereoRecordingIsAvailable(bool& available);
    int32_t SpeakerMute(bool& enabled) const;
    int32_t MicrophoneMuteIsAvailable(bool& available);
    int32_t SetMicrophoneMute(bool enable);
    int32_t MicrophoneMute(bool& enabled) const;
    int32_t MicrophoneBoostIsAvailable(bool& available);
    int32_t SetMicrophoneBoost(bool enable);
    int32_t MicrophoneBoost(bool& enabled) const;
    int32_t MicrophoneVolumeIsAvailable(bool& available);
    int32_t SetMicrophoneVolume(uint32_t volume);
    int32_t MicrophoneVolume(uint32_t& volume) const;
    int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const;
    int32_t MinMicrophoneVolume(uint32_t& minVolume) const;
    int32_t MicrophoneVolumeStepSize(uint16_t& stepSize) const;
    int32_t SetPulseAudioObjects(pa_threaded_mainloop* mainloop,
                                 pa_context* context);
    int32_t Close();
    int32_t CloseSpeaker();
    int32_t CloseMicrophone();
    bool SpeakerIsInitialized() const;
    bool MicrophoneIsInitialized() const;

public:
    AudioMixerManagerLinuxPulse(const int32_t id);
    ~AudioMixerManagerLinuxPulse();

private:
    static void PaSinkInfoCallback(pa_context *c, const pa_sink_info *i,
                                   int eol, void *pThis);
    static void PaSinkInputInfoCallback(pa_context *c,
                                        const pa_sink_input_info *i, int eol,
                                        void *pThis);
    static void PaSourceInfoCallback(pa_context *c, const pa_source_info *i,
                                     int eol, void *pThis);
    static void
        PaSetVolumeCallback(pa_context* , int success, void* );
    void PaSinkInfoCallbackHandler(const pa_sink_info *i, int eol);
    void PaSinkInputInfoCallbackHandler(const pa_sink_input_info *i, int eol);
    void PaSourceInfoCallbackHandler(const pa_source_info *i, int eol);

    void ResetCallbackVariables() const;
    void WaitForOperationCompletion(pa_operation* paOperation) const;
    void PaLock() const;
    void PaUnLock() const;

    bool GetSinkInputInfo() const;
    bool GetSinkInfoByIndex(int device_index)const ;
    bool GetSourceInfoByIndex(int device_index) const;

private:
    CriticalSectionWrapper& _critSect;
    int32_t _id;
    int16_t _paOutputDeviceIndex;
    int16_t _paInputDeviceIndex;

    pa_stream* _paPlayStream;
    pa_stream* _paRecStream;

    pa_threaded_mainloop* _paMainloop;
    pa_context* _paContext;

    mutable uint32_t _paVolume;
    mutable uint32_t _paMute;
    mutable uint32_t _paVolSteps;
    bool _paSpeakerMute;
    mutable uint32_t _paSpeakerVolume;
    mutable uint8_t _paChannels;
    bool _paObjectsSet;
    mutable bool _callbackValues;
};

}

#endif  
