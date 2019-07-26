









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_AUDIO_CONFERENCE_MIXER_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_AUDIO_CONFERENCE_MIXER_IMPL_H_

#include <map>

#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_conference_mixer/interface/audio_conference_mixer.h"
#include "webrtc/modules/audio_conference_mixer/source/level_indicator.h"
#include "webrtc/modules/audio_conference_mixer/source/memory_pool.h"
#include "webrtc/modules/audio_conference_mixer/source/time_scheduler.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/list_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
class AudioProcessing;
class CriticalSectionWrapper;


class MixHistory
{
public:
    MixHistory();
    ~MixHistory();

    
    int32_t IsMixed(bool& mixed) const;

    
    
    int32_t WasMixed(bool& wasMixed) const;

    
    int32_t SetIsMixed(const bool mixed);

    void ResetMixedStatus();
private:
    bool _isMixed;
};

class AudioConferenceMixerImpl : public AudioConferenceMixer
{
public:
    
    enum {kProcessPeriodicityInMs = 10};

    AudioConferenceMixerImpl(int id);
    ~AudioConferenceMixerImpl();

    
    bool Init();

    
    virtual int32_t ChangeUniqueId(const int32_t id);
    virtual int32_t TimeUntilNextProcess();
    virtual int32_t Process();

    
    virtual int32_t RegisterMixedStreamCallback(
        AudioMixerOutputReceiver& mixReceiver);
    virtual int32_t UnRegisterMixedStreamCallback();
    virtual int32_t RegisterMixerStatusCallback(
        AudioMixerStatusReceiver& mixerStatusCallback,
        const uint32_t amountOf10MsBetweenCallbacks);
    virtual int32_t UnRegisterMixerStatusCallback();
    virtual int32_t SetMixabilityStatus(MixerParticipant& participant,
                                        const bool mixable);
    virtual int32_t MixabilityStatus(MixerParticipant& participant,
                                     bool& mixable);
    virtual int32_t SetMinimumMixingFrequency(Frequency freq);
    virtual int32_t SetAnonymousMixabilityStatus(
        MixerParticipant& participant, const bool mixable);
    virtual int32_t AnonymousMixabilityStatus(
        MixerParticipant& participant, bool& mixable);
private:
    enum{DEFAULT_AUDIO_FRAME_POOLSIZE = 50};

    
    int32_t SetOutputFrequency(const Frequency frequency);
    Frequency OutputFrequency() const;

    
    
    bool SetNumLimiterChannels(int numChannels);

    
    
    
    
    
    
    
    
    void UpdateToMix(
        ListWrapper& mixList,
        ListWrapper& rampOutList,
        std::map<int, MixerParticipant*>* mixParticipantList,
        uint32_t& maxAudioFrameCounter);

    
    
    int32_t GetLowestMixingFrequency();
    int32_t GetLowestMixingFrequencyFromList(ListWrapper& mixList);

    
    void GetAdditionalAudio(ListWrapper& additionalFramesList);

    
    
    void UpdateMixedStatus(
        std::map<int, MixerParticipant*>& mixedParticipantsList);

    
    void ClearAudioFrameList(ListWrapper& audioFrameList);

    
    
    void UpdateVADPositiveParticipants(
        ListWrapper& mixList);

    
    
    bool IsParticipantInList(
        MixerParticipant& participant,
        ListWrapper& participantList);

    
    
    bool AddParticipantToList(
        MixerParticipant& participant,
        ListWrapper& participantList);
    bool RemoveParticipantFromList(
        MixerParticipant& removeParticipant,
        ListWrapper& participantList);

    
    int32_t MixFromList(
        AudioFrame& mixedAudio,
        const ListWrapper& audioFrameList);
    
    
    
    int32_t MixAnonomouslyFromList(AudioFrame& mixedAudio,
                                   const ListWrapper& audioFrameList);

    bool LimitMixedAudio(AudioFrame& mixedAudio);

    
    
    
    uint32_t         _scratchParticipantsToMixAmount;
    ParticipantStatistics  _scratchMixedParticipants[
        kMaximumAmountOfMixedParticipants];
    uint32_t         _scratchVadPositiveParticipantsAmount;
    ParticipantStatistics  _scratchVadPositiveParticipants[
        kMaximumAmountOfMixedParticipants];

    scoped_ptr<CriticalSectionWrapper> _crit;
    scoped_ptr<CriticalSectionWrapper> _cbCrit;

    int32_t _id;

    Frequency _minimumMixingFreq;

    
    AudioMixerOutputReceiver* _mixReceiver;

    AudioMixerStatusReceiver* _mixerStatusCallback;
    uint32_t            _amountOf10MsBetweenCallbacks;
    uint32_t            _amountOf10MsUntilNextCallback;
    bool                      _mixerStatusCb;

    
    Frequency _outputFrequency;
    uint16_t _sampleSize;

    
    MemoryPool<AudioFrame>* _audioFramePool;

    
    ListWrapper _participantList;              
    ListWrapper _additionalParticipantList;    

    uint32_t _numMixedParticipants;

    uint32_t _timeStamp;

    
    TimeScheduler _timeScheduler;

    
    LevelIndicator _mixedAudioLevel;

    
    
    int16_t _processCalls;

    
    scoped_ptr<AudioProcessing> _limiter;
};
}  

#endif
