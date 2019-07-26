









#ifndef WEBRTC_VOICE_ENGINE_VOE_EXTERNAL_MEDIA_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_EXTERNAL_MEDIA_IMPL_H

#include "webrtc/voice_engine/include/voe_external_media.h"

#include "webrtc/voice_engine/shared_data.h"

namespace webrtc {

class VoEExternalMediaImpl : public VoEExternalMedia
{
public:
    virtual int RegisterExternalMediaProcessing(
        int channel,
        ProcessingTypes type,
        VoEMediaProcess& processObject);

    virtual int DeRegisterExternalMediaProcessing(
        int channel,
        ProcessingTypes type);

    virtual int SetExternalRecordingStatus(bool enable);

    virtual int SetExternalPlayoutStatus(bool enable);

    virtual int ExternalRecordingInsertData(
        const int16_t speechData10ms[],
        int lengthSamples,
        int samplingFreqHz,
        int current_delay_ms);

    virtual int ExternalPlayoutGetData(int16_t speechData10ms[],
                                       int samplingFreqHz,
                                       int current_delay_ms,
                                       int& lengthSamples);

    virtual int GetAudioFrame(int channel, int desired_sample_rate_hz,
                              AudioFrame* frame);

    virtual int SetExternalMixing(int channel, bool enable);

protected:
    VoEExternalMediaImpl(voe::SharedData* shared);
    virtual ~VoEExternalMediaImpl();

private:
#ifdef WEBRTC_VOE_EXTERNAL_REC_AND_PLAYOUT
    int playout_delay_ms_;
#endif
    voe::SharedData* shared_;
};

}  

#endif  
