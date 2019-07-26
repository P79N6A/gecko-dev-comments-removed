





#include "AudioBufferSourceNode.h"
#include "mozilla/dom/AudioBufferSourceNodeBinding.h"
#include "nsMathUtils.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(AudioBufferSourceNode, AudioSourceNode, mBuffer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioBufferSourceNode)
NS_INTERFACE_MAP_END_INHERITING(AudioSourceNode)

NS_IMPL_ADDREF_INHERITED(AudioBufferSourceNode, AudioSourceNode)
NS_IMPL_RELEASE_INHERITED(AudioBufferSourceNode, AudioSourceNode)

class AudioBufferSourceNodeEngine : public AudioNodeEngine
{
public:
  AudioBufferSourceNodeEngine() :
    mStart(0), mStop(TRACK_TICKS_MAX),
    mOffset(0), mDuration(0),
    mLoop(NotLooping), mLoopStart(0), mLoopEnd(0)
  {}

  enum LoopState {
    NotLooping, 
    WillLoop,   
    IsLooping   
  };

  
  
  
  enum Parameters {
    START,
    STOP,
    OFFSET,
    DURATION,
    LOOP,
    LOOPSTART,
    LOOPEND
  };
  virtual void SetStreamTimeParameter(uint32_t aIndex, TrackTicks aParam)
  {
    switch (aIndex) {
    case START: mStart = aParam; break;
    case STOP: mStop = aParam; break;
    default:
      NS_ERROR("Bad AudioBufferSourceNodeEngine StreamTimeParameter");
    }
  }
  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aParam)
  {
    switch (aIndex) {
    case OFFSET: mOffset = aParam; break;
    case DURATION: mDuration = aParam; break;
    case LOOP: mLoop = aParam ? WillLoop: NotLooping; break;
    case LOOPSTART: mLoopStart = aParam; break;
    case LOOPEND: mLoopEnd = aParam; break;
    default:
      NS_ERROR("Bad AudioBufferSourceNodeEngine Int32Parameter");
    }
  }
  virtual void SetBuffer(already_AddRefed<ThreadSharedFloatArrayBufferList> aBuffer)
  {
    mBuffer = aBuffer;
  }

  void BorrowFromInputBuffer(AudioChunk* aOutput,
                             uint32_t aChannels,
                             uintptr_t aBufferOffset)
  {
    aOutput->mDuration = WEBAUDIO_BLOCK_SIZE;
    aOutput->mBuffer = mBuffer;
    aOutput->mChannelData.SetLength(aChannels);
    for (uint32_t i = 0; i < aChannels; ++i) {
      aOutput->mChannelData[i] = mBuffer->GetData(i) + aBufferOffset;
    }
    aOutput->mVolume = 1.0f;
    aOutput->mBufferFormat = AUDIO_FORMAT_FLOAT32;
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    if (!mBuffer)
      return;
    TrackTicks currentPosition = aStream->GetCurrentPosition();
    if (currentPosition + WEBAUDIO_BLOCK_SIZE <= mStart) {
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }

    
    TrackTicks endTime = std::min(mStart + mDuration, mStop);
    if (mLoop != NotLooping) {
      if (mStop != TRACK_TICKS_MAX &&
          currentPosition + WEBAUDIO_BLOCK_SIZE >= mStop) {
        *aFinished = true;
        aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
        return;
      }
    } else {
      
      
      if (currentPosition + WEBAUDIO_BLOCK_SIZE >= mStart + mDuration) {
        *aFinished = true;
      }
      if (currentPosition >= endTime || mStart >= endTime) {
        aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
        return;
      }
    }

    uint32_t channels = mBuffer->GetChannels();
    if (!channels) {
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }

    
    if (mLoop == NotLooping &&
        currentPosition >= mStart &&
        currentPosition + WEBAUDIO_BLOCK_SIZE <= endTime) {
      
      BorrowFromInputBuffer(aOutput, channels,
                            uintptr_t(currentPosition - mStart + mOffset));
      return;
    }

    
    TrackTicks startLoop = std::min(mStart + mLoopEnd - mOffset, mStop);
    if (mLoop == WillLoop &&
        currentPosition >= mStart &&
        currentPosition + WEBAUDIO_BLOCK_SIZE <= startLoop) {
      
      BorrowFromInputBuffer(aOutput, channels,
                            uintptr_t(currentPosition - mStart + mOffset));

      if (currentPosition + WEBAUDIO_BLOCK_SIZE == startLoop) {
        
        mLoop = IsLooping;
      }
      return;
    }

    
    int32_t loopLength;
    TrackTicks distanceFromLoopStart;
    if (mLoop == IsLooping &&
        currentPosition + WEBAUDIO_BLOCK_SIZE <= mStop) {
      MOZ_ASSERT(currentPosition >= mStart);

      loopLength = mLoopEnd - mLoopStart;
      TrackTicks intoLoop = currentPosition - mStart + mOffset - mLoopEnd;
      distanceFromLoopStart = intoLoop % loopLength;

      if (loopLength >= WEBAUDIO_BLOCK_SIZE &&
          distanceFromLoopStart + WEBAUDIO_BLOCK_SIZE <= loopLength) {
        
        BorrowFromInputBuffer(aOutput, channels, mLoopStart + distanceFromLoopStart);
        return;
      }
    }

    
    
    AllocateAudioBlock(channels, aOutput);
    TrackTicks start = std::max(currentPosition, mStart);
    if (mLoop == NotLooping) {
      
      TrackTicks end = std::min(currentPosition + WEBAUDIO_BLOCK_SIZE, endTime);
      WriteZeroesToAudioBlock(aOutput, 0, uint32_t(start - currentPosition));
      for (uint32_t i = 0; i < channels; ++i) {
        memcpy(static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i])) +
               uint32_t(start - currentPosition),
               mBuffer->GetData(i) +
               uintptr_t(start - mStart + mOffset),
               uint32_t(end - start) * sizeof(float));
      }
      uint32_t endOffset = uint32_t(end - currentPosition);
      WriteZeroesToAudioBlock(aOutput, endOffset, WEBAUDIO_BLOCK_SIZE - endOffset);
    } else if (mLoop == WillLoop) {
      
      TrackTicks end = std::min(currentPosition + WEBAUDIO_BLOCK_SIZE, mStop);
      TrackTicks endPreLoop = std::min(currentPosition + WEBAUDIO_BLOCK_SIZE,
                                       std::min(mStart + mLoopEnd, mStop));
      WriteZeroesToAudioBlock(aOutput, 0, uint32_t(start - currentPosition));
      for (uint32_t i = 0; i < channels; ++i) {
        float* baseChannelData = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i]));
        
        memcpy(baseChannelData + uint32_t(start - currentPosition),
               mBuffer->GetData(i) + uintptr_t(start - mStart + mOffset),
               uint32_t(endPreLoop - start) * sizeof(float));
        
        memcpy(baseChannelData + uint32_t(endPreLoop - currentPosition),
               mBuffer->GetData(i) + mLoopStart,
               uint32_t(end - endPreLoop) * sizeof(float));
      }
      uint32_t endOffset = uint32_t(end - currentPosition);
      WriteZeroesToAudioBlock(aOutput, endOffset, WEBAUDIO_BLOCK_SIZE - endOffset);

      if (currentPosition + WEBAUDIO_BLOCK_SIZE >= startLoop) {
        
        mLoop = IsLooping;
      }
    } else {
      
      MOZ_ASSERT(start == currentPosition);

      TrackTicks end = std::min(currentPosition + WEBAUDIO_BLOCK_SIZE, mStop);
      TrackTicks endLoop = std::min(currentPosition + loopLength - distanceFromLoopStart, mStop);
      MOZ_ASSERT(endLoop < currentPosition + WEBAUDIO_BLOCK_SIZE);
      for (uint32_t i = 0; i < channels; ++i) {
        float* baseChannelData = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[i]));
        
        memcpy(baseChannelData + uint32_t(start - currentPosition),
               mBuffer->GetData(i) + uintptr_t(distanceFromLoopStart + mLoopStart),
               uint32_t(endLoop - start) * sizeof(float));
        
        memcpy(baseChannelData + uint32_t(endLoop - currentPosition),
               mBuffer->GetData(i) + mLoopStart,
               uint32_t(end - endLoop) * sizeof(float));
      }
      uint32_t endOffset = uint32_t(end - currentPosition);
      WriteZeroesToAudioBlock(aOutput, endOffset, WEBAUDIO_BLOCK_SIZE - endOffset);
    }
  }

  TrackTicks mStart;
  TrackTicks mStop;
  nsRefPtr<ThreadSharedFloatArrayBufferList> mBuffer;
  int32_t mOffset;
  int32_t mDuration;
  LoopState mLoop;
  int32_t mLoopStart;
  int32_t mLoopEnd;
};

AudioBufferSourceNode::AudioBufferSourceNode(AudioContext* aContext)
  : AudioSourceNode(aContext)
  , mLoopStart(0.0)
  , mLoopEnd(0.0)
  , mLoop(false)
  , mStartCalled(false)
{
  SetProduceOwnOutput(true);
  mStream = aContext->Graph()->CreateAudioNodeStream(new AudioBufferSourceNodeEngine());
  mStream->AddMainThreadListener(this);
}

AudioBufferSourceNode::~AudioBufferSourceNode()
{
  DestroyMediaStream();
}

JSObject*
AudioBufferSourceNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return AudioBufferSourceNodeBinding::Wrap(aCx, aScope, this);
}

void
AudioBufferSourceNode::Start(JSContext* aCx, double aWhen, double aOffset,
                             const Optional<double>& aDuration, ErrorResult& aRv)
{
  if (mStartCalled) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mStartCalled = true;

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  if (!mBuffer || !ns) {
    
    return;
  }

  uint32_t rate = Context()->GetRate();
  uint32_t lengthSamples;
  nsRefPtr<ThreadSharedFloatArrayBufferList> data =
    mBuffer->GetThreadSharedChannelsForRate(aCx, rate, &lengthSamples);
  double length = double(lengthSamples)/rate;
  double offset = std::max(0.0, aOffset);
  double endOffset = aDuration.WasPassed() ?
      std::min(aOffset + aDuration.Value(), length) : length;
  if (offset >= endOffset) {
    return;
  }

  
  if (mLoop) {
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
    ns->SetInt32Parameter(AudioBufferSourceNodeEngine::LOOP, 1);
    ns->SetInt32Parameter(AudioBufferSourceNodeEngine::LOOPSTART, loopStartTicks);
    ns->SetInt32Parameter(AudioBufferSourceNodeEngine::LOOPEND, loopEndTicks);
  }

  ns->SetBuffer(data.forget());
  
  if (aWhen > 0.0) {
    ns->SetStreamTimeParameter(AudioBufferSourceNodeEngine::START,
                               Context()->DestinationStream(),
                               aWhen);
  }
  int32_t offsetTicks = NS_lround(offset*rate);
  
  if (offsetTicks > 0) {
    ns->SetInt32Parameter(AudioBufferSourceNodeEngine::OFFSET, offsetTicks);
  }
  ns->SetInt32Parameter(AudioBufferSourceNodeEngine::DURATION,
      NS_lround(endOffset*rate) - offsetTicks);
}

void
AudioBufferSourceNode::Stop(double aWhen, ErrorResult& aRv)
{
  if (!mStartCalled) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  if (!ns) {
    
    return;
  }

  ns->SetStreamTimeParameter(AudioBufferSourceNodeEngine::STOP,
                             Context()->DestinationStream(),
                             std::max(0.0, aWhen));
}

void
AudioBufferSourceNode::NotifyMainThreadStateChanged()
{
  if (mStream->IsFinished()) {
    SetProduceOwnOutput(false);
  }
}

}
}
