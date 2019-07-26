





#include "AudioBufferSourceNode.h"
#include "mozilla/dom/AudioBufferSourceNodeBinding.h"
#include "mozilla/dom/AudioParam.h"
#include "nsMathUtils.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "AudioParamTimeline.h"
#include "speex/speex_resampler.h"
#include <limits>

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(AudioBufferSourceNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AudioBufferSourceNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mBuffer)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPlaybackRate)
  if (tmp->Context()) {
    
    
    
    
    tmp->DisconnectFromGraph();
    tmp->Context()->UnregisterAudioBufferSourceNode(tmp);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END_INHERITED(AudioNode)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(AudioBufferSourceNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBuffer)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPlaybackRate)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioBufferSourceNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(AudioBufferSourceNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(AudioBufferSourceNode, AudioNode)







class AudioBufferSourceNodeEngine : public AudioNodeEngine
{
public:
  explicit AudioBufferSourceNodeEngine(AudioNode* aNode,
                                       AudioDestinationNode* aDestination) :
    AudioNodeEngine(aNode),
    mStart(0), mStop(TRACK_TICKS_MAX),
    mResampler(nullptr),
    mOffset(0), mDuration(0),
    mLoopStart(0), mLoopEnd(0),
    mBufferSampleRate(0), mPosition(0), mChannels(0), mPlaybackRate(1.0f),
    mDopplerShift(1.0f),
    mDestination(static_cast<AudioNodeStream*>(aDestination->Stream())),
    mPlaybackRateTimeline(1.0f), mLoop(false)
  {}

  ~AudioBufferSourceNodeEngine()
  {
    if (mResampler) {
      speex_resampler_destroy(mResampler);
    }
  }

  void SetSourceStream(AudioNodeStream* aSource)
  {
    mSource = aSource;
  }

  virtual void SetTimelineParameter(uint32_t aIndex,
                                    const dom::AudioParamTimeline& aValue,
                                    TrackRate aSampleRate) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case AudioBufferSourceNode::PLAYBACKRATE:
      mPlaybackRateTimeline = aValue;
      
      
      
      if (mResampler && mPlaybackRateTimeline.HasSimpleValue() &&
          mPlaybackRateTimeline.GetValue() == 1.0 &&
          mBufferSampleRate == aSampleRate) {
        speex_resampler_destroy(mResampler);
        mResampler = nullptr;
      }
      WebAudioUtils::ConvertAudioParamToTicks(mPlaybackRateTimeline, mSource, mDestination);
      break;
    default:
      NS_ERROR("Bad AudioBufferSourceNodeEngine TimelineParameter");
    }
  }
  virtual void SetStreamTimeParameter(uint32_t aIndex, TrackTicks aParam)
  {
    switch (aIndex) {
    case AudioBufferSourceNode::START: mStart = aParam; break;
    case AudioBufferSourceNode::STOP: mStop = aParam; break;
    default:
      NS_ERROR("Bad AudioBufferSourceNodeEngine StreamTimeParameter");
    }
  }
  virtual void SetDoubleParameter(uint32_t aIndex, double aParam)
  {
    switch (aIndex) {
      case AudioBufferSourceNode::DOPPLERSHIFT:
        mDopplerShift = aParam;
        break;
      default:
        NS_ERROR("Bad AudioBufferSourceNodeEngine double parameter.");
    };
  }
  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aParam)
  {
    switch (aIndex) {
    case AudioBufferSourceNode::SAMPLE_RATE: mBufferSampleRate = aParam; break;
    case AudioBufferSourceNode::OFFSET: mOffset = aParam; break;
    case AudioBufferSourceNode::DURATION: mDuration = aParam; break;
    case AudioBufferSourceNode::LOOP: mLoop = !!aParam; break;
    case AudioBufferSourceNode::LOOPSTART: mLoopStart = aParam; break;
    case AudioBufferSourceNode::LOOPEND: mLoopEnd = aParam; break;
    default:
      NS_ERROR("Bad AudioBufferSourceNodeEngine Int32Parameter");
    }
  }
  virtual void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer)
  {
    mBuffer = aBuffer;
  }

  SpeexResamplerState* Resampler(AudioNodeStream* aStream, uint32_t aChannels)
  {
    if (aChannels != mChannels && mResampler) {
      speex_resampler_destroy(mResampler);
      mResampler = nullptr;
    }

    if (!mResampler) {
      mChannels = aChannels;
      mResampler = speex_resampler_init(mChannels, mBufferSampleRate,
                                        ComputeFinalOutSampleRate(aStream->SampleRate()),
                                        SPEEX_RESAMPLER_QUALITY_DEFAULT,
                                        nullptr);
    }
    return mResampler;
  }

  
  
  void BorrowFromInputBuffer(AudioChunk* aOutput,
                             uint32_t aChannels,
                             uintptr_t aSourceOffset)
  {
    aOutput->mDuration = WEBAUDIO_BLOCK_SIZE;
    aOutput->mBuffer = mBuffer;
    aOutput->mChannelData.SetLength(aChannels);
    for (uint32_t i = 0; i < aChannels; ++i) {
      aOutput->mChannelData[i] = mBuffer->GetData(i) + aSourceOffset;
    }
    aOutput->mVolume = 1.0f;
    aOutput->mBufferFormat = AUDIO_FORMAT_FLOAT32;
  }

  
  
  void CopyFromInputBuffer(AudioChunk* aOutput,
                           uint32_t aChannels,
                           uintptr_t aSourceOffset,
                           uintptr_t aBufferOffset,
                           uint32_t aNumberOfFrames) {
    for (uint32_t i = 0; i < aChannels; ++i) {
      float* baseChannelData = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i]));
      memcpy(baseChannelData + aBufferOffset,
             mBuffer->GetData(i) + aSourceOffset,
             aNumberOfFrames * sizeof(float));
    }
  }

  
  
  
  
  
  void CopyFromInputBufferWithResampling(AudioNodeStream* aStream,
                                         AudioChunk* aOutput,
                                         uint32_t aChannels,
                                         uintptr_t aSourceOffset,
                                         uintptr_t aBufferOffset,
                                         uint32_t aAvailableInInputBuffer,
                                         uint32_t& aFramesRead,
                                         uint32_t& aFramesWritten) {
    double finalPlaybackRate =
      static_cast<double>(mBufferSampleRate) / ComputeFinalOutSampleRate(aStream->SampleRate());
    uint32_t availableInOuputBuffer = WEBAUDIO_BLOCK_SIZE - aBufferOffset;
    uint32_t inputSamples, outputSamples;

    
    if (aAvailableInInputBuffer < availableInOuputBuffer * finalPlaybackRate) {
      outputSamples = ceil(aAvailableInInputBuffer / finalPlaybackRate);
      inputSamples = aAvailableInInputBuffer;
    } else {
      inputSamples = ceil(availableInOuputBuffer * finalPlaybackRate);
      outputSamples = availableInOuputBuffer;
    }

    SpeexResamplerState* resampler = Resampler(aStream, aChannels);

    for (uint32_t i = 0; i < aChannels; ++i) {
      uint32_t inSamples = inputSamples;
      uint32_t outSamples = outputSamples;

      const float* inputData = mBuffer->GetData(i) + aSourceOffset;
      float* outputData =
        static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i])) +
        aBufferOffset;

      WebAudioUtils::SpeexResamplerProcess(resampler, i,
                                           inputData, &inSamples,
                                           outputData, &outSamples);

      aFramesRead = inSamples;
      aFramesWritten = outSamples;
    }
  }

  







  void FillWithZeroes(AudioChunk* aOutput,
                      uint32_t aChannels,
                      uint32_t* aOffsetWithinBlock,
                      TrackTicks* aCurrentPosition,
                      TrackTicks aMaxPos)
  {
    MOZ_ASSERT(*aCurrentPosition < aMaxPos);
    uint32_t numFrames =
      std::min<TrackTicks>(WEBAUDIO_BLOCK_SIZE - *aOffsetWithinBlock,
                           aMaxPos - *aCurrentPosition);
    if (numFrames == WEBAUDIO_BLOCK_SIZE) {
      aOutput->SetNull(numFrames);
    } else {
      if (aOutput->IsNull()) {
        AllocateAudioBlock(aChannels, aOutput);
      }
      WriteZeroesToAudioBlock(aOutput, *aOffsetWithinBlock, numFrames);
    }
    *aOffsetWithinBlock += numFrames;
    *aCurrentPosition += numFrames;
  }

  








  void CopyFromBuffer(AudioNodeStream* aStream,
                      AudioChunk* aOutput,
                      uint32_t aChannels,
                      uint32_t* aOffsetWithinBlock,
                      TrackTicks* aCurrentPosition,
                      uint32_t aBufferOffset,
                      uint32_t aBufferMax)
  {
    MOZ_ASSERT(*aCurrentPosition < mStop);
    uint32_t numFrames =
      std::min<TrackTicks>(std::min(WEBAUDIO_BLOCK_SIZE - *aOffsetWithinBlock,
                                    aBufferMax - aBufferOffset),
                           mStop - *aCurrentPosition);
    if (numFrames == WEBAUDIO_BLOCK_SIZE && !ShouldResample(aStream->SampleRate())) {
      BorrowFromInputBuffer(aOutput, aChannels, aBufferOffset);
      *aOffsetWithinBlock += numFrames;
      *aCurrentPosition += numFrames;
      mPosition += numFrames;
    } else {
      if (aOutput->IsNull()) {
        MOZ_ASSERT(*aOffsetWithinBlock == 0);
        AllocateAudioBlock(aChannels, aOutput);
      }
      if (!ShouldResample(aStream->SampleRate())) {
        CopyFromInputBuffer(aOutput, aChannels, aBufferOffset, *aOffsetWithinBlock, numFrames);
        *aOffsetWithinBlock += numFrames;
        *aCurrentPosition += numFrames;
        mPosition += numFrames;
      } else {
        uint32_t framesRead, framesWritten, availableInInputBuffer;

        availableInInputBuffer = aBufferMax - aBufferOffset;

        CopyFromInputBufferWithResampling(aStream, aOutput, aChannels, aBufferOffset, *aOffsetWithinBlock, availableInInputBuffer, framesRead, framesWritten);
        *aOffsetWithinBlock += framesWritten;
        *aCurrentPosition += framesRead;
        mPosition += framesRead;
      }
    }
  }

  TrackTicks GetPosition(AudioNodeStream* aStream)
  {
    if (aStream->GetCurrentPosition() < mStart) {
      return aStream->GetCurrentPosition();
    }
    return mStart + mPosition;
  }

  uint32_t ComputeFinalOutSampleRate(TrackRate aStreamSampleRate)
  {
    if (mPlaybackRate <= 0 || mPlaybackRate != mPlaybackRate) {
      mPlaybackRate = 1.0f;
    }
    if (mDopplerShift <= 0 || mDopplerShift != mDopplerShift) {
      mDopplerShift = 1.0f;
    }
    return WebAudioUtils::TruncateFloatToInt<uint32_t>(aStreamSampleRate /
                                                       (mPlaybackRate * mDopplerShift));
  }

  bool ShouldResample(TrackRate aStreamSampleRate) const
  {
    return !(mPlaybackRate == 1.0 &&
             mDopplerShift == 1.0 &&
             mBufferSampleRate == aStreamSampleRate);
  }

  void UpdateSampleRateIfNeeded(AudioNodeStream* aStream, uint32_t aChannels)
  {
    if (mPlaybackRateTimeline.HasSimpleValue()) {
      mPlaybackRate = mPlaybackRateTimeline.GetValue();
    } else {
      mPlaybackRate = mPlaybackRateTimeline.GetValueAtTime(aStream->GetCurrentPosition());
    }

    
    
    if (ComputeFinalOutSampleRate(aStream->SampleRate()) == 0) {
      mPlaybackRate = 1.0;
      mDopplerShift = 1.0;
    }

    uint32_t currentOutSampleRate, currentInSampleRate;
    if (ShouldResample(aStream->SampleRate())) {
      SpeexResamplerState* resampler = Resampler(aStream, aChannels);
      speex_resampler_get_rate(resampler, &currentInSampleRate, &currentOutSampleRate);
      uint32_t finalSampleRate = ComputeFinalOutSampleRate(aStream->SampleRate());
      if (currentOutSampleRate != finalSampleRate) {
        speex_resampler_set_rate(resampler, currentInSampleRate, finalSampleRate);
      }
    }
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    if (!mBuffer || !mDuration) {
      return;
    }

    uint32_t channels = mBuffer->GetChannels();
    if (!channels) {
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }

    
    
    
    UpdateSampleRateIfNeeded(aStream, channels);

    uint32_t written = 0;
    TrackTicks currentPosition = GetPosition(aStream);
    while (written < WEBAUDIO_BLOCK_SIZE) {
      if (mStop != TRACK_TICKS_MAX &&
          currentPosition >= mStop) {
        FillWithZeroes(aOutput, channels, &written, &currentPosition, TRACK_TICKS_MAX);
        continue;
      }
      if (currentPosition < mStart) {
        FillWithZeroes(aOutput, channels, &written, &currentPosition, mStart);
        continue;
      }
      TrackTicks t = currentPosition - mStart;
      if (mLoop) {
        if (mOffset + t < mLoopEnd) {
          CopyFromBuffer(aStream, aOutput, channels, &written, &currentPosition, mOffset + t, mLoopEnd);
        } else {
          uint32_t offsetInLoop = (mOffset + t - mLoopEnd) % (mLoopEnd - mLoopStart);
          CopyFromBuffer(aStream, aOutput, channels, &written, &currentPosition, mLoopStart + offsetInLoop, mLoopEnd);
        }
      } else {
        if (t < mDuration) {
          CopyFromBuffer(aStream, aOutput, channels, &written, &currentPosition, mOffset + t, mOffset + mDuration);
        } else {
          FillWithZeroes(aOutput, channels, &written, &currentPosition, TRACK_TICKS_MAX);
        }
      }
    }

    
    
    if (currentPosition >= mStop ||
        (!mLoop && currentPosition - mStart >= mDuration)) {
      *aFinished = true;
    }
  }

  TrackTicks mStart;
  TrackTicks mStop;
  nsRefPtr<ThreadSharedFloatArrayBufferList> mBuffer;
  SpeexResamplerState* mResampler;
  int32_t mOffset;
  int32_t mDuration;
  int32_t mLoopStart;
  int32_t mLoopEnd;
  int32_t mBufferSampleRate;
  uint32_t mPosition;
  uint32_t mChannels;
  float mPlaybackRate;
  float mDopplerShift;
  AudioNodeStream* mDestination;
  AudioNodeStream* mSource;
  AudioParamTimeline mPlaybackRateTimeline;
  bool mLoop;
};

AudioBufferSourceNode::AudioBufferSourceNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mLoopStart(0.0)
  , mLoopEnd(0.0)
  , mOffset(0.0)
  , mDuration(std::numeric_limits<double>::min())
  , mPlaybackRate(new AudioParam(MOZ_THIS_IN_INITIALIZER_LIST(),
                  SendPlaybackRateToStream, 1.0f))
  , mLoop(false)
  , mStartCalled(false)
  , mStopped(false)
{
  AudioBufferSourceNodeEngine* engine = new AudioBufferSourceNodeEngine(this, aContext->Destination());
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::SOURCE_STREAM);
  engine->SetSourceStream(static_cast<AudioNodeStream*>(mStream.get()));
  mStream->AddMainThreadListener(this);
}

AudioBufferSourceNode::~AudioBufferSourceNode()
{
  if (Context()) {
    Context()->UnregisterAudioBufferSourceNode(this);
  }
}

JSObject*
AudioBufferSourceNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return AudioBufferSourceNodeBinding::Wrap(aCx, aScope, this);
}

void
AudioBufferSourceNode::Start(double aWhen, double aOffset,
                             const Optional<double>& aDuration, ErrorResult& aRv)
{
  if (!WebAudioUtils::IsTimeValid(aWhen) ||
      (aDuration.WasPassed() && !WebAudioUtils::IsTimeValid(aDuration.Value()))) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  if (mStartCalled) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mStartCalled = true;

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  if (!ns) {
    
    return;
  }

  if (mBuffer) {
    double duration = aDuration.WasPassed() ?
                      aDuration.Value() :
                      std::numeric_limits<double>::min();
    SendOffsetAndDurationParametersToStream(ns, aOffset, duration);
  } else {
    
    
    
    mOffset = aOffset;
    mDuration = aDuration.WasPassed() ?
                aDuration.Value() :
                std::numeric_limits<double>::min();
  }

  
  if (aWhen > 0.0) {
    ns->SetStreamTimeParameter(START, Context()->DestinationStream(), aWhen);
  }

  MarkActive();
}

void
AudioBufferSourceNode::SendBufferParameterToStream(JSContext* aCx)
{
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  MOZ_ASSERT(ns, "Why don't we have a stream here?");

  if (mBuffer) {
    float rate = mBuffer->SampleRate();
    nsRefPtr<ThreadSharedFloatArrayBufferList> data =
      mBuffer->GetThreadSharedChannelsForRate(aCx);
    ns->SetBuffer(data.forget());
    ns->SetInt32Parameter(SAMPLE_RATE, rate);

    if (mStartCalled) {
      SendOffsetAndDurationParametersToStream(ns, mOffset, mDuration);
    }
  } else {
    ns->SetBuffer(nullptr);
  }
}

void
AudioBufferSourceNode::SendOffsetAndDurationParametersToStream(AudioNodeStream* aStream,
                                                               double aOffset,
                                                               double aDuration)
{
  NS_ASSERTION(mBuffer && mStartCalled,
               "Only call this when we have a buffer and start() has been called");

  float rate = mBuffer->SampleRate();
  int32_t bufferLength = mBuffer->Length();
  int32_t offsetSamples = std::max(0, NS_lround(aOffset * rate));

  if (offsetSamples >= bufferLength) {
    
    
    
    
    if (mStartCalled) {
      aStream->SetBuffer(nullptr);
    }
    return;
  }
  
  if (offsetSamples > 0) {
    aStream->SetInt32Parameter(OFFSET, offsetSamples);
  }

  int32_t playingLength = bufferLength - offsetSamples;
  if (aDuration != std::numeric_limits<double>::min()) {
    playingLength = std::min(NS_lround(aDuration * rate), playingLength);
  }
  aStream->SetInt32Parameter(DURATION, playingLength);
}

void
AudioBufferSourceNode::Stop(double aWhen, ErrorResult& aRv)
{
  if (!WebAudioUtils::IsTimeValid(aWhen)) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  if (!mStartCalled) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (!mBuffer) {
    
    
    MarkInactive();
  }

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  if (!ns || !Context()) {
    
    return;
  }

  ns->SetStreamTimeParameter(STOP, Context()->DestinationStream(),
                             std::max(0.0, aWhen));
}

void
AudioBufferSourceNode::NotifyMainThreadStateChanged()
{
  if (mStream->IsFinished()) {
    class EndedEventDispatcher : public nsRunnable
    {
    public:
      explicit EndedEventDispatcher(AudioBufferSourceNode* aNode)
        : mNode(aNode) {}
      NS_IMETHODIMP Run()
      {
        
        if (!nsContentUtils::IsSafeToRunScript()) {
          nsContentUtils::AddScriptRunner(this);
          return NS_OK;
        }

        mNode->DispatchTrustedEvent(NS_LITERAL_STRING("ended"));
        return NS_OK;
      }
    private:
      nsRefPtr<AudioBufferSourceNode> mNode;
    };
    if (!mStopped) {
      
      NS_DispatchToMainThread(new EndedEventDispatcher(this));
      mStopped = true;
    }

    
    
    MarkInactive();
  }
}

void
AudioBufferSourceNode::SendPlaybackRateToStream(AudioNode* aNode)
{
  AudioBufferSourceNode* This = static_cast<AudioBufferSourceNode*>(aNode);
  SendTimelineParameterToStream(This, PLAYBACKRATE, *This->mPlaybackRate);
}

void
AudioBufferSourceNode::SendDopplerShiftToStream(double aDopplerShift)
{
  SendDoubleParameterToStream(DOPPLERSHIFT, aDopplerShift);
}

void
AudioBufferSourceNode::SendLoopParametersToStream()
{
  
  if (mLoop && mBuffer) {
    float rate = mBuffer->SampleRate();
    double length = (double(mBuffer->Length()) / mBuffer->SampleRate());
    double actualLoopStart, actualLoopEnd;
    if (mLoopStart >= 0.0 && mLoopEnd > 0.0 &&
        mLoopStart < mLoopEnd) {
      MOZ_ASSERT(mLoopStart != 0.0 || mLoopEnd != 0.0);
      actualLoopStart = (mLoopStart > length) ? 0.0 : mLoopStart;
      actualLoopEnd = std::min(mLoopEnd, length);
    } else {
      actualLoopStart = 0.0;
      actualLoopEnd = length;
    }
    int32_t loopStartTicks = NS_lround(actualLoopStart * rate);
    int32_t loopEndTicks = NS_lround(actualLoopEnd * rate);
    if (loopStartTicks < loopEndTicks) {
      SendInt32ParameterToStream(LOOPSTART, loopStartTicks);
      SendInt32ParameterToStream(LOOPEND, loopEndTicks);
      SendInt32ParameterToStream(LOOP, 1);
    } else {
      
      
      SendInt32ParameterToStream(LOOP, 0);
    }
  } else if (!mLoop) {
    SendInt32ParameterToStream(LOOP, 0);
  }
}

}
}
