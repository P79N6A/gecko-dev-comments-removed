





#include "AudioBufferSourceNode.h"
#include "mozilla/dom/AudioBufferSourceNodeBinding.h"
#include "nsMathUtils.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "PannerNode.h"
#include "speex/speex_resampler.h"
#include <limits>

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AudioBufferSourceNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mBuffer)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPlaybackRate)
  if (tmp->Context()) {
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
    mSampleRate(0), mPosition(0), mChannels(0), mPlaybackRate(1.0f),
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

  virtual void SetTimelineParameter(uint32_t aIndex, const dom::AudioParamTimeline& aValue)
  {
    switch (aIndex) {
    case AudioBufferSourceNode::PLAYBACKRATE:
      mPlaybackRateTimeline = aValue;
      
      
      
      if (mResampler && mPlaybackRateTimeline.HasSimpleValue() &&
          mPlaybackRateTimeline.GetValue() == 1.0 &&
          mSampleRate == IdealAudioRate()) {
        speex_resampler_destroy(mResampler);
        mResampler = nullptr;
      }
      WebAudioUtils::ConvertAudioParamToTicks(mPlaybackRateTimeline, nullptr, mDestination);
      break;
    default:
      NS_ERROR("Bad GainNodeEngine TimelineParameter");
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
    case AudioBufferSourceNode::SAMPLE_RATE: mSampleRate = aParam; break;
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

  SpeexResamplerState* Resampler(uint32_t aChannels)
  {
    if (aChannels != mChannels && mResampler) {
      speex_resampler_destroy(mResampler);
      mResampler = nullptr;
    }

    if (!mResampler) {
      mChannels = aChannels;
      mResampler = speex_resampler_init(mChannels, mSampleRate,
                                        ComputeFinalOutSampleRate(),
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

  
  
  
  
  
  void CopyFromInputBufferWithResampling(AudioChunk* aOutput,
                                         uint32_t aChannels,
                                         uintptr_t aSourceOffset,
                                         uintptr_t aBufferOffset,
                                         uint32_t aAvailableInInputBuffer,
                                         uint32_t& aFramesRead,
                                         uint32_t& aFramesWritten) {
    double finalPlaybackRate = static_cast<double>(mSampleRate) / ComputeFinalOutSampleRate();
    uint32_t availableInOuputBuffer = WEBAUDIO_BLOCK_SIZE - aBufferOffset;
    uint32_t inputSamples, outputSamples;

    
    if (aAvailableInInputBuffer < availableInOuputBuffer * finalPlaybackRate) {
      outputSamples = ceil(aAvailableInInputBuffer / finalPlaybackRate);
      inputSamples = aAvailableInInputBuffer;
    } else {
      inputSamples = ceil(availableInOuputBuffer * finalPlaybackRate);
      outputSamples = availableInOuputBuffer;
    }

    SpeexResamplerState* resampler = Resampler(aChannels);

    for (uint32_t i = 0; i < aChannels; ++i) {
      uint32_t inSamples = inputSamples;
      uint32_t outSamples = outputSamples;

      const float* inputData = mBuffer->GetData(i) + aSourceOffset;
      float* outputData =
        static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i])) +
        aBufferOffset;

      speex_resampler_process_float(resampler, i,
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
    uint32_t numFrames = std::min(WEBAUDIO_BLOCK_SIZE - *aOffsetWithinBlock,
                                  uint32_t(aMaxPos - *aCurrentPosition));
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

  








  void CopyFromBuffer(AudioChunk* aOutput,
                      uint32_t aChannels,
                      uint32_t* aOffsetWithinBlock,
                      TrackTicks* aCurrentPosition,
                      uint32_t aBufferOffset,
                      uint32_t aBufferMax)
  {
    uint32_t numFrames = std::min(std::min(WEBAUDIO_BLOCK_SIZE - *aOffsetWithinBlock,
                                           aBufferMax - aBufferOffset),
                                  uint32_t(mStop - *aCurrentPosition));
    if (numFrames == WEBAUDIO_BLOCK_SIZE && !ShouldResample()) {
      BorrowFromInputBuffer(aOutput, aChannels, aBufferOffset);
      *aOffsetWithinBlock += numFrames;
      *aCurrentPosition += numFrames;
      mPosition += numFrames;
    } else {
      if (aOutput->IsNull()) {
        MOZ_ASSERT(*aOffsetWithinBlock == 0);
        AllocateAudioBlock(aChannels, aOutput);
      }
      if (!ShouldResample()) {
        CopyFromInputBuffer(aOutput, aChannels, aBufferOffset, *aOffsetWithinBlock, numFrames);
        *aOffsetWithinBlock += numFrames;
        *aCurrentPosition += numFrames;
        mPosition += numFrames;
      } else {
        uint32_t framesRead, framesWritten, availableInInputBuffer;

        availableInInputBuffer = aBufferMax - aBufferOffset;

        CopyFromInputBufferWithResampling(aOutput, aChannels, aBufferOffset, *aOffsetWithinBlock, availableInInputBuffer, framesRead, framesWritten);
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

  int32_t ComputeFinalOutSampleRate() const
  {
    return static_cast<uint32_t>(IdealAudioRate() / (mPlaybackRate * mDopplerShift));
  }

  bool ShouldResample() const
  {
    return !(mPlaybackRate == 1.0 &&
             mDopplerShift == 1.0 &&
             mSampleRate == IdealAudioRate());
  }

  void UpdateSampleRateIfNeeded(AudioNodeStream* aStream, uint32_t aChannels)
  {
    if (mPlaybackRateTimeline.HasSimpleValue()) {
      mPlaybackRate = mPlaybackRateTimeline.GetValue();
    } else {
      mPlaybackRate = mPlaybackRateTimeline.GetValueAtTime<TrackTicks>(aStream->GetCurrentPosition());
    }

    
    
    if (ComputeFinalOutSampleRate() == 0) {
      mPlaybackRate = 1.0;
      mDopplerShift = 1.0;
    }

    uint32_t currentOutSampleRate, currentInSampleRate;
    if (ShouldResample()) {
      SpeexResamplerState* resampler = Resampler(aChannels);
      speex_resampler_get_rate(resampler, &currentInSampleRate, &currentOutSampleRate);
      uint32_t finalSampleRate = ComputeFinalOutSampleRate();
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
    if (!mBuffer)
      return;

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
          CopyFromBuffer(aOutput, channels, &written, &currentPosition, mOffset + t, mLoopEnd);
        } else {
          uint32_t offsetInLoop = (mOffset + t - mLoopEnd) % (mLoopEnd - mLoopStart);
          CopyFromBuffer(aOutput, channels, &written, &currentPosition, mLoopStart + offsetInLoop, mLoopEnd);
        }
      } else {
        if (mOffset + t < mDuration) {
          CopyFromBuffer(aOutput, channels, &written, &currentPosition, mOffset + t, mDuration);
        } else {
          FillWithZeroes(aOutput, channels, &written, &currentPosition, TRACK_TICKS_MAX);
        }
      }
    }

    
    
    if (currentPosition >= mStop ||
        (!mLoop && currentPosition - mStart + mOffset > mDuration)) {
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
  int32_t mSampleRate;
  uint32_t mPosition;
  uint32_t mChannels;
  float mPlaybackRate;
  float mDopplerShift;
  AudioNodeStream* mDestination;
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
  , mPlaybackRate(new AudioParam(this, SendPlaybackRateToStream, 1.0f))
  , mPannerNode(nullptr)
  , mLoop(false)
  , mStartCalled(false)
  , mOffsetAndDurationRemembered(false)
{
  mStream = aContext->Graph()->CreateAudioNodeStream(
      new AudioBufferSourceNodeEngine(this, aContext->Destination()),
      MediaStreamGraph::INTERNAL_STREAM);
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
    mOffsetAndDurationRemembered = true;
  }

  
  if (aWhen > 0.0) {
    ns->SetStreamTimeParameter(START, Context()->DestinationStream(), aWhen);
  }

  MOZ_ASSERT(!mPlayingRef, "We can only accept a successful start() call once");
  mPlayingRef.Take(this);
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
  } else {
    ns->SetBuffer(nullptr);
  }

  if (mOffsetAndDurationRemembered) {
    SendOffsetAndDurationParametersToStream(ns, mOffset, mDuration);
  }
}

void
AudioBufferSourceNode::SendOffsetAndDurationParametersToStream(AudioNodeStream* aStream,
                                                               double aOffset,
                                                               double aDuration)
{
  float rate = mBuffer ? mBuffer->SampleRate() : Context()->SampleRate();
  int32_t lengthSamples = mBuffer ? mBuffer->Length() : 0;
  double length = double(lengthSamples) / rate;
  double offset = std::max(0.0, aOffset);
  double endOffset = aDuration == std::numeric_limits<double>::min() ?
                     length : std::min(aOffset + aDuration, length);

  if (offset >= endOffset) {
    return;
  }

  int32_t offsetTicks = NS_lround(offset*rate);
  
  if (offsetTicks > 0) {
    aStream->SetInt32Parameter(OFFSET, offsetTicks);
  }
  aStream->SetInt32Parameter(DURATION, NS_lround(endOffset*rate) - offsetTicks);
}

void
AudioBufferSourceNode::Stop(double aWhen, ErrorResult& aRv)
{
  if (!mStartCalled) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (!mBuffer) {
    
    
    mPlayingRef.Drop(this);
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
    
    
    mPlayingRef.Drop(this);
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
    if (((mLoopStart != 0.0) || (mLoopEnd != 0.0)) &&
        mLoopStart >= 0.0 && mLoopEnd > 0.0 &&
        mLoopStart < mLoopEnd) {
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
    }
  } else if (!mLoop) {
    SendInt32ParameterToStream(LOOP, 0);
  }
}

}
}
