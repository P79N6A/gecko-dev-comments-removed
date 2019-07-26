









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_INTERFACE_AUDIO_CONFERENCE_MIXER_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_INTERFACE_AUDIO_CONFERENCE_MIXER_H_

#include "webrtc/modules/audio_conference_mixer/interface/audio_conference_mixer_defines.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/interface/module_common_types.h"

namespace webrtc {
class AudioMixerOutputReceiver;
class AudioMixerStatusReceiver;
class MixerParticipant;
class Trace;

class AudioConferenceMixer : public Module
{
public:
    enum {kMaximumAmountOfMixedParticipants = 3};
    enum Frequency
    {
        kNbInHz           = 8000,
        kWbInHz           = 16000,
        kSwbInHz          = 32000,
        kFbInHz           = 48000,
        kLowestPossible   = -1,
        kDefaultFrequency = kWbInHz
    };

    
    static AudioConferenceMixer* Create(int id);
    virtual ~AudioConferenceMixer() {}

    
    virtual int32_t ChangeUniqueId(const int32_t id) = 0;
    virtual int32_t TimeUntilNextProcess() = 0 ;
    virtual int32_t Process() = 0;

    
    virtual int32_t RegisterMixedStreamCallback(
        AudioMixerOutputReceiver& receiver) = 0;
    virtual int32_t UnRegisterMixedStreamCallback() = 0;

    
    virtual int32_t RegisterMixerStatusCallback(
        AudioMixerStatusReceiver& mixerStatusCallback,
        const uint32_t amountOf10MsBetweenCallbacks) = 0;
    virtual int32_t UnRegisterMixerStatusCallback() = 0;

    
    virtual int32_t SetMixabilityStatus(MixerParticipant& participant,
                                        const bool mixable) = 0;
    
    virtual int32_t MixabilityStatus(MixerParticipant& participant,
                                     bool& mixable) = 0;

    
    
    
    
    virtual int32_t SetAnonymousMixabilityStatus(MixerParticipant& participant,
                                                 const bool mixable) = 0;
    
    virtual int32_t AnonymousMixabilityStatus(MixerParticipant& participant,
                                              bool& mixable) = 0;

    
    
    
    virtual int32_t SetMinimumMixingFrequency(Frequency freq) = 0;

protected:
    AudioConferenceMixer() {}
};
}  

#endif
