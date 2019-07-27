





#include "AudioBufferSourceNode.h"
#include "mozilla/dom/AudioBufferSourceNodeBinding.h"
#include "mozilla/dom/AudioParam.h"
#include "mozilla/FloatingPoint.h"
#include "nsMathUtils.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "AudioParamTimeline.h"
#include <limits>
#include <algorithm>

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(AudioBufferSourceNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AudioBufferSourceNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mBuffer)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPlaybackRate)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDetune)
  if (tmp->Context()) {
    
    
    
    
    tmp->DisconnectFromGraph();
    tmp->Context()->UnregisterAudioBufferSourceNode(tmp);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END_INHERITED(AudioNode)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(AudioBufferSourceNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBuffer)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPlaybackRate)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDetune)
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
    mStart(0.0), mBeginProcessing(0),
    mStop(STREAM_TIME_MAX),
    mResampler(nullptr), mRemainingResamplerTail(0),
    mBufferEnd(0),
    mLoopStart(0), mLoopEnd(0),
    mBufferSampleRate(0), mBufferPosition(0), mChannels(0),
    mDopplerShift(1.0f),
    mDestination(static_cast<AudioNodeStream*>(aDestination->Stream())),
    mPlaybackRateTimeline(1.0f),
    mDetuneTimeline(0.0f),
    mLoop(false)
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
                                    TrackRate aSampleRate) override
  {
    switch (aIndex) {
    case AudioBufferSourceNode::PLAYBACKRATE:
      mPlaybackRateTimeline = aValue;
      WebAudioUtils::ConvertAudioParamToTicks(mPlaybackRateTimeline, mSource, mDestination);
      break;
    case AudioBufferSourceNode::DETUNE:
      mDetuneTimeline = aValue;
      WebAudioUtils::ConvertAudioParamToTicks(mDetuneTimeline, mSource, mDestination);
      break;
    default:
      NS_ERROR("Bad AudioBufferSourceNodeEngine TimelineParameter");
    }
  }
  virtual void SetStreamTimeParameter(uint32_t aIndex, StreamTime aParam) override
  {
    switch (aIndex) {
    case AudioBufferSourceNode::STOP: mStop = aParam; break;
    default:
      NS_ERROR("Bad AudioBufferSourceNodeEngine StreamTimeParameter");
    }
  }
  virtual void SetDoubleParameter(uint32_t aIndex, double aParam) override
  {
    switch (aIndex) {
    case AudioBufferSourceNode::START:
      MOZ_ASSERT(!mStart, "Another START?");
      mStart =
        mSource->FractionalTicksFromDestinationTime(mDestination, aParam);
      
      mBeginProcessing = mStart + 0.5;
      break;
    case AudioBufferSourceNode::DOPPLERSHIFT:
      mDopplerShift = (aParam <= 0 || mozilla::IsNaN(aParam)) ? 1.0 : aParam;
      break;
    default:
      NS_ERROR("Bad AudioBufferSourceNodeEngine double parameter.");
    };
  }
  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aParam) override
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
  virtual void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer) override
  {
    mBuffer = aBuffer;
  }

  bool BegunResampling()
  {
    return mBeginProcessing == -STREAM_TIME_MAX;
  }

  void UpdateResampler(int32_t aOutRate, uint32_t aChannels)
  {
    if (mResampler &&
        (aChannels != mChannels ||
         
         
         
         
         (aOutRate == mBufferSampleRate && !BegunResampling()))) {
      speex_resampler_destroy(mResampler);
      mResampler = nullptr;
      mRemainingResamplerTail = 0;
      mBeginProcessing = mStart + 0.5;
    }

    if (aOutRate == mBufferSampleRate && !mResampler) {
      return;
    }

    if (!mResampler) {
      mChannels = aChannels;
      mResampler = speex_resampler_init(mChannels, mBufferSampleRate, aOutRate,
                                        SPEEX_RESAMPLER_QUALITY_DEFAULT,
                                        nullptr);
    } else {
      uint32_t currentOutSampleRate, currentInSampleRate;
      speex_resampler_get_rate(mResampler, &currentInSampleRate,
                               &currentOutSampleRate);
      if (currentOutSampleRate == static_cast<uint32_t>(aOutRate)) {
        return;
      }
      speex_resampler_set_rate(mResampler, currentInSampleRate, aOutRate);
    }

    if (!BegunResampling()) {
      
      
      
      int64_t inputLatency = speex_resampler_get_input_latency(mResampler);
      uint32_t ratioNum, ratioDen;
      speex_resampler_get_ratio(mResampler, &ratioNum, &ratioDen);
      
      
      int64_t subsample = mStart * ratioNum + 0.5;
      
      
      
      mBeginProcessing =
        (subsample - inputLatency * ratioDen + ratioNum - 1) / ratioNum;
    }
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
                                         uint32_t* aOffsetWithinBlock,
                                         StreamTime* aCurrentPosition,
                                         int32_t aBufferMax) {
    
    uint32_t availableInOutputBuffer =
      WEBAUDIO_BLOCK_SIZE - *aOffsetWithinBlock;
    SpeexResamplerState* resampler = mResampler;
    MOZ_ASSERT(aChannels > 0);

    if (mBufferPosition < aBufferMax) {
      uint32_t availableInInputBuffer = aBufferMax - mBufferPosition;
      uint32_t ratioNum, ratioDen;
      speex_resampler_get_ratio(resampler, &ratioNum, &ratioDen);
      
      
      
      
      uint32_t inputLimit = availableInOutputBuffer * ratioNum / ratioDen + 10;
      if (!BegunResampling()) {
        
        uint32_t inputLatency = speex_resampler_get_input_latency(resampler);
        inputLimit += inputLatency;
        
        
        
        
        uint32_t skipFracNum = inputLatency * ratioDen;
        double leadTicks = mStart - *aCurrentPosition;
        if (leadTicks > 0.0) {
          
          
          skipFracNum -= leadTicks * ratioNum + 0.5;
          MOZ_ASSERT(skipFracNum < INT32_MAX, "mBeginProcessing is wrong?");
        }
        speex_resampler_set_skip_frac_num(resampler, skipFracNum);

        mBeginProcessing = -STREAM_TIME_MAX;
      }
      inputLimit = std::min(inputLimit, availableInInputBuffer);

      for (uint32_t i = 0; true; ) {
        uint32_t inSamples = inputLimit;
        const float* inputData = mBuffer->GetData(i) + mBufferPosition;

        uint32_t outSamples = availableInOutputBuffer;
        float* outputData =
          static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i])) +
          *aOffsetWithinBlock;

        WebAudioUtils::SpeexResamplerProcess(resampler, i,
                                             inputData, &inSamples,
                                             outputData, &outSamples);
        if (++i == aChannels) {
          mBufferPosition += inSamples;
          MOZ_ASSERT(mBufferPosition <= mBufferEnd || mLoop);
          *aOffsetWithinBlock += outSamples;
          *aCurrentPosition += outSamples;
          if (inSamples == availableInInputBuffer && !mLoop) {
            
            
            
            mRemainingResamplerTail =
              2 * speex_resampler_get_input_latency(resampler) - 1;
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
          *aOffsetWithinBlock;

        
        
        WebAudioUtils::SpeexResamplerProcess(resampler, i,
                         static_cast<AudioDataValue*>(nullptr), &inSamples,
                         outputData, &outSamples);
        if (++i == aChannels) {
          mRemainingResamplerTail -= inSamples;
          MOZ_ASSERT(mRemainingResamplerTail >= 0);
          *aOffsetWithinBlock += outSamples;
          *aCurrentPosition += outSamples;
          break;
        }
      }
    }
  }

  







  void FillWithZeroes(AudioChunk* aOutput,
                      uint32_t aChannels,
                      uint32_t* aOffsetWithinBlock,
                      StreamTime* aCurrentPosition,
                      StreamTime aMaxPos)
  {
    MOZ_ASSERT(*aCurrentPosition < aMaxPos);
    uint32_t numFrames =
      std::min<StreamTime>(WEBAUDIO_BLOCK_SIZE - *aOffsetWithinBlock,
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
                      StreamTime* aCurrentPosition,
                      int32_t aBufferMax)
  {
    MOZ_ASSERT(*aCurrentPosition < mStop);
    uint32_t numFrames =
      std::min(std::min<StreamTime>(WEBAUDIO_BLOCK_SIZE - *aOffsetWithinBlock,
                                    aBufferMax - mBufferPosition),
               mStop - *aCurrentPosition);
    if (numFrames == WEBAUDIO_BLOCK_SIZE && !mResampler) {
      MOZ_ASSERT(mBufferPosition < aBufferMax);
      BorrowFromInputBuffer(aOutput, aChannels);
      *aOffsetWithinBlock += numFrames;
      *aCurrentPosition += numFrames;
      mBufferPosition += numFrames;
    } else {
      if (*aOffsetWithinBlock == 0) {
        AllocateAudioBlock(aChannels, aOutput);
      }
      if (!mResampler) {
        MOZ_ASSERT(mBufferPosition < aBufferMax);
        CopyFromInputBuffer(aOutput, aChannels, *aOffsetWithinBlock, numFrames);
        *aOffsetWithinBlock += numFrames;
        *aCurrentPosition += numFrames;
        mBufferPosition += numFrames;
      } else {
        CopyFromInputBufferWithResampling(aStream, aOutput, aChannels, aOffsetWithinBlock, aCurrentPosition, aBufferMax);
      }
    }
  }

  int32_t ComputeFinalOutSampleRate(float aPlaybackRate, float aDetune)
  {
    float computedPlaybackRate = aPlaybackRate * pow(2, aDetune / 1200.f);
    
    
    int32_t rate = WebAudioUtils::
      TruncateFloatToInt<int32_t>(mSource->SampleRate() /
                                  (computedPlaybackRate * mDopplerShift));
    return rate ? rate : mBufferSampleRate;
  }

  void UpdateSampleRateIfNeeded(uint32_t aChannels)
  {
    float playbackRate;
    float detune;

    if (mPlaybackRateTimeline.HasSimpleValue()) {
      playbackRate = mPlaybackRateTimeline.GetValue();
    } else {
      playbackRate = mPlaybackRateTimeline.GetValueAtTime(mSource->GetCurrentPosition());
    }
    if (mDetuneTimeline.HasSimpleValue()) {
      detune = mDetuneTimeline.GetValue();
    } else {
      detune = mDetuneTimeline.GetValueAtTime(mSource->GetCurrentPosition());
    }
    if (playbackRate <= 0 || mozilla::IsNaN(playbackRate)) {
      playbackRate = 1.0f;
    }

    detune = std::min(std::max(-1200.f, detune), 1200.f);

    int32_t outRate = ComputeFinalOutSampleRate(playbackRate, detune);
    UpdateResampler(outRate, aChannels);
  }

  virtual void ProcessBlock(AudioNodeStream* aStream,
                            const AudioChunk& aInput,
                            AudioChunk* aOutput,
                            bool* aFinished) override
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

    UpdateSampleRateIfNeeded(channels);

    uint32_t written = 0;
    StreamTime streamPosition = aStream->GetCurrentPosition();
    while (written < WEBAUDIO_BLOCK_SIZE) {
      if (mStop != STREAM_TIME_MAX &&
          streamPosition >= mStop) {
        FillWithZeroes(aOutput, channels, &written, &streamPosition, STREAM_TIME_MAX);
        continue;
      }
      if (streamPosition < mBeginProcessing) {
        FillWithZeroes(aOutput, channels, &written, &streamPosition,
                       mBeginProcessing);
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
          FillWithZeroes(aOutput, channels, &written, &streamPosition, STREAM_TIME_MAX);
        }
      }
    }

    
    
    if (streamPosition >= mStop ||
        (!mLoop && mBufferPosition >= mBufferEnd && !mRemainingResamplerTail)) {
      *aFinished = true;
    }
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    
    
    
    

    size_t amount = AudioNodeEngine::SizeOfExcludingThis(aMallocSizeOf);

    
    
    
    
    
    
    
    amount += aMallocSizeOf(mResampler);

    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  double mStart; 
  
  
  
  
  StreamTime mBeginProcessing;
  StreamTime mStop;
  nsRefPtr<ThreadSharedFloatArrayBufferList> mBuffer;
  SpeexResamplerState* mResampler;
  
  
  int mRemainingResamplerTail;
  int32_t mBufferEnd;
  int32_t mLoopStart;
  int32_t mLoopEnd;
  int32_t mBufferSampleRate;
  int32_t mBufferPosition;
  uint32_t mChannels;
  float mDopplerShift;
  AudioNodeStream* mDestination;
  AudioNodeStream* mSource;
  AudioParamTimeline mPlaybackRateTimeline;
  AudioParamTimeline mDetuneTimeline;
  bool mLoop;
};

AudioBufferSourceNode::AudioBufferSourceNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mLoopStart(0.0)
  , mLoopEnd(0.0)
  
  , mPlaybackRate(new AudioParam(this, SendPlaybackRateToStream, 1.0f, "playbackRate"))
  , mDetune(new AudioParam(this, SendDetuneToStream, 0.0f, "detune"))
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

size_t
AudioBufferSourceNode::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = AudioNode::SizeOfExcludingThis(aMallocSizeOf);
  if (mBuffer) {
    amount += mBuffer->SizeOfIncludingThis(aMallocSizeOf);
  }

  amount += mPlaybackRate->SizeOfIncludingThis(aMallocSizeOf);
  amount += mDetune->SizeOfIncludingThis(aMallocSizeOf);
  return amount;
}

size_t
AudioBufferSourceNode::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

JSObject*
AudioBufferSourceNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return AudioBufferSourceNodeBinding::Wrap(aCx, this, aGivenProto);
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
    ns->SetDoubleParameter(START, mContext->DOMTimeToStreamTime(aWhen));
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
AudioBufferSourceNode::SendDetuneToStream(AudioNode* aNode)
{
  AudioBufferSourceNode* This = static_cast<AudioBufferSourceNode*>(aNode);
  SendTimelineParameterToStream(This, DETUNE, *This->mDetune);
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
