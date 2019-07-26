









#include "audio_conference_mixer_defines.h"
#include "audio_conference_mixer_impl.h"
#include "audio_frame_manipulator.h"
#include "audio_processing.h"
#include "critical_section_wrapper.h"
#include "map_wrapper.h"
#include "modules/utility/interface/audio_frame_operations.h"
#include "trace.h"

namespace webrtc {
namespace {







void MixFrames(AudioFrame* mixed_frame, AudioFrame* frame) {
  assert(mixed_frame->num_channels_ >= frame->num_channels_);
  
  *frame >>= 1;
  if (mixed_frame->num_channels_ > frame->num_channels_) {
    
    assert(mixed_frame->num_channels_ == 2 &&
           frame->num_channels_ == 1);
    AudioFrameOperations::MonoToStereo(frame);
  }

  *mixed_frame += *frame;
}


int MaxNumChannels(const ListWrapper& list) {
  ListItem* item = list.First();
  int max_num_channels = 1;
  while (item) {
    AudioFrame* frame = static_cast<AudioFrame*>(item->GetItem());
    max_num_channels = std::max(max_num_channels, frame->num_channels_);
    item = list.Next(item);
  }
  return max_num_channels;
}

void SetParticipantStatistics(ParticipantStatistics* stats,
                              const AudioFrame& frame)
{
    stats->participant = frame.id_;
    stats->level = 0;  
}

}  

MixerParticipant::MixerParticipant()
    : _mixHistory(new MixHistory())
{
}

MixerParticipant::~MixerParticipant()
{
    delete _mixHistory;
}

WebRtc_Word32 MixerParticipant::IsMixed(bool& mixed) const
{
    return _mixHistory->IsMixed(mixed);
}

MixHistory::MixHistory()
    : _isMixed(0)
{
}

MixHistory::~MixHistory()
{
}

WebRtc_Word32 MixHistory::IsMixed(bool& mixed) const
{
    mixed = _isMixed;
    return 0;
}

WebRtc_Word32 MixHistory::WasMixed(bool& wasMixed) const
{
    
    
    return IsMixed(wasMixed);
}

WebRtc_Word32 MixHistory::SetIsMixed(const bool mixed)
{
    _isMixed = mixed;
    return 0;
}

void MixHistory::ResetMixedStatus()
{
    _isMixed = false;
}

AudioConferenceMixer* AudioConferenceMixer::Create(int id)
{
    AudioConferenceMixerImpl* mixer = new AudioConferenceMixerImpl(id);
    if(!mixer->Init())
    {
        delete mixer;
        return NULL;
    }
    return mixer;
}

AudioConferenceMixerImpl::AudioConferenceMixerImpl(int id)
    : _scratchParticipantsToMixAmount(0),
      _scratchMixedParticipants(),
      _scratchVadPositiveParticipantsAmount(0),
      _scratchVadPositiveParticipants(),
      _crit(NULL),
      _cbCrit(NULL),
      _id(id),
      _minimumMixingFreq(kLowestPossible),
      _mixReceiver(NULL),
      _mixerStatusCallback(NULL),
      _amountOf10MsBetweenCallbacks(1),
      _amountOf10MsUntilNextCallback(0),
      _mixerStatusCb(false),
      _outputFrequency(kDefaultFrequency),
      _sampleSize(0),
      _audioFramePool(NULL),
      _participantList(),
      _additionalParticipantList(),
      _numMixedParticipants(0),
      _timeStamp(0),
      _timeScheduler(kProcessPeriodicityInMs),
      _mixedAudioLevel(),
      _processCalls(0),
      _limiter(NULL)
{}

bool AudioConferenceMixerImpl::Init()
{
    _crit.reset(CriticalSectionWrapper::CreateCriticalSection());
    if (_crit.get() == NULL)
        return false;

    _cbCrit.reset(CriticalSectionWrapper::CreateCriticalSection());
    if(_cbCrit.get() == NULL)
        return false;

    _limiter.reset(AudioProcessing::Create(_id));
    if(_limiter.get() == NULL)
        return false;

    MemoryPool<AudioFrame>::CreateMemoryPool(_audioFramePool,
                                             DEFAULT_AUDIO_FRAME_POOLSIZE);
    if(_audioFramePool == NULL)
        return false;

    if(SetOutputFrequency(kDefaultFrequency) == -1)
        return false;

    
    if (!SetNumLimiterChannels(1))
        return false;

    if(_limiter->gain_control()->set_mode(GainControl::kFixedDigital) != 
        _limiter->kNoError)
        return false;

    
    
    
    if(_limiter->gain_control()->set_target_level_dbfs(7) != _limiter->kNoError)
        return false;

    if(_limiter->gain_control()->set_compression_gain_db(0)
        != _limiter->kNoError)
        return false;

    if(_limiter->gain_control()->enable_limiter(true) != _limiter->kNoError)
        return false;

    if(_limiter->gain_control()->Enable(true) != _limiter->kNoError)
        return false;

    return true;
}

AudioConferenceMixerImpl::~AudioConferenceMixerImpl()
{
    MemoryPool<AudioFrame>::DeleteMemoryPool(_audioFramePool);
    assert(_audioFramePool == NULL);
}

WebRtc_Word32 AudioConferenceMixerImpl::ChangeUniqueId(const WebRtc_Word32 id)
{
    _id = id;
    return 0;
}


WebRtc_Word32 AudioConferenceMixerImpl::TimeUntilNextProcess()
{
    WebRtc_Word32 timeUntilNextProcess = 0;
    CriticalSectionScoped cs(_crit.get());
    if(_timeScheduler.TimeToNextUpdate(timeUntilNextProcess) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioMixerServer, _id,
                     "failed in TimeToNextUpdate() call");
        
        assert(false);
        return -1;
    }
    return timeUntilNextProcess;
}

WebRtc_Word32 AudioConferenceMixerImpl::Process()
{
    WebRtc_UWord32 remainingParticipantsAllowedToMix =
        kMaximumAmountOfMixedParticipants;
    {
        CriticalSectionScoped cs(_crit.get());
        assert(_processCalls == 0);
        _processCalls++;

        
        _timeScheduler.UpdateScheduler();
    }

    ListWrapper mixList;
    ListWrapper rampOutList;
    ListWrapper additionalFramesList;
    MapWrapper mixedParticipantsMap;
    {
        CriticalSectionScoped cs(_cbCrit.get());

        WebRtc_Word32 lowFreq = GetLowestMixingFrequency();
        
        
        
        
        
        if (lowFreq == 12000)
        {
            lowFreq = 16000;
        } else if (lowFreq == 24000) {
            lowFreq = 32000;
        }
        if(lowFreq <= 0)
        {
            CriticalSectionScoped cs(_crit.get());
            _processCalls--;
            return 0;
        } else  {
            switch(lowFreq)
            {
            case 8000:
                if(OutputFrequency() != kNbInHz)
                {
                    SetOutputFrequency(kNbInHz);
                }
                break;
            case 16000:
                if(OutputFrequency() != kWbInHz)
                {
                    SetOutputFrequency(kWbInHz);
                }
                break;
            case 32000:
                if(OutputFrequency() != kSwbInHz)
                {
                    SetOutputFrequency(kSwbInHz);
                }
                break;
            case 48000:
                if(OutputFrequency() != kFbInHz)
                {
                    SetOutputFrequency(kFbInHz);
                }
                break;
            default:
                assert(false);

                CriticalSectionScoped cs(_crit.get());
                _processCalls--;
                return -1;
            }
        }

        UpdateToMix(mixList, rampOutList, mixedParticipantsMap,
                    remainingParticipantsAllowedToMix);

        GetAdditionalAudio(additionalFramesList);
        UpdateMixedStatus(mixedParticipantsMap);
        _scratchParticipantsToMixAmount = mixedParticipantsMap.Size();
    }

    
    
    
    while(mixedParticipantsMap.Erase(mixedParticipantsMap.First()) == 0) {}

    
    AudioFrame* mixedAudio = NULL;
    if(_audioFramePool->PopMemory(mixedAudio) == -1)
    {
        WEBRTC_TRACE(kTraceMemory, kTraceAudioMixerServer, _id,
                     "failed PopMemory() call");
        assert(false);
        return -1;
    }

    bool timeForMixerCallback = false;
    int retval = 0;
    WebRtc_Word32 audioLevel = 0;
    {
        CriticalSectionScoped cs(_crit.get());

        
        

        
        const int num_mixed_channels = std::max(MaxNumChannels(mixList),
            std::max(MaxNumChannels(additionalFramesList),
                     MaxNumChannels(rampOutList)));

        if (!SetNumLimiterChannels(num_mixed_channels))
            retval = -1;

        mixedAudio->UpdateFrame(-1, _timeStamp, NULL, 0, _outputFrequency,
                                AudioFrame::kNormalSpeech,
                                AudioFrame::kVadPassive, num_mixed_channels);

        _timeStamp += _sampleSize;

        MixFromList(*mixedAudio, mixList);
        MixAnonomouslyFromList(*mixedAudio, additionalFramesList);
        MixAnonomouslyFromList(*mixedAudio, rampOutList);

        if(mixedAudio->samples_per_channel_ == 0)
        {
            
            mixedAudio->samples_per_channel_ = _sampleSize;
            mixedAudio->Mute();
        }
        else
        {
            
            if(!LimitMixedAudio(*mixedAudio))
                retval = -1;
        }

        _mixedAudioLevel.ComputeLevel(mixedAudio->data_,_sampleSize);
        audioLevel = _mixedAudioLevel.GetLevel();

        if(_mixerStatusCb)
        {
            _scratchVadPositiveParticipantsAmount = 0;
            UpdateVADPositiveParticipants(mixList);
            if(_amountOf10MsUntilNextCallback-- == 0)
            {
                _amountOf10MsUntilNextCallback = _amountOf10MsBetweenCallbacks;
                timeForMixerCallback = true;
            }
        }
    }

    {
        CriticalSectionScoped cs(_cbCrit.get());
        if(_mixReceiver != NULL)
        {
            const AudioFrame** dummy = NULL;
            _mixReceiver->NewMixedAudio(
                _id,
                *mixedAudio,
                dummy,
                0);
        }

        if((_mixerStatusCallback != NULL) &&
            timeForMixerCallback)
        {
            _mixerStatusCallback->MixedParticipants(
                _id,
                _scratchMixedParticipants,
                _scratchParticipantsToMixAmount);

            _mixerStatusCallback->VADPositiveParticipants(
                _id,
                _scratchVadPositiveParticipants,
                _scratchVadPositiveParticipantsAmount);
            _mixerStatusCallback->MixedAudioLevel(_id,audioLevel);
        }
    }

    
    _audioFramePool->PushMemory(mixedAudio);
    ClearAudioFrameList(mixList);
    ClearAudioFrameList(rampOutList);
    ClearAudioFrameList(additionalFramesList);
    {
        CriticalSectionScoped cs(_crit.get());
        _processCalls--;
    }
    return retval;
}

WebRtc_Word32 AudioConferenceMixerImpl::RegisterMixedStreamCallback(
    AudioMixerOutputReceiver& mixReceiver)
{
    CriticalSectionScoped cs(_cbCrit.get());
    if(_mixReceiver != NULL)
    {
        return -1;
    }
    _mixReceiver = &mixReceiver;
    return 0;
}

WebRtc_Word32 AudioConferenceMixerImpl::UnRegisterMixedStreamCallback()
{
    CriticalSectionScoped cs(_cbCrit.get());
    if(_mixReceiver == NULL)
    {
        return -1;
    }
    _mixReceiver = NULL;
    return 0;
}

WebRtc_Word32 AudioConferenceMixerImpl::SetOutputFrequency(
    const Frequency frequency)
{
    CriticalSectionScoped cs(_crit.get());
    const int error = _limiter->set_sample_rate_hz(frequency);
    if(error != _limiter->kNoError)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioMixerServer, _id,
                     "Error from AudioProcessing: %d", error);
        return -1;
    }

    _outputFrequency = frequency;
    _sampleSize = (_outputFrequency*kProcessPeriodicityInMs) / 1000;

    return 0;
}

AudioConferenceMixer::Frequency
AudioConferenceMixerImpl::OutputFrequency() const
{
    CriticalSectionScoped cs(_crit.get());
    return _outputFrequency;
}

bool AudioConferenceMixerImpl::SetNumLimiterChannels(int numChannels)
{
    if(_limiter->num_input_channels() != numChannels)
    {
        const int error = _limiter->set_num_channels(numChannels,
                                                     numChannels);
        if(error != _limiter->kNoError)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioMixerServer, _id,
                         "Error from AudioProcessing: %d", error);
            assert(false);
            return false;
        }
    }

    return true;
}

WebRtc_Word32 AudioConferenceMixerImpl::RegisterMixerStatusCallback(
    AudioMixerStatusReceiver& mixerStatusCallback,
    const WebRtc_UWord32 amountOf10MsBetweenCallbacks)
{
    if(amountOf10MsBetweenCallbacks == 0)
    {
        WEBRTC_TRACE(
            kTraceWarning,
            kTraceAudioMixerServer,
            _id,
            "amountOf10MsBetweenCallbacks(%d) needs to be larger than 0");
        return -1;
    }
    {
        CriticalSectionScoped cs(_cbCrit.get());
        if(_mixerStatusCallback != NULL)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioMixerServer, _id,
                         "Mixer status callback already registered");
            return -1;
        }
        _mixerStatusCallback = &mixerStatusCallback;
    }
    {
        CriticalSectionScoped cs(_crit.get());
        _amountOf10MsBetweenCallbacks  = amountOf10MsBetweenCallbacks;
        _amountOf10MsUntilNextCallback = 0;
        _mixerStatusCb                 = true;
    }
    return 0;
}

WebRtc_Word32 AudioConferenceMixerImpl::UnRegisterMixerStatusCallback()
{
    {
        CriticalSectionScoped cs(_crit.get());
        if(!_mixerStatusCb)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioMixerServer, _id,
                         "Mixer status callback not registered");
            return -1;
        }
        _mixerStatusCb = false;
    }
    {
        CriticalSectionScoped cs(_cbCrit.get());
        _mixerStatusCallback = NULL;
    }
    return 0;
}

WebRtc_Word32 AudioConferenceMixerImpl::SetMixabilityStatus(
    MixerParticipant& participant,
    const bool mixable)
{
    if (!mixable)
    {
        
        
        SetAnonymousMixabilityStatus(participant, false);
    }
    WebRtc_UWord32 numMixedParticipants;
    {
        CriticalSectionScoped cs(_cbCrit.get());
        const bool isMixed =
            IsParticipantInList(participant,_participantList);
        
        if(!(mixable ^ isMixed))
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioMixerServer, _id,
                         "Mixable is aready %s",
                         isMixed ? "ON" : "off");
            return -1;
        }
        bool success = false;
        if(mixable)
        {
            success = AddParticipantToList(participant,_participantList);
        }
        else
        {
            success = RemoveParticipantFromList(participant,_participantList);
        }
        if(!success)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioMixerServer, _id,
                         "failed to %s participant",
                         mixable ? "add" : "remove");
            assert(false);
            return -1;
        }

        int numMixedNonAnonymous = _participantList.GetSize();
        if (numMixedNonAnonymous > kMaximumAmountOfMixedParticipants)
        {
            numMixedNonAnonymous = kMaximumAmountOfMixedParticipants;
        }
        numMixedParticipants = numMixedNonAnonymous +
                               _additionalParticipantList.GetSize();
    }
    
    
    
    CriticalSectionScoped cs(_crit.get());
    _numMixedParticipants = numMixedParticipants;
    return 0;
}

WebRtc_Word32 AudioConferenceMixerImpl::MixabilityStatus(
    MixerParticipant& participant,
    bool& mixable)
{
    CriticalSectionScoped cs(_cbCrit.get());
    mixable = IsParticipantInList(participant, _participantList);
    return 0;
}

WebRtc_Word32 AudioConferenceMixerImpl::SetAnonymousMixabilityStatus(
    MixerParticipant& participant, const bool anonymous)
{
    CriticalSectionScoped cs(_cbCrit.get());
    if(IsParticipantInList(participant, _additionalParticipantList))
    {
        if(anonymous)
        {
            return 0;
        }
        if(!RemoveParticipantFromList(participant, _additionalParticipantList))
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioMixerServer, _id,
                         "unable to remove participant from anonymous list");
            assert(false);
            return -1;
        }
        return AddParticipantToList(participant, _participantList) ? 0 : -1;
    }
    if(!anonymous)
    {
        return 0;
    }
    const bool mixable = RemoveParticipantFromList(participant,
                                                   _participantList);
    if(!mixable)
    {
        WEBRTC_TRACE(
            kTraceWarning,
            kTraceAudioMixerServer,
            _id,
            "participant must be registered before turning it into anonymous");
        
        
        return -1;
    }
    return AddParticipantToList(participant, _additionalParticipantList) ?
        0 : -1;
}

WebRtc_Word32 AudioConferenceMixerImpl::AnonymousMixabilityStatus(
    MixerParticipant& participant, bool& mixable)
{
    CriticalSectionScoped cs(_cbCrit.get());
    mixable = IsParticipantInList(participant,
                                  _additionalParticipantList);
    return 0;
}

WebRtc_Word32 AudioConferenceMixerImpl::SetMinimumMixingFrequency(
    Frequency freq)
{
    
    
    if (static_cast<int>(freq) == 12000)
    {
         freq = kWbInHz;
    } else if (static_cast<int>(freq) == 24000) {
        freq = kSwbInHz;
    }

    if((freq == kNbInHz) || (freq == kWbInHz) || (freq == kSwbInHz) ||
       (freq == kLowestPossible))
    {
        _minimumMixingFreq=freq;
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioMixerServer, _id,
                     "SetMinimumMixingFrequency incorrect frequency: %i",freq);
        assert(false);
        return -1;
    }
}



WebRtc_Word32 AudioConferenceMixerImpl::GetLowestMixingFrequency()
{
    const int participantListFrequency =
        GetLowestMixingFrequencyFromList(_participantList);
    const int anonymousListFrequency =
        GetLowestMixingFrequencyFromList(_additionalParticipantList);
    const int highestFreq =
        (participantListFrequency > anonymousListFrequency) ?
            participantListFrequency : anonymousListFrequency;
    
    if(_minimumMixingFreq != kLowestPossible)
    {
        if(_minimumMixingFreq > highestFreq)
        {
            return _minimumMixingFreq;
        }
    }
    return highestFreq;
}

WebRtc_Word32 AudioConferenceMixerImpl::GetLowestMixingFrequencyFromList(
    ListWrapper& mixList)
{
    WebRtc_Word32 highestFreq = 8000;
    ListItem* item = mixList.First();
    while(item)
    {
        MixerParticipant* participant =
            static_cast<MixerParticipant*>(item->GetItem());
        const WebRtc_Word32 neededFrequency = participant->NeededFrequency(_id);
        if(neededFrequency > highestFreq)
        {
            highestFreq = neededFrequency;
        }
        item = mixList.Next(item);
    }
    return highestFreq;
}

void AudioConferenceMixerImpl::UpdateToMix(
    ListWrapper& mixList,
    ListWrapper& rampOutList,
    MapWrapper& mixParticipantList,
    WebRtc_UWord32& maxAudioFrameCounter)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "UpdateToMix(mixList,rampOutList,mixParticipantList,%d)",
                 maxAudioFrameCounter);
    const WebRtc_UWord32 mixListStartSize = mixList.GetSize();
    ListWrapper activeList; 
    
    
    struct ParticipantFramePair
    {
        MixerParticipant* participant;
        AudioFrame* audioFrame;
    };
    ListWrapper passiveWasNotMixedList; 
    ListWrapper passiveWasMixedList;    
    ListItem* item = _participantList.First();
    while(item)
    {
        
        
        bool mustAddToPassiveList = (maxAudioFrameCounter >
                                    (activeList.GetSize() +
                                     passiveWasMixedList.GetSize() +
                                     passiveWasNotMixedList.GetSize()));

        MixerParticipant* participant = static_cast<MixerParticipant*>(
            item->GetItem());
        bool wasMixed = false;
        participant->_mixHistory->WasMixed(wasMixed);
        AudioFrame* audioFrame = NULL;
        if(_audioFramePool->PopMemory(audioFrame) == -1)
        {
            WEBRTC_TRACE(kTraceMemory, kTraceAudioMixerServer, _id,
                         "failed PopMemory() call");
            assert(false);
            return;
        }
        audioFrame->sample_rate_hz_ = _outputFrequency;

        if(participant->GetAudioFrame(_id,*audioFrame) != 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioMixerServer, _id,
                         "failed to GetAudioFrame() from participant");
            _audioFramePool->PushMemory(audioFrame);
            item = _participantList.Next(item);
            continue;
        }
        
        
        
        
        if (audioFrame->vad_activity_ == AudioFrame::kVadUnknown)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioMixerServer, _id,
                         "invalid VAD state from participant");
        }

        if(audioFrame->vad_activity_ == AudioFrame::kVadActive)
        {
            if(!wasMixed)
            {
                RampIn(*audioFrame);
            }

            if(activeList.GetSize() >= maxAudioFrameCounter)
            {
                
                
                ListItem* replaceItem = NULL;
                CalculateEnergy(*audioFrame);
                WebRtc_UWord32 lowestEnergy = audioFrame->energy_;

                ListItem* activeItem = activeList.First();
                while(activeItem)
                {
                    AudioFrame* replaceFrame = static_cast<AudioFrame*>(
                        activeItem->GetItem());
                    CalculateEnergy(*replaceFrame);
                    if(replaceFrame->energy_ < lowestEnergy)
                    {
                        replaceItem = activeItem;
                        lowestEnergy = replaceFrame->energy_;
                    }
                    activeItem = activeList.Next(activeItem);
                }
                if(replaceItem != NULL)
                {
                    AudioFrame* replaceFrame = static_cast<AudioFrame*>(
                        replaceItem->GetItem());

                    bool replaceWasMixed = false;
                    MapItem* replaceParticipant = mixParticipantList.Find(
                        replaceFrame->id_);
                    
                    
                    
                    if(replaceParticipant == NULL)
                    {
                        assert(false);
                    } else {
                        static_cast<MixerParticipant*>(
                            replaceParticipant->GetItem())->_mixHistory->
                            WasMixed(replaceWasMixed);

                        mixParticipantList.Erase(replaceFrame->id_);
                        activeList.Erase(replaceItem);

                        activeList.PushFront(static_cast<void*>(audioFrame));
                        mixParticipantList.Insert(
                            audioFrame->id_,
                            static_cast<void*>(participant));
                        assert(mixParticipantList.Size() <=
                               kMaximumAmountOfMixedParticipants);

                        if(replaceWasMixed)
                        {
                            RampOut(*replaceFrame);
                            rampOutList.PushBack(
                                static_cast<void*>(replaceFrame));
                            assert(rampOutList.GetSize() <=
                                   kMaximumAmountOfMixedParticipants);
                        } else {
                            _audioFramePool->PushMemory(replaceFrame);
                        }
                    }
                } else {
                    if(wasMixed)
                    {
                        RampOut(*audioFrame);
                        rampOutList.PushBack(static_cast<void*>(audioFrame));
                        assert(rampOutList.GetSize() <=
                               kMaximumAmountOfMixedParticipants);
                    } else {
                        _audioFramePool->PushMemory(audioFrame);
                    }
                }
            } else {
                activeList.PushFront(static_cast<void*>(audioFrame));
                mixParticipantList.Insert(audioFrame->id_,
                                          static_cast<void*>(participant));
                assert(mixParticipantList.Size() <=
                       kMaximumAmountOfMixedParticipants);
            }
        } else {
            if(wasMixed)
            {
                ParticipantFramePair* pair = new ParticipantFramePair;
                pair->audioFrame  = audioFrame;
                pair->participant = participant;
                passiveWasMixedList.PushBack(static_cast<void*>(pair));
            } else if(mustAddToPassiveList) {
                RampIn(*audioFrame);
                ParticipantFramePair* pair = new ParticipantFramePair;
                pair->audioFrame  = audioFrame;
                pair->participant = participant;
                passiveWasNotMixedList.PushBack(static_cast<void*>(pair));
            } else {
                _audioFramePool->PushMemory(audioFrame);
            }
        }
        item = _participantList.Next(item);
    }
    assert(activeList.GetSize() <= maxAudioFrameCounter);
    
    
    while(!activeList.Empty())
    {
        ListItem* mixItem = activeList.First();
        mixList.PushBack(mixItem->GetItem());
        activeList.Erase(mixItem);
    }
    
    
    
    while(!passiveWasMixedList.Empty())
    {
        ListItem* mixItem = passiveWasMixedList.First();
        ParticipantFramePair* pair = static_cast<ParticipantFramePair*>(
            mixItem->GetItem());
        if(mixList.GetSize() <  maxAudioFrameCounter + mixListStartSize)
        {
            mixList.PushBack(pair->audioFrame);
            mixParticipantList.Insert(pair->audioFrame->id_,
                                      static_cast<void*>(pair->participant));
            assert(mixParticipantList.Size() <=
                   kMaximumAmountOfMixedParticipants);
        }
        else
        {
            _audioFramePool->PushMemory(pair->audioFrame);
        }
        delete pair;
        passiveWasMixedList.Erase(mixItem);
    }
    
    while(!passiveWasNotMixedList.Empty())
    {
        ListItem* mixItem = passiveWasNotMixedList.First();
        ParticipantFramePair* pair = static_cast<ParticipantFramePair*>(
            mixItem->GetItem());
        if(mixList.GetSize() <  maxAudioFrameCounter + mixListStartSize)
        {
            mixList.PushBack(pair->audioFrame);
            mixParticipantList.Insert(pair->audioFrame->id_,
                                      static_cast<void*>(pair->participant));
            assert(mixParticipantList.Size() <=
                   kMaximumAmountOfMixedParticipants);
        }
        else
        {
            _audioFramePool->PushMemory(pair->audioFrame);
        }
        delete pair;
        passiveWasNotMixedList.Erase(mixItem);
    }
    assert(maxAudioFrameCounter + mixListStartSize >= mixList.GetSize());
    maxAudioFrameCounter += mixListStartSize - mixList.GetSize();
}

void AudioConferenceMixerImpl::GetAdditionalAudio(
    ListWrapper& additionalFramesList)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "GetAdditionalAudio(additionalFramesList)");
    ListItem* item = _additionalParticipantList.First();
    while(item)
    {
        
        
        ListItem* nextItem = _additionalParticipantList.Next(item);

        MixerParticipant* participant = static_cast<MixerParticipant*>(
            item->GetItem());
        AudioFrame* audioFrame = NULL;
        if(_audioFramePool->PopMemory(audioFrame) == -1)
        {
            WEBRTC_TRACE(kTraceMemory, kTraceAudioMixerServer, _id,
                         "failed PopMemory() call");
            assert(false);
            return;
        }
        audioFrame->sample_rate_hz_ = _outputFrequency;
        if(participant->GetAudioFrame(_id, *audioFrame) != 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioMixerServer, _id,
                         "failed to GetAudioFrame() from participant");
            _audioFramePool->PushMemory(audioFrame);
            item = nextItem;
            continue;
        }
        if(audioFrame->samples_per_channel_ == 0)
        {
            
            _audioFramePool->PushMemory(audioFrame);
            item = nextItem;
            continue;
        }
        additionalFramesList.PushBack(static_cast<void*>(audioFrame));
        item = nextItem;
    }
}

void AudioConferenceMixerImpl::UpdateMixedStatus(
    MapWrapper& mixedParticipantsMap)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "UpdateMixedStatus(mixedParticipantsMap)");
    assert(mixedParticipantsMap.Size() <= kMaximumAmountOfMixedParticipants);

    
    
    ListItem* participantItem = _participantList.First();
    while(participantItem != NULL)
    {
        bool isMixed = false;
        MixerParticipant* participant =
            static_cast<MixerParticipant*>(participantItem->GetItem());

        MapItem* mixedItem = mixedParticipantsMap.First();
        while(mixedItem)
        {
            if(participant == mixedItem->GetItem())
            {
                isMixed = true;
                break;
            }
            mixedItem = mixedParticipantsMap.Next(mixedItem);
        }
        participant->_mixHistory->SetIsMixed(isMixed);
        participantItem = _participantList.Next(participantItem);
    }
}

void AudioConferenceMixerImpl::ClearAudioFrameList(ListWrapper& audioFrameList)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "ClearAudioFrameList(audioFrameList)");
    ListItem* item = audioFrameList.First();
    while(item)
    {
        AudioFrame* audioFrame = static_cast<AudioFrame*>(item->GetItem());
        _audioFramePool->PushMemory(audioFrame);
        audioFrameList.Erase(item);
        item = audioFrameList.First();
    }
}

void AudioConferenceMixerImpl::UpdateVADPositiveParticipants(
    ListWrapper& mixList)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "UpdateVADPositiveParticipants(mixList)");

    ListItem* item = mixList.First();
    while(item != NULL)
    {
        AudioFrame* audioFrame = static_cast<AudioFrame*>(item->GetItem());
        CalculateEnergy(*audioFrame);
        if(audioFrame->vad_activity_ == AudioFrame::kVadActive)
        {
            _scratchVadPositiveParticipants[
                _scratchVadPositiveParticipantsAmount].participant =
                audioFrame->id_;
            
            _scratchVadPositiveParticipants[
                _scratchVadPositiveParticipantsAmount].level = 0;
            _scratchVadPositiveParticipantsAmount++;
        }
        item = mixList.Next(item);
    }
}

bool AudioConferenceMixerImpl::IsParticipantInList(
    MixerParticipant& participant,
    ListWrapper& participantList)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "IsParticipantInList(participant,participantList)");
    ListItem* item = participantList.First();
    while(item != NULL)
    {
        MixerParticipant* rhsParticipant =
            static_cast<MixerParticipant*>(item->GetItem());
        if(&participant == rhsParticipant)
        {
            return true;
        }
        item = participantList.Next(item);
    }
    return false;
}

bool AudioConferenceMixerImpl::AddParticipantToList(
    MixerParticipant& participant,
    ListWrapper& participantList)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "AddParticipantToList(participant, participantList)");
    if(participantList.PushBack(static_cast<void*>(&participant)) == -1)
    {
        return false;
    }
    
    participant._mixHistory->ResetMixedStatus();
    return true;
}

bool AudioConferenceMixerImpl::RemoveParticipantFromList(
    MixerParticipant& participant,
    ListWrapper& participantList)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "RemoveParticipantFromList(participant, participantList)");
    ListItem* item = participantList.First();
    while(item)
    {
        if(item->GetItem() == &participant)
        {
            participantList.Erase(item);
            
            participant._mixHistory->ResetMixedStatus();
            return true;
        }
        item = participantList.Next(item);
    }
    return false;
}

WebRtc_Word32 AudioConferenceMixerImpl::MixFromList(
    AudioFrame& mixedAudio,
    const ListWrapper& audioFrameList)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "MixFromList(mixedAudio, audioFrameList)");
    WebRtc_UWord32 position = 0;
    ListItem* item = audioFrameList.First();
    if(item == NULL)
    {
        return 0;
    }

    if(_numMixedParticipants == 1)
    {
        
        AudioFrame* audioFrame = static_cast<AudioFrame*>(item->GetItem());
        mixedAudio = *audioFrame;
        SetParticipantStatistics(&_scratchMixedParticipants[position],
                                 *audioFrame);
        return 0;
    }

    while(item != NULL)
    {
        if(position >= kMaximumAmountOfMixedParticipants)
        {
            WEBRTC_TRACE(
                kTraceMemory,
                kTraceAudioMixerServer,
                _id,
                "Trying to mix more than max amount of mixed participants:%d!",
                kMaximumAmountOfMixedParticipants);
            
            assert(false);
            position = 0;
        }
        AudioFrame* audioFrame = static_cast<AudioFrame*>(item->GetItem());
        MixFrames(&mixedAudio, audioFrame);

        SetParticipantStatistics(&_scratchMixedParticipants[position],
                                 *audioFrame);

        position++;
        item = audioFrameList.Next(item);
    }

    return 0;
}


WebRtc_Word32 AudioConferenceMixerImpl::MixAnonomouslyFromList(
    AudioFrame& mixedAudio,
    const ListWrapper& audioFrameList)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioMixerServer, _id,
                 "MixAnonomouslyFromList(mixedAudio, audioFrameList)");
    ListItem* item = audioFrameList.First();
    if(item == NULL)
        return 0;

    if(_numMixedParticipants == 1)
    {
        
        AudioFrame* audioFrame = static_cast<AudioFrame*>(item->GetItem());
        mixedAudio = *audioFrame;
        return 0;
    }

    while(item != NULL)
    {
        AudioFrame* audioFrame = static_cast<AudioFrame*>(item->GetItem());
        MixFrames(&mixedAudio, audioFrame);
        item = audioFrameList.Next(item);
    }
    return 0;
}

bool AudioConferenceMixerImpl::LimitMixedAudio(AudioFrame& mixedAudio)
{
    if(_numMixedParticipants == 1)
    {
        return true;
    }

    
    const int error = _limiter->ProcessStream(&mixedAudio);

    
    
    
    
    
    
    
    
    
    
    mixedAudio += mixedAudio;

    if(error != _limiter->kNoError)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioMixerServer, _id,
                     "Error from AudioProcessing: %d", error);
        assert(false);
        return false;
    }
    return true;
}
} 
