









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_AUDIO_CONFERENCE_MIXER_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_AUDIO_CONFERENCE_MIXER_IMPL_H_

#include "audio_conference_mixer.h"
#include "engine_configurations.h"
#include "level_indicator.h"
#include "list_wrapper.h"
#include "memory_pool.h"
#include "module_common_types.h"
#include "scoped_ptr.h"
#include "time_scheduler.h"

namespace webrtc {
class AudioProcessing;
class CriticalSectionWrapper;


class MixHistory
{
public:
    MixHistory();
    ~MixHistory();

    
    WebRtc_Word32 IsMixed(bool& mixed) const;

    
    
    WebRtc_Word32 WasMixed(bool& wasMixed) const;

    
    WebRtc_Word32 SetIsMixed(const bool mixed);

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

    
    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);
    virtual WebRtc_Word32 TimeUntilNextProcess();
    virtual WebRtc_Word32 Process();

    
    virtual WebRtc_Word32 RegisterMixedStreamCallback(
        AudioMixerOutputReceiver& mixReceiver);
    virtual WebRtc_Word32 UnRegisterMixedStreamCallback();
    virtual WebRtc_Word32 RegisterMixerStatusCallback(
        AudioMixerStatusReceiver& mixerStatusCallback,
        const WebRtc_UWord32 amountOf10MsBetweenCallbacks);
    virtual WebRtc_Word32 UnRegisterMixerStatusCallback();
    virtual WebRtc_Word32 SetMixabilityStatus(MixerParticipant& participant,
                                              const bool mixable);
    virtual WebRtc_Word32 MixabilityStatus(MixerParticipant& participant,
                                           bool& mixable);
    virtual WebRtc_Word32 SetMinimumMixingFrequency(Frequency freq);
    virtual WebRtc_Word32 SetAnonymousMixabilityStatus(
        MixerParticipant& participant, const bool mixable);
    virtual WebRtc_Word32 AnonymousMixabilityStatus(
        MixerParticipant& participant, bool& mixable);
private:
    enum{DEFAULT_AUDIO_FRAME_POOLSIZE = 50};

    
    WebRtc_Word32 SetOutputFrequency(const Frequency frequency);
    Frequency OutputFrequency() const;

    
    
    bool SetNumLimiterChannels(int numChannels);

    
    
    
    
    
    
    
    
    void UpdateToMix(ListWrapper& mixList, ListWrapper& rampOutList,
                     MapWrapper& mixParticipantList,
                     WebRtc_UWord32& maxAudioFrameCounter);

    
    
    WebRtc_Word32 GetLowestMixingFrequency();
    WebRtc_Word32 GetLowestMixingFrequencyFromList(ListWrapper& mixList);

    
    void GetAdditionalAudio(ListWrapper& additionalFramesList);

    
    
    void UpdateMixedStatus(MapWrapper& mixedParticipantsList);

    
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

    
    WebRtc_Word32 MixFromList(
        AudioFrame& mixedAudio,
        const ListWrapper& audioFrameList);
    
    
    
    WebRtc_Word32 MixAnonomouslyFromList(AudioFrame& mixedAudio,
                                         const ListWrapper& audioFrameList);

    bool LimitMixedAudio(AudioFrame& mixedAudio);

    
    
    
    WebRtc_UWord32         _scratchParticipantsToMixAmount;
    ParticipantStatistics  _scratchMixedParticipants[
        kMaximumAmountOfMixedParticipants];
    WebRtc_UWord32         _scratchVadPositiveParticipantsAmount;
    ParticipantStatistics  _scratchVadPositiveParticipants[
        kMaximumAmountOfMixedParticipants];

    scoped_ptr<CriticalSectionWrapper> _crit;
    scoped_ptr<CriticalSectionWrapper> _cbCrit;

    WebRtc_Word32 _id;

    Frequency _minimumMixingFreq;

    
    AudioMixerOutputReceiver* _mixReceiver;

    AudioMixerStatusReceiver* _mixerStatusCallback;
    WebRtc_UWord32            _amountOf10MsBetweenCallbacks;
    WebRtc_UWord32            _amountOf10MsUntilNextCallback;
    bool                      _mixerStatusCb;

    
    Frequency _outputFrequency;
    WebRtc_UWord16 _sampleSize;

    
    MemoryPool<AudioFrame>* _audioFramePool;

    
    ListWrapper _participantList;              
    ListWrapper _additionalParticipantList;    

    WebRtc_UWord32 _numMixedParticipants;

    WebRtc_UWord32 _timeStamp;

    
    TimeScheduler _timeScheduler;

    
    LevelIndicator _mixedAudioLevel;

    
    
    WebRtc_Word16 _processCalls;

    
    scoped_ptr<AudioProcessing> _limiter;
};
} 

#endif
