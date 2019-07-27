




#include "AudioNodeStream.h"

#include "MediaStreamGraphImpl.h"
#include "AudioNodeEngine.h"
#include "ThreeDPoint.h"
#include "AudioChannelFormat.h"
#include "AudioParamTimeline.h"
#include "AudioContext.h"

using namespace mozilla::dom;

namespace mozilla {










AudioNodeStream::AudioNodeStream(AudioNodeEngine* aEngine,
                                 MediaStreamGraph::AudioNodeStreamKind aKind,
                                 TrackRate aSampleRate,
                                 AudioContext::AudioContextId aContextId)
  : ProcessedMediaStream(nullptr),
    mEngine(aEngine),
    mSampleRate(aSampleRate),
    mAudioContextId(aContextId),
    mKind(aKind),
    mNumberOfInputChannels(2),
    mMarkAsFinishedAfterThisBlock(false),
    mAudioParamStream(false),
    mPassThrough(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  mChannelCountMode = ChannelCountMode::Max;
  mChannelInterpretation = ChannelInterpretation::Speakers;
  
  mHasCurrentData = true;
  mLastChunks.SetLength(std::max(uint16_t(1), mEngine->OutputCount()));
  MOZ_COUNT_CTOR(AudioNodeStream);
}

AudioNodeStream::~AudioNodeStream()
{
  MOZ_COUNT_DTOR(AudioNodeStream);
}

size_t
AudioNodeStream::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = 0;

  
  

  amount += ProcessedMediaStream::SizeOfExcludingThis(aMallocSizeOf);
  amount += mLastChunks.SizeOfExcludingThis(aMallocSizeOf);
  for (size_t i = 0; i < mLastChunks.Length(); i++) {
    
    
    amount += mLastChunks[i].SizeOfExcludingThisIfUnshared(aMallocSizeOf);
  }

  return amount;
}

size_t
AudioNodeStream::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

void
AudioNodeStream::SizeOfAudioNodesIncludingThis(MallocSizeOf aMallocSizeOf,
                                               AudioNodeSizes& aUsage) const
{
  
  aUsage.mStream = SizeOfIncludingThis(aMallocSizeOf);

  if (mEngine) {
    
    mEngine->SizeOfIncludingThis(aMallocSizeOf, aUsage);
  }
}

void
AudioNodeStream::SetStreamTimeParameter(uint32_t aIndex, AudioContext* aContext,
                                        double aStreamTime)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex, MediaStream* aRelativeToStream,
            double aStreamTime)
      : ControlMessage(aStream), mStreamTime(aStreamTime),
        mRelativeToStream(aRelativeToStream), mIndex(aIndex)
    {}
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->
          SetStreamTimeParameterImpl(mIndex, mRelativeToStream, mStreamTime);
    }
    double mStreamTime;
    MediaStream* mRelativeToStream;
    uint32_t mIndex;
  };

  GraphImpl()->AppendMessage(new Message(this, aIndex,
      aContext->DestinationStream(),
      aContext->DOMTimeToStreamTime(aStreamTime)));
}

void
AudioNodeStream::SetStreamTimeParameterImpl(uint32_t aIndex, MediaStream* aRelativeToStream,
                                            double aStreamTime)
{
  StreamTime ticks = TicksFromDestinationTime(aRelativeToStream, aStreamTime);
  mEngine->SetStreamTimeParameter(aIndex, ticks);
}

void
AudioNodeStream::SetDoubleParameter(uint32_t aIndex, double aValue)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex, double aValue)
      : ControlMessage(aStream), mValue(aValue), mIndex(aIndex)
    {}
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetDoubleParameter(mIndex, mValue);
    }
    double mValue;
    uint32_t mIndex;
  };

  GraphImpl()->AppendMessage(new Message(this, aIndex, aValue));
}

void
AudioNodeStream::SetInt32Parameter(uint32_t aIndex, int32_t aValue)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex, int32_t aValue)
      : ControlMessage(aStream), mValue(aValue), mIndex(aIndex)
    {}
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetInt32Parameter(mIndex, mValue);
    }
    int32_t mValue;
    uint32_t mIndex;
  };

  GraphImpl()->AppendMessage(new Message(this, aIndex, aValue));
}

void
AudioNodeStream::SetTimelineParameter(uint32_t aIndex,
                                      const AudioParamTimeline& aValue)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex,
            const AudioParamTimeline& aValue)
      : ControlMessage(aStream),
        mValue(aValue),
        mSampleRate(aStream->SampleRate()),
        mIndex(aIndex)
    {}
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetTimelineParameter(mIndex, mValue, mSampleRate);
    }
    AudioParamTimeline mValue;
    TrackRate mSampleRate;
    uint32_t mIndex;
  };
  GraphImpl()->AppendMessage(new Message(this, aIndex, aValue));
}

void
AudioNodeStream::SetThreeDPointParameter(uint32_t aIndex, const ThreeDPoint& aValue)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex, const ThreeDPoint& aValue)
      : ControlMessage(aStream), mValue(aValue), mIndex(aIndex)
    {}
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetThreeDPointParameter(mIndex, mValue);
    }
    ThreeDPoint mValue;
    uint32_t mIndex;
  };

  GraphImpl()->AppendMessage(new Message(this, aIndex, aValue));
}

void
AudioNodeStream::SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList>&& aBuffer)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream,
            already_AddRefed<ThreadSharedFloatArrayBufferList>& aBuffer)
      : ControlMessage(aStream), mBuffer(aBuffer)
    {}
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetBuffer(mBuffer.forget());
    }
    nsRefPtr<ThreadSharedFloatArrayBufferList> mBuffer;
  };

  GraphImpl()->AppendMessage(new Message(this, aBuffer));
}

void
AudioNodeStream::SetRawArrayData(nsTArray<float>& aData)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream,
            nsTArray<float>& aData)
      : ControlMessage(aStream)
    {
      mData.SwapElements(aData);
    }
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->SetRawArrayData(mData);
    }
    nsTArray<float> mData;
  };

  GraphImpl()->AppendMessage(new Message(this, aData));
}

void
AudioNodeStream::SetChannelMixingParameters(uint32_t aNumberOfChannels,
                                            ChannelCountMode aChannelCountMode,
                                            ChannelInterpretation aChannelInterpretation)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream,
            uint32_t aNumberOfChannels,
            ChannelCountMode aChannelCountMode,
            ChannelInterpretation aChannelInterpretation)
      : ControlMessage(aStream),
        mNumberOfChannels(aNumberOfChannels),
        mChannelCountMode(aChannelCountMode),
        mChannelInterpretation(aChannelInterpretation)
    {}
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->
        SetChannelMixingParametersImpl(mNumberOfChannels, mChannelCountMode,
                                       mChannelInterpretation);
    }
    uint32_t mNumberOfChannels;
    ChannelCountMode mChannelCountMode;
    ChannelInterpretation mChannelInterpretation;
  };

  GraphImpl()->AppendMessage(new Message(this, aNumberOfChannels,
                                         aChannelCountMode,
                                         aChannelInterpretation));
}

void
AudioNodeStream::SetPassThrough(bool aPassThrough)
{
  class Message final : public ControlMessage
  {
  public:
    Message(AudioNodeStream* aStream, bool aPassThrough)
      : ControlMessage(aStream), mPassThrough(aPassThrough)
    {}
    virtual void Run() override
    {
      static_cast<AudioNodeStream*>(mStream)->mPassThrough = mPassThrough;
    }
    bool mPassThrough;
  };

  GraphImpl()->AppendMessage(new Message(this, aPassThrough));
}

void
AudioNodeStream::SetChannelMixingParametersImpl(uint32_t aNumberOfChannels,
                                                ChannelCountMode aChannelCountMode,
                                                ChannelInterpretation aChannelInterpretation)
{
  
  
  MOZ_ASSERT(int(aChannelCountMode) < INT16_MAX);
  MOZ_ASSERT(int(aChannelInterpretation) < INT16_MAX);

  mNumberOfInputChannels = aNumberOfChannels;
  mChannelCountMode = aChannelCountMode;
  mChannelInterpretation = aChannelInterpretation;
}

uint32_t
AudioNodeStream::ComputedNumberOfChannels(uint32_t aInputChannelCount)
{
  switch (mChannelCountMode) {
  case ChannelCountMode::Explicit:
    
    
    return mNumberOfInputChannels;
  case ChannelCountMode::Clamped_max:
    
    return std::min(aInputChannelCount, mNumberOfInputChannels);
  default:
  case ChannelCountMode::Max:
    
    return aInputChannelCount;
  }
}

void
AudioNodeStream::ObtainInputBlock(AudioChunk& aTmpChunk, uint32_t aPortIndex)
{
  uint32_t inputCount = mInputs.Length();
  uint32_t outputChannelCount = 1;
  nsAutoTArray<AudioChunk*,250> inputChunks;
  for (uint32_t i = 0; i < inputCount; ++i) {
    if (aPortIndex != mInputs[i]->InputNumber()) {
      
      continue;
    }
    MediaStream* s = mInputs[i]->GetSource();
    AudioNodeStream* a = static_cast<AudioNodeStream*>(s);
    MOZ_ASSERT(a == s->AsAudioNodeStream());
    if (a->IsAudioParamStream()) {
      continue;
    }

    AudioChunk* chunk = &a->mLastChunks[mInputs[i]->OutputNumber()];
    MOZ_ASSERT(chunk);
    if (chunk->IsNull() || chunk->mChannelData.IsEmpty()) {
      continue;
    }

    inputChunks.AppendElement(chunk);
    outputChannelCount =
      GetAudioChannelsSuperset(outputChannelCount, chunk->mChannelData.Length());
  }

  outputChannelCount = ComputedNumberOfChannels(outputChannelCount);

  uint32_t inputChunkCount = inputChunks.Length();
  if (inputChunkCount == 0 ||
      (inputChunkCount == 1 && inputChunks[0]->mChannelData.Length() == 0)) {
    aTmpChunk.SetNull(WEBAUDIO_BLOCK_SIZE);
    return;
  }

  if (inputChunkCount == 1 &&
      inputChunks[0]->mChannelData.Length() == outputChannelCount) {
    aTmpChunk = *inputChunks[0];
    return;
  }

  if (outputChannelCount == 0) {
    aTmpChunk.SetNull(WEBAUDIO_BLOCK_SIZE);
    return;
  }

  AllocateAudioBlock(outputChannelCount, &aTmpChunk);
  
  nsAutoTArray<float, GUESS_AUDIO_CHANNELS*WEBAUDIO_BLOCK_SIZE> downmixBuffer;

  for (uint32_t i = 0; i < inputChunkCount; ++i) {
    AccumulateInputChunk(i, *inputChunks[i], &aTmpChunk, &downmixBuffer);
  }
}

void
AudioNodeStream::AccumulateInputChunk(uint32_t aInputIndex, const AudioChunk& aChunk,
                                      AudioChunk* aBlock,
                                      nsTArray<float>* aDownmixBuffer)
{
  nsAutoTArray<const void*,GUESS_AUDIO_CHANNELS> channels;
  UpMixDownMixChunk(&aChunk, aBlock->mChannelData.Length(), channels, *aDownmixBuffer);

  for (uint32_t c = 0; c < channels.Length(); ++c) {
    const float* inputData = static_cast<const float*>(channels[c]);
    float* outputData = static_cast<float*>(const_cast<void*>(aBlock->mChannelData[c]));
    if (inputData) {
      if (aInputIndex == 0) {
        AudioBlockCopyChannelWithScale(inputData, aChunk.mVolume, outputData);
      } else {
        AudioBlockAddChannelWithScale(inputData, aChunk.mVolume, outputData);
      }
    } else {
      if (aInputIndex == 0) {
        PodZero(outputData, WEBAUDIO_BLOCK_SIZE);
      }
    }
  }
}

void
AudioNodeStream::UpMixDownMixChunk(const AudioChunk* aChunk,
                                   uint32_t aOutputChannelCount,
                                   nsTArray<const void*>& aOutputChannels,
                                   nsTArray<float>& aDownmixBuffer)
{
  static const float silenceChannel[WEBAUDIO_BLOCK_SIZE] = {0.f};

  aOutputChannels.AppendElements(aChunk->mChannelData);
  if (aOutputChannels.Length() < aOutputChannelCount) {
    if (mChannelInterpretation == ChannelInterpretation::Speakers) {
      AudioChannelsUpMix(&aOutputChannels, aOutputChannelCount, nullptr);
      NS_ASSERTION(aOutputChannelCount == aOutputChannels.Length(),
                   "We called GetAudioChannelsSuperset to avoid this");
    } else {
      
      for (uint32_t j = aOutputChannels.Length(); j < aOutputChannelCount; ++j) {
        aOutputChannels.AppendElement(silenceChannel);
      }
    }
  } else if (aOutputChannels.Length() > aOutputChannelCount) {
    if (mChannelInterpretation == ChannelInterpretation::Speakers) {
      nsAutoTArray<float*,GUESS_AUDIO_CHANNELS> outputChannels;
      outputChannels.SetLength(aOutputChannelCount);
      aDownmixBuffer.SetLength(aOutputChannelCount * WEBAUDIO_BLOCK_SIZE);
      for (uint32_t j = 0; j < aOutputChannelCount; ++j) {
        outputChannels[j] = &aDownmixBuffer[j * WEBAUDIO_BLOCK_SIZE];
      }

      AudioChannelsDownMix(aOutputChannels, outputChannels.Elements(),
                           aOutputChannelCount, WEBAUDIO_BLOCK_SIZE);

      aOutputChannels.SetLength(aOutputChannelCount);
      for (uint32_t j = 0; j < aOutputChannels.Length(); ++j) {
        aOutputChannels[j] = outputChannels[j];
      }
    } else {
      
      aOutputChannels.RemoveElementsAt(aOutputChannelCount,
        aOutputChannels.Length() - aOutputChannelCount);
    }
  }
}



void
AudioNodeStream::ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags)
{
  if (!mFinished) {
    EnsureTrack(AUDIO_TRACK);
  }
  
  mBuffer.AdvanceKnownTracksTime(STREAM_TIME_MAX);

  uint16_t outputCount = mLastChunks.Length();
  MOZ_ASSERT(outputCount == std::max(uint16_t(1), mEngine->OutputCount()));

  
  
  
  bool blocked = mFinished || mBlocked.GetAt(aFrom);
  
  if (blocked || InMutedCycle()) {
    for (uint16_t i = 0; i < outputCount; ++i) {
      mLastChunks[i].SetNull(WEBAUDIO_BLOCK_SIZE);
    }
  } else {
    
    uint16_t maxInputs = std::max(uint16_t(1), mEngine->InputCount());
    OutputChunks inputChunks;
    inputChunks.SetLength(maxInputs);
    for (uint16_t i = 0; i < maxInputs; ++i) {
      ObtainInputBlock(inputChunks[i], i);
    }
    bool finished = false;
    if (mPassThrough) {
      MOZ_ASSERT(outputCount == 1, "For now, we only support nodes that have one output port");
      mLastChunks[0] = inputChunks[0];
    } else {
      if (maxInputs <= 1 && outputCount <= 1) {
        mEngine->ProcessBlock(this, inputChunks[0], &mLastChunks[0], &finished);
      } else {
        mEngine->ProcessBlocksOnPorts(this, inputChunks, mLastChunks, &finished);
      }
    }
    for (uint16_t i = 0; i < outputCount; ++i) {
      NS_ASSERTION(mLastChunks[i].GetDuration() == WEBAUDIO_BLOCK_SIZE,
                   "Invalid WebAudio chunk size");
    }
    if (finished) {
      mMarkAsFinishedAfterThisBlock = true;
    }

    if (mDisabledTrackIDs.Contains(static_cast<TrackID>(AUDIO_TRACK))) {
      for (uint32_t i = 0; i < outputCount; ++i) {
        mLastChunks[i].SetNull(WEBAUDIO_BLOCK_SIZE);
      }
    }
  }

  if (!blocked) {
    
    AdvanceOutputSegment();
    if (mMarkAsFinishedAfterThisBlock && (aFlags & ALLOW_FINISH)) {
      
      
      
      FinishOutput();
    }
  }
}

void
AudioNodeStream::ProduceOutputBeforeInput(GraphTime aFrom)
{
  MOZ_ASSERT(mEngine->AsDelayNodeEngine());
  MOZ_ASSERT(mEngine->OutputCount() == 1,
             "DelayNodeEngine output count should be 1");
  MOZ_ASSERT(!InMutedCycle(), "DelayNodes should break cycles");
  MOZ_ASSERT(mLastChunks.Length() == 1);

  
  
  
  bool blocked = mFinished || mBlocked.GetAt(aFrom);
  
  if (blocked) {
    mLastChunks[0].SetNull(WEBAUDIO_BLOCK_SIZE);
  } else {
    mEngine->ProduceBlockBeforeInput(&mLastChunks[0]);
    NS_ASSERTION(mLastChunks[0].GetDuration() == WEBAUDIO_BLOCK_SIZE,
                 "Invalid WebAudio chunk size");
    if (mDisabledTrackIDs.Contains(static_cast<TrackID>(AUDIO_TRACK))) {
      mLastChunks[0].SetNull(WEBAUDIO_BLOCK_SIZE);
    }
  }
}

void
AudioNodeStream::AdvanceOutputSegment()
{
  StreamBuffer::Track* track = EnsureTrack(AUDIO_TRACK);
  AudioSegment* segment = track->Get<AudioSegment>();

  if (mKind == MediaStreamGraph::EXTERNAL_STREAM) {
    segment->AppendAndConsumeChunk(&mLastChunks[0]);
  } else {
    segment->AppendNullData(mLastChunks[0].GetDuration());
  }

  for (uint32_t j = 0; j < mListeners.Length(); ++j) {
    MediaStreamListener* l = mListeners[j];
    AudioChunk copyChunk = mLastChunks[0];
    AudioSegment tmpSegment;
    tmpSegment.AppendAndConsumeChunk(&copyChunk);
    l->NotifyQueuedTrackChanges(Graph(), AUDIO_TRACK,
                                segment->GetDuration(), 0, tmpSegment);
  }
}

StreamTime
AudioNodeStream::GetCurrentPosition()
{
  NS_ASSERTION(!mFinished, "Don't create another track after finishing");
  return EnsureTrack(AUDIO_TRACK)->Get<AudioSegment>()->GetDuration();
}

void
AudioNodeStream::FinishOutput()
{
  if (IsFinishedOnGraphThread()) {
    return;
  }

  StreamBuffer::Track* track = EnsureTrack(AUDIO_TRACK);
  track->SetEnded();
  FinishOnGraphThread();

  for (uint32_t j = 0; j < mListeners.Length(); ++j) {
    MediaStreamListener* l = mListeners[j];
    AudioSegment emptySegment;
    l->NotifyQueuedTrackChanges(Graph(), AUDIO_TRACK,
                                track->GetSegment()->GetDuration(),
                                MediaStreamListener::TRACK_EVENT_ENDED, emptySegment);
  }
}

double
AudioNodeStream::FractionalTicksFromDestinationTime(AudioNodeStream* aDestination,
                                                    double aSeconds)
{
  MOZ_ASSERT(aDestination->SampleRate() == SampleRate());
  MOZ_ASSERT(SampleRate() == GraphRate());

  double destinationSeconds = std::max(0.0, aSeconds);
  double destinationFractionalTicks = destinationSeconds * SampleRate();
  MOZ_ASSERT(destinationFractionalTicks < STREAM_TIME_MAX);
  StreamTime destinationStreamTime = destinationFractionalTicks; 
  
  double offset = destinationFractionalTicks - destinationStreamTime;

  GraphTime graphTime =
    aDestination->StreamTimeToGraphTime(destinationStreamTime);
  StreamTime thisStreamTime = GraphTimeToStreamTimeOptimistic(graphTime);
  double thisFractionalTicks = thisStreamTime + offset;
  MOZ_ASSERT(thisFractionalTicks >= 0.0);
  return thisFractionalTicks;
}

StreamTime
AudioNodeStream::TicksFromDestinationTime(MediaStream* aDestination,
                                          double aSeconds)
{
  AudioNodeStream* destination = aDestination->AsAudioNodeStream();
  MOZ_ASSERT(destination);

  double thisSeconds =
    FractionalTicksFromDestinationTime(destination, aSeconds);
  
  StreamTime ticks = thisSeconds + 0.5;
  return ticks;
}

double
AudioNodeStream::DestinationTimeFromTicks(AudioNodeStream* aDestination,
                                          StreamTime aPosition)
{
  MOZ_ASSERT(SampleRate() == aDestination->SampleRate());
  GraphTime graphTime = StreamTimeToGraphTime(aPosition);
  StreamTime destinationTime = aDestination->GraphTimeToStreamTimeOptimistic(graphTime);
  return StreamTimeToSeconds(destinationTime);
}

}
