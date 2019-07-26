





#include "DelayNode.h"
#include "mozilla/dom/DelayNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "WebAudioUtils.h"
#include "DelayBuffer.h"
#include "PlayingRefChangeHandler.h"

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
  typedef PlayingRefChangeHandler PlayingRefChanged;
public:
  DelayNodeEngine(AudioNode* aNode, AudioDestinationNode* aDestination,
                  int aMaxDelayTicks)
    : AudioNodeEngine(aNode)
    , mSource(nullptr)
    , mDestination(static_cast<AudioNodeStream*> (aDestination->Stream()))
    
    , mDelay(0.f)
    
    , mBuffer(aMaxDelayTicks,
              WebAudioUtils::ComputeSmoothingRate(0.02,
                                                  mDestination->SampleRate()))
    , mLeftOverData(INT32_MIN)
  {
  }

  virtual DelayNodeEngine* AsDelayNodeEngine()
  {
    return this;
  }

  void SetSourceStream(AudioNodeStream* aSource)
  {
    mSource = aSource;
  }

  enum Parameters {
    DELAY,
  };
  void SetTimelineParameter(uint32_t aIndex,
                            const AudioParamTimeline& aValue,
                            TrackRate aSampleRate) MOZ_OVERRIDE
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

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    MOZ_ASSERT(mSource == aStream, "Invalid source stream");
    MOZ_ASSERT(aStream->SampleRate() == mDestination->SampleRate());

    if (!aInput.IsNull()) {
      if (mLeftOverData <= 0) {
        nsRefPtr<PlayingRefChanged> refchanged =
          new PlayingRefChanged(aStream, PlayingRefChanged::ADDREF);
        aStream->Graph()->
          DispatchToMainThreadAfterStreamStateUpdate(refchanged.forget());
      }
      mLeftOverData = mBuffer.MaxDelayTicks();
    } else if (mLeftOverData > 0) {
      mLeftOverData -= WEBAUDIO_BLOCK_SIZE;
    } else {
      if (mLeftOverData != INT32_MIN) {
        mLeftOverData = INT32_MIN;
        
        mBuffer.Reset();

        nsRefPtr<PlayingRefChanged> refchanged =
          new PlayingRefChanged(aStream, PlayingRefChanged::RELEASE);
        aStream->Graph()->
          DispatchToMainThreadAfterStreamStateUpdate(refchanged.forget());
      }
      *aOutput = aInput;
      return;
    }

    mBuffer.Write(aInput);

    bool inCycle = aStream->AsProcessedStream()->InCycle();
    double minDelay = inCycle ? static_cast<double>(WEBAUDIO_BLOCK_SIZE) : 0.0;
    double maxDelay = mBuffer.MaxDelayTicks();
    double sampleRate = aStream->SampleRate();
    ChannelInterpretation channelInterpretation =
      aStream->GetChannelInterpretation();
    if (mDelay.HasSimpleValue()) {
      
      
      double delayFrames = mDelay.GetValue() * sampleRate;
      double delayFramesClamped = clamped(delayFrames, minDelay, maxDelay);
      mBuffer.Read(delayFramesClamped, aOutput, channelInterpretation);
    } else {
      
      
      
      double computedDelay[WEBAUDIO_BLOCK_SIZE];
      TrackTicks tick = aStream->GetCurrentPosition();
      for (size_t counter = 0; counter < WEBAUDIO_BLOCK_SIZE; ++counter) {
        double delayAtTick = mDelay.GetValueAtTime(tick, counter) * sampleRate;
        double delayAtTickClamped = clamped(delayAtTick, minDelay, maxDelay);
        computedDelay[counter] = delayAtTickClamped;
      }
      mBuffer.Read(computedDelay, aOutput, channelInterpretation);
    }
    mBuffer.NextBlock();
  }

  AudioNodeStream* mSource;
  AudioNodeStream* mDestination;
  AudioParamTimeline mDelay;
  DelayBuffer mBuffer;
  
  
  int32_t mLeftOverData;
};

DelayNode::DelayNode(AudioContext* aContext, double aMaxDelay)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mDelay(new AudioParam(MOZ_THIS_IN_INITIALIZER_LIST(),
                          SendDelayToStream, 0.0f))
{
  DelayNodeEngine* engine =
    new DelayNodeEngine(this, aContext->Destination(),
                        ceil(aContext->SampleRate() * aMaxDelay));
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);
  engine->SetSourceStream(static_cast<AudioNodeStream*> (mStream.get()));
}

JSObject*
DelayNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
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

