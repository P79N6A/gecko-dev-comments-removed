









#ifndef WEBRTC_VOICE_ENGINE_VOE_VIDEO_SYNC_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_VIDEO_SYNC_IMPL_H

#include "voe_video_sync.h"

#include "shared_data.h"

namespace webrtc {

class VoEVideoSyncImpl : public VoEVideoSync
{
public:
    virtual int GetPlayoutBufferSize(int& bufferMs);

    virtual int SetMinimumPlayoutDelay(int channel, int delayMs);

    virtual int GetDelayEstimate(int channel, int& delayMs);

    virtual int SetInitTimestamp(int channel, unsigned int timestamp);

    virtual int SetInitSequenceNumber(int channel, short sequenceNumber);

    virtual int GetPlayoutTimestamp(int channel, unsigned int& timestamp);

    virtual int GetRtpRtcp(int channel, RtpRtcp* &rtpRtcpModule);

protected:
    VoEVideoSyncImpl(voe::SharedData* shared);
    virtual ~VoEVideoSyncImpl();

private:
    voe::SharedData* _shared;
};

}   

#endif    
