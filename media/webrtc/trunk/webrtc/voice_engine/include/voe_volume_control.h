


































#ifndef WEBRTC_VOICE_ENGINE_VOE_VOLUME_CONTROL_H
#define WEBRTC_VOICE_ENGINE_VOE_VOLUME_CONTROL_H

#include "webrtc/common_types.h"

namespace webrtc {

class VoiceEngine;

class WEBRTC_DLLEXPORT VoEVolumeControl
{
public:
    
    
    
    static VoEVolumeControl* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    virtual int SetSpeakerVolume(unsigned int volume) = 0;

    
    virtual int GetSpeakerVolume(unsigned int& volume) = 0;

    
    virtual int SetSystemOutputMute(bool enable) = 0;

    
    virtual int GetSystemOutputMute(bool &enabled) = 0;

    
    virtual int SetMicVolume(unsigned int volume) = 0;

    
    virtual int GetMicVolume(unsigned int& volume) = 0;

    
    
    virtual int SetInputMute(int channel, bool enable) = 0;

    
    virtual int GetInputMute(int channel, bool& enabled) = 0;

    
    virtual int SetSystemInputMute(bool enable) = 0;

    
    virtual int GetSystemInputMute(bool& enabled) = 0;

    
    
    virtual int GetSpeechInputLevel(unsigned int& level) = 0;

    
    
    virtual int GetSpeechOutputLevel(int channel, unsigned int& level) = 0;

    
    
    virtual int GetSpeechInputLevelFullRange(unsigned int& level) = 0;

    
    virtual int GetSpeechOutputLevelFullRange(
        int channel, unsigned int& level) = 0;

    
    
    virtual int SetChannelOutputVolumeScaling(int channel, float scaling) = 0;

    
    virtual int GetChannelOutputVolumeScaling(int channel, float& scaling) = 0;

    
    
    virtual int SetOutputVolumePan(int channel, float left, float right) = 0;

    
    virtual int GetOutputVolumePan(int channel, float& left, float& right) = 0;

protected:
    VoEVolumeControl() {};
    virtual ~VoEVolumeControl() {};
};

}  

#endif  
