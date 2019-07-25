































#ifndef WEBRTC_VOICE_ENGINE_VOE_VIDEO_SYNC_H
#define WEBRTC_VOICE_ENGINE_VOE_VIDEO_SYNC_H

#include "common_types.h"

namespace webrtc {

class RtpRtcp;
class VoiceEngine;

class WEBRTC_DLLEXPORT VoEVideoSync
{
public:
    
    
    
    static VoEVideoSync* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    virtual int GetPlayoutBufferSize(int& bufferMs) = 0;

    
    virtual int SetMinimumPlayoutDelay(int channel, int delayMs) = 0;

    
    
    virtual int GetDelayEstimate(int channel, int& delayMs) = 0;

    
    virtual int SetInitTimestamp(int channel, unsigned int timestamp) = 0;

    
    virtual int SetInitSequenceNumber(int channel, short sequenceNumber) = 0;

    
    virtual int GetPlayoutTimestamp(int channel, unsigned int& timestamp) = 0;

    virtual int GetRtpRtcp (int channel, RtpRtcp* &rtpRtcpModule) = 0;

protected:
    VoEVideoSync() { }
    virtual ~VoEVideoSync() { }
};

}   

#endif  
