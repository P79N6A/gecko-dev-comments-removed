









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_INTERFACE_AUDIO_CONFERENCE_MIXER_DEFINES_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_INTERFACE_AUDIO_CONFERENCE_MIXER_DEFINES_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class MixHistory;


class MixerParticipant
{
public:
    
    
    
    
    virtual int32_t GetAudioFrame(const int32_t id, AudioFrame& audioFrame) = 0;

    
    int32_t IsMixed(bool& mixed) const;

    
    
    virtual int32_t NeededFrequency(const int32_t id) = 0;

    MixHistory* _mixHistory;
protected:
    MixerParticipant();
    virtual ~MixerParticipant();
};


struct ParticipantStatistics
{
    int32_t participant;
    int32_t level;
};

class AudioMixerStatusReceiver
{
public:
    
    
    virtual void MixedParticipants(
        const int32_t id,
        const ParticipantStatistics* participantStatistics,
        const uint32_t size) = 0;
    
    
    virtual void VADPositiveParticipants(
        const int32_t id,
        const ParticipantStatistics* participantStatistics,
        const uint32_t size) = 0;
    
    
    virtual void MixedAudioLevel(
        const int32_t  id,
        const uint32_t level) = 0;
protected:
    AudioMixerStatusReceiver() {}
    virtual ~AudioMixerStatusReceiver() {}
};

class AudioMixerOutputReceiver
{
public:
    
    
    
    virtual void NewMixedAudio(const int32_t id,
                               const AudioFrame& generalAudioFrame,
                               const AudioFrame** uniqueAudioFrames,
                               const uint32_t size) = 0;
protected:
    AudioMixerOutputReceiver() {}
    virtual ~AudioMixerOutputReceiver() {}
};
}  

#endif 
