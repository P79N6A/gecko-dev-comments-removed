









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


    virtual int GetAudioFrame(int channel, int desired_sample_rate_hz,
                              AudioFrame* frame);

    virtual int SetExternalMixing(int channel, bool enable);

protected:
    VoEExternalMediaImpl(voe::SharedData* shared);
    virtual ~VoEExternalMediaImpl();

private:
    voe::SharedData* shared_;
};

}  

#endif  
