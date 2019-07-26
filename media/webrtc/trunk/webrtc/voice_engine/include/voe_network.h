
































#ifndef WEBRTC_VOICE_ENGINE_VOE_NETWORK_H
#define WEBRTC_VOICE_ENGINE_VOE_NETWORK_H

#include "webrtc/common_types.h"

namespace webrtc {

class VoiceEngine;


class WEBRTC_DLLEXPORT VoEConnectionObserver
{
public:
    
    
    
    virtual void OnPeriodicDeadOrAlive(int channel, bool alive) = 0;

protected:
    virtual ~VoEConnectionObserver() {}
};


class WEBRTC_DLLEXPORT VoENetwork
{
public:
    
    
    
    static VoENetwork* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    virtual int RegisterExternalTransport(
        int channel, Transport& transport) = 0;

    
    
    virtual int DeRegisterExternalTransport(int channel) = 0;

    
    
    
    virtual int ReceivedRTPPacket(
        int channel, const void* data, unsigned int length) = 0;

    
    
    
    virtual int ReceivedRTCPPacket(
        int channel, const void* data, unsigned int length) = 0;

protected:
    VoENetwork() {}
    virtual ~VoENetwork() {}
};

}  

#endif  
