









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_INTERFACE_AUDIO_CONFERENCE_MIXER_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_INTERFACE_AUDIO_CONFERENCE_MIXER_H_

#include "audio_conference_mixer_defines.h"
#include "module.h"
#include "module_common_types.h"

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

    
    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id) = 0;
    virtual WebRtc_Word32 TimeUntilNextProcess() = 0 ;
    virtual WebRtc_Word32 Process() = 0;

    
    virtual WebRtc_Word32 RegisterMixedStreamCallback(
        AudioMixerOutputReceiver& receiver) = 0;
    virtual WebRtc_Word32 UnRegisterMixedStreamCallback() = 0;

    
    virtual WebRtc_Word32 RegisterMixerStatusCallback(
        AudioMixerStatusReceiver& mixerStatusCallback,
        const WebRtc_UWord32 amountOf10MsBetweenCallbacks) = 0;
    virtual WebRtc_Word32 UnRegisterMixerStatusCallback() = 0;

    
    virtual WebRtc_Word32 SetMixabilityStatus(
        MixerParticipant& participant,
        const bool mixable) = 0;
    
    virtual WebRtc_Word32 MixabilityStatus(
        MixerParticipant& participant,
        bool& mixable) = 0;

    
    
    
    
    virtual WebRtc_Word32 SetAnonymousMixabilityStatus(
        MixerParticipant& participant, const bool mixable) = 0;
    
    virtual WebRtc_Word32 AnonymousMixabilityStatus(
        MixerParticipant& participant, bool& mixable) = 0;

    
    
    
    virtual WebRtc_Word32 SetMinimumMixingFrequency(Frequency freq) = 0;

protected:
    AudioConferenceMixer() {}
};
} 

#endif
