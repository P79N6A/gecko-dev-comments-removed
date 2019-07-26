




#include "AudioNodeStream.h"

#include "MediaStreamGraphImpl.h"
#include "AudioNodeEngine.h"
#include "ThreeDPoint.h"

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
  StreamTime streamTime = std::max<MediaTime>(0, SecondsToMediaTime(aStreamTime));
  GraphTime graphTime = aRelativeToStream->StreamTimeToGraphTime(streamTime);
  StreamTime thisStreamTime = GraphTimeToStreamTimeOptimistic(graphTime);
  TrackTicks ticks = TimeToTicksRoundUp(IdealAudioRate(), thisStreamTime);
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
      : ControlMessage(aStream), mValue(aValue), mIndex(aIndex) {}
    virtual void Run()
    {
      static_cast<AudioNodeStream*>(mStream)->Engine()->
          SetTimelineParameter(mIndex, mValue);
    }
    AudioParamTimeline mValue;
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
  mMixingMode.mChannelCountMode = aChannelCountMode;
  mMixingMode.mChannelInterpretation = aChannelInterpretation;
}

StreamBuffer::Track*
AudioNodeStream::EnsureTrack()
{
  StreamBuffer::Track* track = mBuffer.FindTrack(AUDIO_NODE_STREAM_TRACK_ID);
  if (!track) {
    nsAutoPtr<MediaSegment> segment(new AudioSegment());
    for (uint32_t j = 0; j < mListeners.Length(); ++j) {
      MediaStreamListener* l = mListeners[j];
      l->NotifyQueuedTrackChanges(Graph(), AUDIO_NODE_STREAM_TRACK_ID, IdealAudioRate(), 0,
                                  MediaStreamListener::TRACK_EVENT_CREATED,
                                  *segment);
    }
    track = &mBuffer.AddTrack(AUDIO_NODE_STREAM_TRACK_ID, IdealAudioRate(), 0, segment.forget());
  }
  return track;
}

bool
AudioNodeStream::AllInputsFinished() const
{
  uint32_t inputCount = mInputs.Length();
  for (uint32_t i = 0; i < inputCount; ++i) {
    if (!mInputs[i]->GetSource()->IsFinishedOnGraphThread()) {
      return false;
    }
  }
  return !!inputCount;
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
    AudioChunk* chunk = &a->mLastChunks[mInputs[i]->OutputNumber()];
    MOZ_ASSERT(chunk);
    if (chunk->IsNull()) {
      continue;
    }

    inputChunks.AppendElement(chunk);
    outputChannelCount =
      GetAudioChannelsSuperset(outputChannelCount, chunk->mChannelData.Length());
  }

  switch (mMixingMode.mChannelCountMode) {
  case ChannelCountMode::Explicit:
    
    
    outputChannelCount = mNumberOfInputChannels;
    break;
  case ChannelCountMode::Clamped_max:
    
    outputChannelCount = std::min(outputChannelCount, mNumberOfInputChannels);
    break;
  case ChannelCountMode::Max:
    
    break;
  }

  uint32_t inputChunkCount = inputChunks.Length();
  if (inputChunkCount == 0) {
    aTmpChunk.SetNull(WEBAUDIO_BLOCK_SIZE);
    return;
  }

  if (inputChunkCount == 1 &&
      inputChunks[0]->mChannelData.Length() == outputChannelCount) {
    aTmpChunk = *inputChunks[0];
    return;
  }

  AllocateAudioBlock(outputChannelCount, &aTmpChunk);
  float silenceChannel[WEBAUDIO_BLOCK_SIZE] = {0.f};
  
  nsAutoTArray<float, GUESS_AUDIO_CHANNELS*WEBAUDIO_BLOCK_SIZE> downmixBuffer;

  for (uint32_t i = 0; i < inputChunkCount; ++i) {
    AudioChunk* chunk = inputChunks[i];
    nsAutoTArray<const void*,GUESS_AUDIO_CHANNELS> channels;
    channels.AppendElements(chunk->mChannelData);
    if (channels.Length() < outputChannelCount) {
      if (mMixingMode.mChannelInterpretation == ChannelInterpretation::Speakers) {
        AudioChannelsUpMix(&channels, outputChannelCount, nullptr);
        NS_ASSERTION(outputChannelCount == channels.Length(),
                     "We called GetAudioChannelsSuperset to avoid this");
      } else {
        
        for (uint32_t j = channels.Length(); j < outputChannelCount; ++j) {
          channels.AppendElement(silenceChannel);
        }
      }
    } else if (channels.Length() > outputChannelCount) {
      if (mMixingMode.mChannelInterpretation == ChannelInterpretation::Speakers) {
        nsAutoTArray<float*,GUESS_AUDIO_CHANNELS> outputChannels;
        outputChannels.SetLength(outputChannelCount);
        downmixBuffer.SetLength(outputChannelCount * WEBAUDIO_BLOCK_SIZE);
        for (uint32_t j = 0; j < outputChannelCount; ++j) {
          outputChannels[j] = &downmixBuffer[j * WEBAUDIO_BLOCK_SIZE];
        }

        AudioChannelsDownMix(channels, outputChannels.Elements(),
                             outputChannelCount, WEBAUDIO_BLOCK_SIZE);

        channels.SetLength(outputChannelCount);
        for (uint32_t j = 0; j < channels.Length(); ++j) {
          channels[j] = outputChannels[j];
        }
      } else {
        
        channels.RemoveElementsAt(outputChannelCount,
                                  channels.Length() - outputChannelCount);
      }
    }

    for (uint32_t c = 0; c < channels.Length(); ++c) {
      const float* inputData = static_cast<const float*>(channels[c]);
      float* outputData = static_cast<float*>(const_cast<void*>(aTmpChunk.mChannelData[c]));
      if (inputData) {
        if (i == 0) {
          AudioBlockCopyChannelWithScale(inputData, chunk->mVolume, outputData);
        } else {
          AudioBlockAddChannelWithScale(inputData, chunk->mVolume, outputData);
        }
      } else {
        if (i == 0) {
          memset(outputData, 0, WEBAUDIO_BLOCK_SIZE*sizeof(float));
        }
      }
    }
  }
}



void
AudioNodeStream::ProduceOutput(GraphTime aFrom, GraphTime aTo)
{
  if (mMarkAsFinishedAfterThisBlock) {
    
    
    
    FinishOutput();
  }

  StreamBuffer::Track* track = EnsureTrack();

  AudioSegment* segment = track->Get<AudioSegment>();

  mLastChunks.SetLength(1);
  mLastChunks[0].SetNull(0);

  if (mInCycle) {
    
    mLastChunks[0].SetNull(WEBAUDIO_BLOCK_SIZE);
  } else {
    
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
                                IdealAudioRate(), segment->GetDuration(), 0,
                                tmpSegment);
  }
}

TrackTicks
AudioNodeStream::GetCurrentPosition()
{
  return EnsureTrack()->Get<AudioSegment>()->GetDuration();
}

void
AudioNodeStream::FinishOutput()
{
  if (IsFinishedOnGraphThread()) {
    return;
  }

  StreamBuffer::Track* track = EnsureTrack();
  track->SetEnded();
  FinishOnGraphThread();

  for (uint32_t j = 0; j < mListeners.Length(); ++j) {
    MediaStreamListener* l = mListeners[j];
    AudioSegment emptySegment;
    l->NotifyQueuedTrackChanges(Graph(), AUDIO_NODE_STREAM_TRACK_ID,
                                IdealAudioRate(),
                                track->GetSegment()->GetDuration(),
                                MediaStreamListener::TRACK_EVENT_ENDED, emptySegment);
  }
}

}
