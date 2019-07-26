































#ifndef WEBRTC_VOICE_ENGINE_VOE_VIDEO_SYNC_H
#define WEBRTC_VOICE_ENGINE_VOE_VIDEO_SYNC_H

#include "webrtc/common_types.h"

namespace webrtc {

class RtpReceiver;
class RtpRtcp;
class VoiceEngine;

class WEBRTC_DLLEXPORT VoEVideoSync
{
public:
    
    
    
    static VoEVideoSync* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    virtual int GetPlayoutBufferSize(int& buffer_ms) = 0;

    
    
    
    
    
    virtual int SetMinimumPlayoutDelay(int channel, int delay_ms) = 0;

    
    
    
    
    
    virtual int SetInitialPlayoutDelay(int channel, int delay_ms) = 0;

    
    
    virtual int GetDelayEstimate(int channel,
                                 int* jitter_buffer_delay_ms,
                                 int* playout_buffer_delay_ms) = 0;

    
    
    
    
    virtual int GetLeastRequiredDelayMs(int channel) const = 0;

    
    virtual int SetInitTimestamp(int channel, unsigned int timestamp) = 0;

    
    virtual int SetInitSequenceNumber(int channel, short sequenceNumber) = 0;

    
    virtual int GetPlayoutTimestamp(int channel, unsigned int& timestamp) = 0;

    virtual int GetRtpRtcp (int channel, RtpRtcp** rtpRtcpModule,
                            RtpReceiver** rtp_receiver) = 0;

protected:
    VoEVideoSync() { }
    virtual ~VoEVideoSync() { }
};

}  

#endif  
