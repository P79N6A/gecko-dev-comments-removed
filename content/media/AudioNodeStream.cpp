




#include "AudioNodeStream.h"

#include "MediaStreamGraphImpl.h"
#include "AudioNodeEngine.h"
#include "ThreeDPoint.h"
#include "AudioChannelFormat.h"
#include "AudioParamTimeline.h"

using namespace mozilla::dom;

namespace mozilla {









static const int AUDIO_NODE_STREAM_TRACK_ID = 1;

AudioNodeStream::~AudioNodeStream()
{
  MOZ_COUNT_DTOR(AudioNodeStream);
}

void
AudioNodeStream::SetStreamTimeParameter(uint32_t aIndex, MediaStream* aRelativeToStream,
                                        double aStreamTime)
{
  class Message : public ControlMessage {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex, MediaStream* aRelativeToStream,
            double aStreamTime)
      : ControlMessage(aStream), mStreamTime(aStreamTime),
        mRelativeToStream(aRelativeToStream), mIndex(aIndex) {}
    virtual void Run()
    {
      static_cast<AudioNodeStream*>(mStream)->
          SetStreamTimeParameterImpl(mIndex, mRelativeToStream, mStreamTime);
    }
    double mStreamTime;
    MediaStream* mRelativeToStream;
    uint32_t mIndex;
  };

  MOZ_ASSERT(this);
  GraphImpl()->AppendMessage(new Message(this, aIndex, aRelativeToStream, aStreamTime));
}

void
AudioNodeStream::SetStreamTimeParameterImpl(uint32_t aIndex, MediaStream* aRelativeToStream,
                                            double aStreamTime)
{
  TrackTicks ticks =
      WebAudioUtils::ConvertDestinationStreamTimeToSourceStreamTime(
          aStreamTime, this, aRelativeToStream);
  mEngine->SetStreamTimeParameter(aIndex, ticks);
}

void
AudioNodeStream::SetDoubleParameter(uint32_t aIndex, double aValue)
{
  class Message : public ControlMessage {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex, double aValue)
      : ControlMessage(aStream), mValue(aValue), mIndex(aIndex) {}
    virtual void Run()
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetDoubleParameter(mIndex, mValue);
    }
    double mValue;
    uint32_t mIndex;
  };

  MOZ_ASSERT(this);
  GraphImpl()->AppendMessage(new Message(this, aIndex, aValue));
}

void
AudioNodeStream::SetInt32Parameter(uint32_t aIndex, int32_t aValue)
{
  class Message : public ControlMessage {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex, int32_t aValue)
      : ControlMessage(aStream), mValue(aValue), mIndex(aIndex) {}
    virtual void Run()
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetInt32Parameter(mIndex, mValue);
    }
    int32_t mValue;
    uint32_t mIndex;
  };

  MOZ_ASSERT(this);
  GraphImpl()->AppendMessage(new Message(this, aIndex, aValue));
}

void
AudioNodeStream::SetTimelineParameter(uint32_t aIndex,
                                      const AudioParamTimeline& aValue)
{
  class Message : public ControlMessage {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex,
            const AudioParamTimeline& aValue)
      : ControlMessage(aStream),
        mValue(aValue),
        mSampleRate(aStream->SampleRate()),
        mIndex(aIndex) {}
    virtual void Run()
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
  class Message : public ControlMessage {
  public:
    Message(AudioNodeStream* aStream, uint32_t aIndex, const ThreeDPoint& aValue)
      : ControlMessage(aStream), mValue(aValue), mIndex(aIndex) {}
    virtual void Run()
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetThreeDPointParameter(mIndex, mValue);
    }
    ThreeDPoint mValue;
    uint32_t mIndex;
  };

  MOZ_ASSERT(this);
  GraphImpl()->AppendMessage(new Message(this, aIndex, aValue));
}

void
AudioNodeStream::SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer)
{
  class Message : public ControlMessage {
  public:
    Message(AudioNodeStream* aStream,
            already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer)
      : ControlMessage(aStream), mBuffer(aBuffer) {}
    virtual void Run()
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetBuffer(mBuffer.forget());
    }
    nsRefPtr<ThreadSharedFloatArrayBufferList> mBuffer;
  };

  MOZ_ASSERT(this);
  GraphImpl()->AppendMessage(new Message(this, aBuffer));
}

void
AudioNodeStream::SetRawArrayData(nsTArray<float>& aData)
{
  class Message : public ControlMessage {
  public:
    Message(AudioNodeStream* aStream,
            nsTArray<float>& aData)
      : ControlMessage(aStream)
    {
      mData.SwapElements(aData);
    }
    virtual void Run()
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->SetRawArrayData(mData);
    }
    nsTArray<float> mData;
  };

  MOZ_ASSERT(this);
  GraphImpl()->AppendMessage(new Message(this, aData));
}

void
AudioNodeStream::SetChannelMixingParameters(uint32_t aNumberOfChannels,
                                            ChannelCountMode aChannelCountMode,
                                            ChannelInterpretation aChannelInterpretation)
{
  class Message : public ControlMessage {
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
    virtual void Run()
    {
      static_cast<AudioNodeStream*>(mStream)->
        SetChannelMixingParametersImpl(mNumberOfChannels, mChannelCountMode,
                                       mChannelInterpretation);
    }
    uint32_t mNumberOfChannels;
    ChannelCountMode mChannelCountMode;
    ChannelInterpretation mChannelInterpretation;
  };

  MOZ_ASSERT(this);
  GraphImpl()->AppendMessage(new Message(this, aNumberOfChannels,
                                         aChannelCountMode,
                                         aChannelInterpretation));
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
AudioNodeStream::ComputeFinalOuputChannelCount(uint32_t aInputChannelCount)
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
    if (a->IsFinishedOnGraphThread() ||
        a->IsAudioParamStream()) {
      continue;
    }

    
    
    
    
    
    
    
    
    
    if (a->mLastChunks.IsEmpty()) {
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

  outputChannelCount = ComputeFinalOuputChannelCount(outputChannelCount);

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
AudioNodeStream::ProduceOutput(GraphTime aFrom, GraphTime aTo)
{
  if (mMarkAsFinishedAfterThisBlock) {
    
    
    
    FinishOutput();
  }

  EnsureTrack(AUDIO_NODE_STREAM_TRACK_ID, mSampleRate);

  uint16_t outputCount = std::max(uint16_t(1), mEngine->OutputCount());
  mLastChunks.SetLength(outputCount);

  if (mMuted) {
    for (uint16_t i = 0; i < outputCount; ++i) {
      mLastChunks[i].SetNull(WEBAUDIO_BLOCK_SIZE);
    }
  } else {
    for (uint16_t i = 0; i < outputCount; ++i) {
      mLastChunks[i].SetNull(0);
    }

    
    uint16_t maxInputs = std::max(uint16_t(1), mEngine->InputCount());
    OutputChunks inputChunks;
    inputChunks.SetLength(maxInputs);
    for (uint16_t i = 0; i < maxInputs; ++i) {
      ObtainInputBlock(inputChunks[i], i);
    }
    bool finished = false;
    if (maxInputs <= 1 && mEngine->OutputCount() <= 1) {
      mEngine->ProduceAudioBlock(this, inputChunks[0], &mLastChunks[0], &finished);
    } else {
      mEngine->ProduceAudioBlocksOnPorts(this, inputChunks, mLastChunks, &finished);
    }
    if (finished) {
      mMarkAsFinishedAfterThisBlock = true;
    }
  }

  if (mDisabledTrackIDs.Contains(AUDIO_NODE_STREAM_TRACK_ID)) {
    for (uint32_t i = 0; i < mLastChunks.Length(); ++i) {
      mLastChunks[i].SetNull(WEBAUDIO_BLOCK_SIZE);
    }
  }

  AdvanceOutputSegment();
}

void
AudioNodeStream::AdvanceOutputSegment()
{
  StreamBuffer::Track* track = EnsureTrack(AUDIO_NODE_STREAM_TRACK_ID, mSampleRate);
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
    l->NotifyQueuedTrackChanges(Graph(), AUDIO_NODE_STREAM_TRACK_ID,
                                mSampleRate, segment->GetDuration(), 0,
                                tmpSegment);
  }
}

TrackTicks
AudioNodeStream::GetCurrentPosition()
{
  return EnsureTrack(AUDIO_NODE_STREAM_TRACK_ID, mSampleRate)->Get<AudioSegment>()->GetDuration();
}

void
AudioNodeStream::FinishOutput()
{
  if (IsFinishedOnGraphThread()) {
    return;
  }

  StreamBuffer::Track* track = EnsureTrack(AUDIO_NODE_STREAM_TRACK_ID, mSampleRate);
  track->SetEnded();
  FinishOnGraphThread();

  for (uint32_t j = 0; j < mListeners.Length(); ++j) {
    MediaStreamListener* l = mListeners[j];
    AudioSegment emptySegment;
    l->NotifyQueuedTrackChanges(Graph(), AUDIO_NODE_STREAM_TRACK_ID,
                                mSampleRate,
                                track->GetSegment()->GetDuration(),
                                MediaStreamListener::TRACK_EVENT_ENDED, emptySegment);
  }
}

}
