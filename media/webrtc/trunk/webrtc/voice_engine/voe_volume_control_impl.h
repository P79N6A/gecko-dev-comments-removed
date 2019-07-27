









#ifndef WEBRTC_VOICE_ENGINE_VOE_VOLUME_CONTROL_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_VOLUME_CONTROL_IMPL_H

#include "webrtc/voice_engine/include/voe_volume_control.h"

#include "webrtc/voice_engine/shared_data.h"

namespace webrtc {

class VoEVolumeControlImpl : public VoEVolumeControl
{
public:
    virtual int SetSpeakerVolume(unsigned int volume);

    virtual int GetSpeakerVolume(unsigned int& volume);

    virtual int SetMicVolume(unsigned int volume);

    virtual int GetMicVolume(unsigned int& volume);

    virtual int SetInputMute(int channel, bool enable);

    virtual int GetInputMute(int channel, bool& enabled);

    virtual int GetSpeechInputLevel(unsigned int& level);

    virtual int GetSpeechOutputLevel(int channel, unsigned int& level);

    virtual int GetSpeechInputLevelFullRange(unsigned int& level);

    virtual int GetSpeechOutputLevelFullRange(int channel,
                                              unsigned int& level);

    virtual int SetChannelOutputVolumeScaling(int channel, float scaling);

    virtual int GetChannelOutputVolumeScaling(int channel, float& scaling);

    virtual int SetOutputVolumePan(int channel, float left, float right);

    virtual int GetOutputVolumePan(int channel, float& left, float& right);


protected:
    VoEVolumeControlImpl(voe::SharedData* shared);
    virtual ~VoEVolumeControlImpl();

private:
    voe::SharedData* _shared;
};

}  

#endif    
