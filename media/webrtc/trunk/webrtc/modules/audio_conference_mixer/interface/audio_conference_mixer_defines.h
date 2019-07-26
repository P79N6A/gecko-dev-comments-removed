









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_INTERFACE_AUDIO_CONFERENCE_MIXER_DEFINES_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_INTERFACE_AUDIO_CONFERENCE_MIXER_DEFINES_H_

#include "map_wrapper.h"
#include "module_common_types.h"
#include "typedefs.h"

namespace webrtc {
class MixHistory;


class MixerParticipant
{
public:
    
    
    
    
    virtual WebRtc_Word32 GetAudioFrame(const WebRtc_Word32 id,
                                        AudioFrame& audioFrame) = 0;

    
    WebRtc_Word32 IsMixed(bool& mixed) const;

    
    
    virtual WebRtc_Word32 NeededFrequency(const WebRtc_Word32 id) = 0;

    MixHistory* _mixHistory;
protected:
    MixerParticipant();
    virtual ~MixerParticipant();
};


struct ParticipantStatistics
{
    WebRtc_Word32 participant;
    WebRtc_Word32 level;
};

class AudioMixerStatusReceiver
{
public:
    
    
    virtual void MixedParticipants(
        const WebRtc_Word32 id,
        const ParticipantStatistics* participantStatistics,
        const WebRtc_UWord32 size) = 0;
    
    
    virtual void VADPositiveParticipants(
        const WebRtc_Word32 id,
        const ParticipantStatistics* participantStatistics,
        const WebRtc_UWord32 size) = 0;
    
    
    virtual void MixedAudioLevel(
        const WebRtc_Word32  id,
        const WebRtc_UWord32 level) = 0;
protected:
    AudioMixerStatusReceiver() {}
    virtual ~AudioMixerStatusReceiver() {}
};

class AudioMixerOutputReceiver
{
public:
    
    
    
    virtual void NewMixedAudio(const WebRtc_Word32 id,
                               const AudioFrame& generalAudioFrame,
                               const AudioFrame** uniqueAudioFrames,
                               const WebRtc_UWord32 size) = 0;
protected:
    AudioMixerOutputReceiver() {}
    virtual ~AudioMixerOutputReceiver() {}
};

class AudioRelayReceiver
{
public:
    
    
    
    virtual void NewAudioToRelay(const WebRtc_Word32 id,
                                 const MapWrapper& mixerList) = 0;
protected:
    AudioRelayReceiver() {}
    virtual ~AudioRelayReceiver() {}
};
} 

#endif 
