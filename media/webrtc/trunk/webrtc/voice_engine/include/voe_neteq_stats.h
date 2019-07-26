









#ifndef WEBRTC_VOICE_ENGINE_VOE_NETEQ_STATS_H
#define WEBRTC_VOICE_ENGINE_VOE_NETEQ_STATS_H

#include "webrtc/common_types.h"

namespace webrtc {

class VoiceEngine;

class WEBRTC_DLLEXPORT VoENetEqStats
{
public:
    
    
    
    static VoENetEqStats* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    virtual int GetNetworkStatistics(int channel, NetworkStatistics& stats) = 0;

    
    virtual int GetDecodingCallStatistics(
        int channel, AudioDecodingCallStats* stats) const = 0;

protected:
    VoENetEqStats() {}
    virtual ~VoENetEqStats() {}
};

}  

#endif    
