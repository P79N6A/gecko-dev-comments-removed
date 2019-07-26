









#ifndef WEBRTC_VOICE_ENGINE_VOE_VIDEO_SYNC_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_VIDEO_SYNC_IMPL_H

#include "webrtc/voice_engine/include/voe_video_sync.h"

#include "webrtc/voice_engine/shared_data.h"

namespace webrtc {

class VoEVideoSyncImpl : public VoEVideoSync
{
public:
    virtual int GetPlayoutBufferSize(int& bufferMs);

    virtual int SetMinimumPlayoutDelay(int channel, int delayMs);

    virtual int SetCurrentSyncOffset(int channel, int offsetMs);

    virtual int SetInitialPlayoutDelay(int channel, int delay_ms);

    virtual int GetDelayEstimate(int channel,
                                 int* jitter_buffer_delay_ms,
                                 int* playout_buffer_delay_ms,
                                 int* avsync_offset_ms);

    virtual int GetLeastRequiredDelayMs(int channel) const;

    virtual int SetInitTimestamp(int channel, unsigned int timestamp);

    virtual int SetInitSequenceNumber(int channel, short sequenceNumber);

    virtual int GetPlayoutTimestamp(int channel, unsigned int& timestamp);

    virtual int GetRtpRtcp(int channel, RtpRtcp** rtpRtcpModule,
                           RtpReceiver** rtp_receiver);

protected:
    VoEVideoSyncImpl(voe::SharedData* shared);
    virtual ~VoEVideoSyncImpl();

private:
    voe::SharedData* _shared;
};

}  

#endif    
