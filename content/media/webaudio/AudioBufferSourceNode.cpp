





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
    mResampler(nullptr), mRemainingResamplerTail(0),
    mBufferEnd(0),
    mLoopStart(0), mLoopEnd(0),
    mBufferSampleRate(0), mBufferPosition(0), mChannels(0), mPlaybackRate(1.0f),
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
    case AudioBufferSourceNode::BUFFERSTART:
      if (mBufferPosition == 0) {
        mBufferPosition = aParam;
      }
      break;
    case AudioBufferSourceNode::BUFFEREND: mBufferEnd = aParam; break;
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
      speex_resampler_skip_zeros(mResampler);
    }
    return mResampler;
  }

  
  
  void BorrowFromInputBuffer(AudioChunk* aOutput,
                             uint32_t aChannels)
  {
    aOutput->mDuration = WEBAUDIO_BLOCK_SIZE;
    aOutput->mBuffer = mBuffer;
    aOutput->mChannelData.SetLength(aChannels);
    for (uint32_t i = 0; i < aChannels; ++i) {
      aOutput->mChannelData[i] = mBuffer->GetData(i) + mBufferPosition;
    }
    aOutput->mVolume = 1.0f;
    aOutput->mBufferFormat = AUDIO_FORMAT_FLOAT32;
  }

  
  
  void CopyFromInputBuffer(AudioChunk* aOutput,
                           uint32_t aChannels,
                           uintptr_t aOffsetWithinBlock,
                           uint32_t aNumberOfFrames) {
    for (uint32_t i = 0; i < aChannels; ++i) {
      float* baseChannelData = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i]));
      memcpy(baseChannelData + aOffsetWithinBlock,
             mBuffer->GetData(i) + mBufferPosition,
             aNumberOfFrames * sizeof(float));
    }
  }

  
  
  
  
  
  void CopyFromInputBufferWithResampling(AudioNodeStream* aStream,
                                         AudioChunk* aOutput,
                                         uint32_t aChannels,
                                         uint32_t aOffsetWithinBlock,
                                         uint32_t& aFramesWritten,
                                         int32_t aBufferMax) {
    
    uint32_t availableInOutputBuffer = WEBAUDIO_BLOCK_SIZE - aOffsetWithinBlock;
    SpeexResamplerState* resampler = Resampler(aStream, aChannels);
    MOZ_ASSERT(aChannels > 0);

    if (mBufferPosition < aBufferMax) {
      uint32_t availableInInputBuffer = aBufferMax - mBufferPosition;
      
      
      
      
      uint32_t num, den;
      speex_resampler_get_ratio(resampler, &num, &den);
      uint32_t inputLimit = std::min(availableInInputBuffer,
                                     availableInOutputBuffer * num / den + 10);
      for (uint32_t i = 0; true; ) {
        uint32_t inSamples = inputLimit;
        const float* inputData = mBuffer->GetData(i) + mBufferPosition;

        uint32_t outSamples = availableInOutputBuffer;
        float* outputData =
          static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i])) +
          aOffsetWithinBlock;

        WebAudioUtils::SpeexResamplerProcess(resampler, i,
                                             inputData, &inSamples,
                                             outputData, &outSamples);
        if (++i == aChannels) {
          mBufferPosition += inSamples;
          MOZ_ASSERT(mBufferPosition <= mBufferEnd || mLoop);
          aFramesWritten = outSamples;
          if (inSamples == availableInInputBuffer && !mLoop) {
            
            
            
            
            
            
            
            
            
            
            mRemainingResamplerTail =
              speex_resampler_get_input_latency(resampler) + 1;
          }
          return;
        }
      }
    } else {
      for (uint32_t i = 0; true; ) {
        uint32_t inSamples = mRemainingResamplerTail;
        uint32_t outSamples = availableInOutputBuffer;
        float* outputData =
          static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i])) +
          aOffsetWithinBlock;

        
        
        WebAudioUtils::SpeexResamplerProcess(resampler, i,
                         static_cast<AudioDataValue*>(nullptr), &inSamples,
                         outputData, &outSamples);
        if (++i == aChannels) {
          mRemainingResamplerTail -= inSamples;
          MOZ_ASSERT(mRemainingResamplerTail >= 0);
          aFramesWritten = outSamples;
          break;
        }
      }
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
      if (*aOffsetWithinBlock == 0) {
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
                      int32_t aBufferMax)
  {
    MOZ_ASSERT(*aCurrentPosition < mStop);
    uint32_t numFrames =
      std::min(std::min<TrackTicks>(WEBAUDIO_BLOCK_SIZE - *aOffsetWithinBlock,
                                    aBufferMax - mBufferPosition),
               mStop - *aCurrentPosition);
    if (numFrames == WEBAUDIO_BLOCK_SIZE && !ShouldResample(aStream->SampleRate())) {
      MOZ_ASSERT(mBufferPosition < aBufferMax);
      BorrowFromInputBuffer(aOutput, aChannels);
      *aOffsetWithinBlock += numFrames;
      *aCurrentPosition += numFrames;
      mBufferPosition += numFrames;
    } else {
      if (*aOffsetWithinBlock == 0) {
        AllocateAudioBlock(aChannels, aOutput);
      }
      if (!ShouldResample(aStream->SampleRate())) {
        MOZ_ASSERT(mBufferPosition < aBufferMax);
        CopyFromInputBuffer(aOutput, aChannels, *aOffsetWithinBlock, numFrames);
        *aOffsetWithinBlock += numFrames;
        *aCurrentPosition += numFrames;
        mBufferPosition += numFrames;
      } else {
        uint32_t framesWritten;
        CopyFromInputBufferWithResampling(aStream, aOutput, aChannels, *aOffsetWithinBlock, framesWritten, aBufferMax);
        *aOffsetWithinBlock += framesWritten;
        *aCurrentPosition += framesWritten;
      }
    }
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
    
    
    
    
    
    return mResampler ||
      (mPlaybackRate * mDopplerShift * mBufferSampleRate != aStreamSampleRate);
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

    if (mResampler) {
      SpeexResamplerState* resampler = Resampler(aStream, aChannels);
      uint32_t currentOutSampleRate, currentInSampleRate;
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
    if (!mBuffer || !mBufferEnd) {
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }

    uint32_t channels = mBuffer->GetChannels();
    if (!channels) {
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }

    
    
    
    UpdateSampleRateIfNeeded(aStream, channels);

    uint32_t written = 0;
    TrackTicks streamPosition = aStream->GetCurrentPosition();
    while (written < WEBAUDIO_BLOCK_SIZE) {
      if (mStop != TRACK_TICKS_MAX &&
          streamPosition >= mStop) {
        FillWithZeroes(aOutput, channels, &written, &streamPosition, TRACK_TICKS_MAX);
        continue;
      }
      if (streamPosition < mStart) {
        FillWithZeroes(aOutput, channels, &written, &streamPosition, mStart);
        continue;
      }
      if (mLoop) {
        
        
        
        if (mBufferPosition >= mLoopEnd) {
          mBufferPosition = mLoopStart;
        }
        CopyFromBuffer(aStream, aOutput, channels, &written, &streamPosition, mLoopEnd);
      } else {
        if (mBufferPosition < mBufferEnd || mRemainingResamplerTail) {
          CopyFromBuffer(aStream, aOutput, channels, &written, &streamPosition, mBufferEnd);
        } else {
          FillWithZeroes(aOutput, channels, &written, &streamPosition, TRACK_TICKS_MAX);
        }
      }
    }

    
    
    if (streamPosition >= mStop ||
        (!mLoop && mBufferPosition >= mBufferEnd && !mRemainingResamplerTail)) {
      *aFinished = true;
    }
  }

  TrackTicks mStart;
  TrackTicks mStop;
  nsRefPtr<ThreadSharedFloatArrayBufferList> mBuffer;
  SpeexResamplerState* mResampler;
  
  
  int mRemainingResamplerTail;
  int32_t mBufferEnd;
  int32_t mLoopStart;
  int32_t mLoopEnd;
  int32_t mBufferSampleRate;
  int32_t mBufferPosition;
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

  
  mOffset = aOffset;
  mDuration = aDuration.WasPassed() ? aDuration.Value()
                                    : std::numeric_limits<double>::min();
  
  
  if (mBuffer) {
    SendOffsetAndDurationParametersToStream(ns);
  }

  
  if (aWhen > 0.0) {
    ns->SetStreamTimeParameter(START, Context(), aWhen);
  }
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
      SendOffsetAndDurationParametersToStream(ns);
    }
  } else {
    ns->SetBuffer(nullptr);

    MarkInactive();
  }
}

void
AudioBufferSourceNode::SendOffsetAndDurationParametersToStream(AudioNodeStream* aStream)
{
  NS_ASSERTION(mBuffer && mStartCalled,
               "Only call this when we have a buffer and start() has been called");

  float rate = mBuffer->SampleRate();
  int32_t bufferEnd = mBuffer->Length();
  int32_t offsetSamples = std::max(0, NS_lround(mOffset * rate));

  
  if (offsetSamples > 0) {
    aStream->SetInt32Parameter(BUFFERSTART, offsetSamples);
  }

  if (mDuration != std::numeric_limits<double>::min()) {
    bufferEnd = std::min(bufferEnd,
                         offsetSamples + NS_lround(mDuration * rate));
  }
  aStream->SetInt32Parameter(BUFFEREND, bufferEnd);

  MarkActive();
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

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  if (!ns || !Context()) {
    
    return;
  }

  ns->SetStreamTimeParameter(STOP, Context(), std::max(0.0, aWhen));
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
