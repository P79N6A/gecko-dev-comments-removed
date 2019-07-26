

































#ifndef WEBRTC_VOICE_ENGINE_VOE_AUDIO_PROCESSING_H
#define WEBRTC_VOICE_ENGINE_VOE_AUDIO_PROCESSING_H

#include <stdio.h>

#include "webrtc/common_types.h"

namespace webrtc {

class VoiceEngine;


class WEBRTC_DLLEXPORT VoERxVadCallback
{
public:
    virtual void OnRxVad(int channel, int vadDecision) = 0;

protected:
    virtual ~VoERxVadCallback() {}
};


class WEBRTC_DLLEXPORT VoEAudioProcessing
{
public:
    
    
    
    static VoEAudioProcessing* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    virtual int SetNsStatus(bool enable, NsModes mode = kNsUnchanged) = 0;

    
    virtual int GetNsStatus(bool& enabled, NsModes& mode) = 0;

    
    
    virtual int SetAgcStatus(bool enable, AgcModes mode = kAgcUnchanged) = 0;

    
    virtual int GetAgcStatus(bool& enabled, AgcModes& mode) = 0;

    
    
    
    virtual int SetAgcConfig(AgcConfig config) = 0;

    
    virtual int GetAgcConfig(AgcConfig& config) = 0;

    
    
    
    
    virtual int SetEcStatus(bool enable, EcModes mode = kEcUnchanged) = 0;

    
    virtual int GetEcStatus(bool& enabled, EcModes& mode) = 0;

    
    
    
    
    
    virtual int EnableDriftCompensation(bool enable) = 0;
    virtual bool DriftCompensationEnabled() = 0;
    static bool DriftCompensationSupported();

    
    
    
    
    
    virtual void SetDelayOffsetMs(int offset) = 0;
    virtual int DelayOffsetMs() = 0;

    
    virtual int SetAecmMode(AecmModes mode = kAecmSpeakerphone,
                            bool enableCNG = true) = 0;

    
    virtual int GetAecmMode(AecmModes& mode, bool& enabledCNG) = 0;

    
    
    virtual int EnableHighPassFilter(bool enable) = 0;
    virtual bool IsHighPassFilterEnabled() = 0;

    
    
    
    virtual int SetRxNsStatus(int channel,
                              bool enable,
                              NsModes mode = kNsUnchanged) = 0;

    
    virtual int GetRxNsStatus(int channel,
                              bool& enabled,
                              NsModes& mode) = 0;

    
    
    
    virtual int SetRxAgcStatus(int channel,
                               bool enable,
                               AgcModes mode = kAgcUnchanged) = 0;

    
    virtual int GetRxAgcStatus(int channel,
                               bool& enabled,
                               AgcModes& mode) = 0;

    
    
    virtual int SetRxAgcConfig(int channel, AgcConfig config) = 0;

    
    virtual int GetRxAgcConfig(int channel, AgcConfig& config) = 0;

    
    
    virtual int RegisterRxVadObserver(int channel,
                                      VoERxVadCallback &observer) = 0;

    
    
    virtual int DeRegisterRxVadObserver(int channel) = 0;

    
    
    
    virtual int VoiceActivityIndicator(int channel) = 0;

    
    
    
    virtual int SetEcMetricsStatus(bool enable) = 0;

    
    virtual int GetEcMetricsStatus(bool& enabled) = 0;

    
    virtual int GetEchoMetrics(int& ERL, int& ERLE, int& RERL, int& A_NLP) = 0;

    
    
    
    virtual int GetEcDelayMetrics(int& delay_median, int& delay_std) = 0;

    
    
    virtual int StartDebugRecording(const char* fileNameUTF8) = 0;

    
    
    virtual int StartDebugRecording(FILE* file_handle) = 0;

    
    virtual int StopDebugRecording() = 0;

    
    
    virtual int SetTypingDetectionStatus(bool enable) = 0;

    
    virtual int GetTypingDetectionStatus(bool& enabled) = 0;

    
    
    
    
    virtual int TimeSinceLastTyping(int &seconds) = 0;

    
    
    
    
    virtual int SetTypingDetectionParameters(int timeWindow,
                                             int costPerTyping,
                                             int reportingThreshold,
                                             int penaltyDecay,
                                             int typeEventDelay = 0) = 0;

    
    
    
    
    
    
    
    virtual void EnableStereoChannelSwapping(bool enable) = 0;
    virtual bool IsStereoChannelSwappingEnabled() = 0;

protected:
    VoEAudioProcessing() {}
    virtual ~VoEAudioProcessing() {}
};

}  

#endif  
