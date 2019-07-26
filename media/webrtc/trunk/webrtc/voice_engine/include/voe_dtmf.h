































#ifndef WEBRTC_VOICE_ENGINE_VOE_DTMF_H
#define WEBRTC_VOICE_ENGINE_VOE_DTMF_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;


class WEBRTC_DLLEXPORT VoETelephoneEventObserver
{
public:
    
    
    
    virtual void OnReceivedTelephoneEventInband(int channel,
                                                int eventCode,
                                                bool endOfEvent) = 0;

    
    
    
    virtual void OnReceivedTelephoneEventOutOfBand(
        int channel,
        int eventCode,
        bool endOfEvent) = 0;

protected:
    virtual ~VoETelephoneEventObserver() {}
};


class WEBRTC_DLLEXPORT VoEDtmf
{
public:
    
    
    
    
    static VoEDtmf* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    virtual int SendTelephoneEvent(int channel, int eventCode,
                                   bool outOfBand = true, int lengthMs = 160,
                                   int attenuationDb = 10) = 0;

   
    
    
    virtual int SetSendTelephoneEventPayloadType(int channel,
                                                 unsigned char type) = 0;

  
    
    virtual int GetSendTelephoneEventPayloadType(int channel,
                                                 unsigned char& type) = 0;

    
    
    virtual int SetDtmfPlayoutStatus(int channel, bool enable) = 0;

    
    virtual int GetDtmfPlayoutStatus(int channel, bool& enabled) = 0;

    
    
    virtual int SetDtmfFeedbackStatus(bool enable,
                                      bool directFeedback = false) = 0;

    
    virtual int GetDtmfFeedbackStatus(bool& enabled, bool& directFeedback) = 0;

    
    virtual int PlayDtmfTone(int eventCode, int lengthMs = 200,
                             int attenuationDb = 10) = 0;

    
    
    
    virtual int StartPlayingDtmfTone(int eventCode,
                                     int attenuationDb = 10) = 0;

    
    virtual int StopPlayingDtmfTone() = 0;

    
    
    virtual int RegisterTelephoneEventDetection(
        int channel, TelephoneEventDetectionMethods detectionMethod,
        VoETelephoneEventObserver& observer) = 0;

    
    
    virtual int DeRegisterTelephoneEventDetection(int channel) = 0;

    
    
    virtual int GetTelephoneEventDetectionStatus(
        int channel, bool& enabled,
        TelephoneEventDetectionMethods& detectionMethod) = 0;

protected:
    VoEDtmf() {}
    virtual ~VoEDtmf() {}
};

}  

#endif  
