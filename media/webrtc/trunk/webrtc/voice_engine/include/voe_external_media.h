






























#ifndef WEBRTC_VOICE_ENGINE_VOE_EXTERNAL_MEDIA_H
#define WEBRTC_VOICE_ENGINE_VOE_EXTERNAL_MEDIA_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;
class AudioFrame;

class WEBRTC_DLLEXPORT VoEMediaProcess
{
public:
    
    
    
    
    
    
    
    
    
    virtual void Process(const int channel, const ProcessingTypes type,
                         WebRtc_Word16 audio10ms[], const int length,
                         const int samplingFreq, const bool isStereo) = 0;

protected:
    virtual ~VoEMediaProcess() {}
};

class WEBRTC_DLLEXPORT VoEExternalMedia
{
public:
    
    
    
    static VoEExternalMedia* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    virtual int RegisterExternalMediaProcessing(
        int channel, ProcessingTypes type, VoEMediaProcess& processObject) = 0;

    
    
    virtual int DeRegisterExternalMediaProcessing(
        int channel, ProcessingTypes type) = 0;

    
    virtual int SetExternalRecordingStatus(bool enable) = 0;

    
    virtual int SetExternalPlayoutStatus(bool enable) = 0;

    
    
    
    virtual int ExternalRecordingInsertData(
        const WebRtc_Word16 speechData10ms[], int lengthSamples,
        int samplingFreqHz, int current_delay_ms) = 0;

    
    
    
    
    
    virtual int ExternalPlayoutGetData(
        WebRtc_Word16 speechData10ms[], int samplingFreqHz,
        int current_delay_ms, int& lengthSamples) = 0;

    
    
    
    
    virtual int GetAudioFrame(int channel, int desired_sample_rate_hz,
                              AudioFrame* frame) = 0;

    
    virtual int SetExternalMixing(int channel, bool enable) = 0;

protected:
    VoEExternalMedia() {}
    virtual ~VoEExternalMedia() {}
};

}  

#endif  
