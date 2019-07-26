





#include "DelayNode.h"
#include "mozilla/dom/DelayNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "WebAudioUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(DelayNode, AudioNode,
                                     mDelay)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DelayNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(DelayNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(DelayNode, AudioNode)

class DelayNodeEngine : public AudioNodeEngine
{
  class PlayingRefChanged : public nsRunnable
  {
  public:
    enum ChangeType { ADDREF, RELEASE };
    PlayingRefChanged(AudioNodeStream* aStream, ChangeType aChange)
      : mStream(aStream)
      , mChange(aChange)
    {
    }

    NS_IMETHOD Run()
    {
      nsRefPtr<DelayNode> node;
      {
        
        
        
        
        MutexAutoLock lock(mStream->Engine()->NodeMutex());
        node = static_cast<DelayNode*>(mStream->Engine()->Node());
      }
      if (node) {
        if (mChange == ADDREF) {
          node->mPlayingRef.Take(node);
        } else if (mChange == RELEASE) {
          node->mPlayingRef.Drop(node);
        }
      }
      return NS_OK;
    }

  private:
    nsRefPtr<AudioNodeStream> mStream;
    ChangeType mChange;
  };

public:
  DelayNodeEngine(AudioNode* aNode, AudioDestinationNode* aDestination)
    : AudioNodeEngine(aNode)
    , mSource(nullptr)
    , mDestination(static_cast<AudioNodeStream*> (aDestination->Stream()))
    
    , mDelay(0.f)
    , mMaxDelay(0.)
    , mWriteIndex(0)
    , mLeftOverData(INT32_MIN)
    , mCurrentDelayTime(0.)
  {
  }

  void SetSourceStream(AudioNodeStream* aSource)
  {
    mSource = aSource;
  }

  enum Parameters {
    DELAY,
    MAX_DELAY
  };
  void SetTimelineParameter(uint32_t aIndex, const AudioParamTimeline& aValue) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case DELAY:
      MOZ_ASSERT(mSource && mDestination);
      mDelay = aValue;
      WebAudioUtils::ConvertAudioParamToTicks(mDelay, mSource, mDestination);
      break;
    default:
      NS_ERROR("Bad DelayNodeEngine TimelineParameter");
    }
  }
  void SetDoubleParameter(uint32_t aIndex, double aValue) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case MAX_DELAY: mMaxDelay = aValue; break;
    default:
      NS_ERROR("Bad DelayNodeEngine DoubleParameter");
    }
  }

  bool EnsureBuffer(uint32_t aNumberOfChannels)
  {
    if (aNumberOfChannels == 0) {
      return false;
    }
    if (mBuffer.Length() == 0) {
      if (!mBuffer.SetLength(aNumberOfChannels)) {
        return false;
      }
      const int32_t numFrames = NS_lround(mMaxDelay) * IdealAudioRate();
      for (uint32_t channel = 0; channel < aNumberOfChannels; ++channel) {
        if (!mBuffer[channel].SetLength(numFrames)) {
          return false;
        }
        memset(mBuffer[channel].Elements(), 0, numFrames * sizeof(float));
      }
    } else if (mBuffer.Length() != aNumberOfChannels) {
      
      return false;
    }
    return true;
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    MOZ_ASSERT(mSource == aStream, "Invalid source stream");

    const bool firstTime = !!!mBuffer.Length();
    const uint32_t numChannels = aInput.IsNull() ?
                                 mBuffer.Length() :
                                 aInput.mChannelData.Length();

    bool playedBackAllLeftOvers = false;
    if (!mBuffer.IsEmpty() &&
        mLeftOverData == INT32_MIN &&
        aStream->AllInputsFinished()) {
      mLeftOverData = static_cast<int32_t>(mCurrentDelayTime * IdealAudioRate());

      nsRefPtr<PlayingRefChanged> refchanged =
        new PlayingRefChanged(aStream, PlayingRefChanged::ADDREF);
      NS_DispatchToMainThread(refchanged);
    } else if (mLeftOverData != INT32_MIN) {
      mLeftOverData -= WEBAUDIO_BLOCK_SIZE;
      if (mLeftOverData <= 0) {
        mLeftOverData = INT32_MIN;
        playedBackAllLeftOvers = true;

        nsRefPtr<PlayingRefChanged> refchanged =
          new PlayingRefChanged(aStream, PlayingRefChanged::RELEASE);
        NS_DispatchToMainThread(refchanged);
      }
    }

    if (!EnsureBuffer(numChannels)) {
      aOutput->SetNull(0);
      return;
    }

    AllocateAudioBlock(numChannels, aOutput);

    double delayTime = 0;
    float computedDelay[WEBAUDIO_BLOCK_SIZE];
    
    const double smoothingRate = WebAudioUtils::ComputeSmoothingRate(0.02, IdealAudioRate());

    if (mDelay.HasSimpleValue()) {
      delayTime = std::max(0.0, std::min(mMaxDelay, double(mDelay.GetValue())));
      if (firstTime) {
        
        
        mCurrentDelayTime = delayTime;
      }
    } else {
      
      TrackTicks tick = aStream->GetCurrentPosition();
      for (size_t counter = 0; counter < WEBAUDIO_BLOCK_SIZE; ++counter) {
        computedDelay[counter] = std::max(0.0, std::min(mMaxDelay,
                                   double(mDelay.GetValueAtTime<TrackTicks>(tick + counter))));
      }
    }

    for (uint32_t channel = 0; channel < numChannels; ++channel) {
      double currentDelayTime = mCurrentDelayTime;
      uint32_t writeIndex = mWriteIndex;

      float* buffer = mBuffer[channel].Elements();
      const uint32_t bufferLength = mBuffer[channel].Length();
      const float* input = static_cast<const float*>(aInput.mChannelData.SafeElementAt(channel));
      float* output = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[channel]));

      for (uint32_t i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
        if (mDelay.HasSimpleValue()) {
          
          currentDelayTime += (delayTime - currentDelayTime) * smoothingRate;
        } else {
          currentDelayTime = computedDelay[i];
        }

        
        if (input) {
          buffer[writeIndex] = input[i] * aInput.mVolume;
        }

        
        
        
        double readPosition = writeIndex + bufferLength -
                              (currentDelayTime * IdealAudioRate());
        if (readPosition >= bufferLength) {
          readPosition -= bufferLength;
        }
        MOZ_ASSERT(readPosition >= 0.0, "Why are we reading before the beginning of the buffer?");

        
        
        
        
        
        
        
        
        
        int readIndex1 = int(readPosition);
        int readIndex2 = (readIndex1 + 1) % bufferLength;
        double interpolationFactor = readPosition - readIndex1;

        output[i] = (1.0 - interpolationFactor) * buffer[readIndex1] +
                           interpolationFactor  * buffer[readIndex2];
        writeIndex = (writeIndex + 1) % bufferLength;
      }

      
      
      if (channel == numChannels - 1) {
        mCurrentDelayTime = currentDelayTime;
        mWriteIndex = writeIndex;
      }
    }

    if (playedBackAllLeftOvers) {
      
      mBuffer.Clear();
    }
  }

  AudioNodeStream* mSource;
  AudioNodeStream* mDestination;
  AudioParamTimeline mDelay;
  
  double mMaxDelay;
  
  AutoFallibleTArray<FallibleTArray<float>, 2> mBuffer;
  
  
  uint32_t mWriteIndex;
  
  
  int32_t mLeftOverData;
  
  double mCurrentDelayTime;
};

DelayNode::DelayNode(AudioContext* aContext, double aMaxDelay)
  : AudioNode(aContext)
  , mDelay(new AudioParam(this, SendDelayToStream, 0.0f))
{
  DelayNodeEngine* engine = new DelayNodeEngine(this, aContext->Destination());
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);
  engine->SetSourceStream(static_cast<AudioNodeStream*> (mStream.get()));
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(mStream.get());
  ns->SetDoubleParameter(DelayNodeEngine::MAX_DELAY, aMaxDelay);
}

JSObject*
DelayNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return DelayNodeBinding::Wrap(aCx, aScope, this);
}

void
DelayNode::SendDelayToStream(AudioNode* aNode)
{
  DelayNode* This = static_cast<DelayNode*>(aNode);
  SendTimelineParameterToStream(This, DelayNodeEngine::DELAY, *This->mDelay);
}

}
}

