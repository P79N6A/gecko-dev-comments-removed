





#include "GainNode.h"
#include "mozilla/dom/GainNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(GainNode, AudioNode,
                                     mGain)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(GainNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(GainNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(GainNode, AudioNode)

struct ConvertTimeToTickHelper
{
  AudioNodeStream* mSourceStream;
  AudioNodeStream* mDestinationStream;

  static int64_t Convert(double aTime, void* aClosure)
  {
    TrackRate sampleRate = IdealAudioRate();

    ConvertTimeToTickHelper* This = static_cast<ConvertTimeToTickHelper*> (aClosure);
    TrackTicks tick = This->mSourceStream->GetCurrentPosition();
    StreamTime streamTime = TicksToTimeRoundDown(sampleRate, tick);
    GraphTime graphTime = This->mSourceStream->StreamTimeToGraphTime(streamTime);
    StreamTime destinationStreamTime = This->mDestinationStream->GraphTimeToStreamTime(graphTime);
    return TimeToTicksRoundDown(sampleRate, destinationStreamTime + SecondsToMediaTime(aTime));
  }
};

class GainNodeEngine : public AudioNodeEngine
{
public:
  explicit GainNodeEngine(AudioDestinationNode* aDestination)
    : mSource(nullptr)
    , mDestination(static_cast<AudioNodeStream*> (aDestination->Stream()))
  {
  }

  void SetSourceStream(AudioNodeStream* aSource)
  {
    mSource = aSource;
  }

  enum Parameters {
    GAIN
  };
  void SetTimelineParameter(uint32_t aIndex, const AudioParamTimeline& aValue) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case GAIN:
      MOZ_ASSERT(mSource && mDestination);
      mGain = aValue;
      ConvertTimeToTickHelper ctth;
      ctth.mSourceStream = mSource;
      ctth.mDestinationStream = mDestination;
      mGain.ConvertEventTimesToTicks(ConvertTimeToTickHelper::Convert, &ctth);
      break;
    default:
      NS_ERROR("Bad GainNodeEngine TimelineParameter");
    }
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished)
  {
    MOZ_ASSERT(mSource == aStream, "Invalid source stream");

    *aOutput = aInput;
    if (mGain.HasSimpleValue()) {
      
      aOutput->mVolume *= mGain.GetValue();
    } else {
      
      
      

      
      
      float computedGain[WEBAUDIO_BLOCK_SIZE];
      for (size_t counter = 0; counter < WEBAUDIO_BLOCK_SIZE; ++counter) {
        TrackTicks tick = aStream->GetCurrentPosition() + counter;
        computedGain[counter] = mGain.GetValueAtTime<TrackTicks>(tick);
      }

      
      for (size_t channel = 0; channel < aOutput->mChannelData.Length(); ++channel) {
        float* buffer = static_cast<float*> (const_cast<void*>
                          (aOutput->mChannelData[channel]));
        AudioBlockCopyChannelWithScale(buffer, computedGain, buffer);
      }
    }
  }

  AudioNodeStream* mSource;
  AudioNodeStream* mDestination;
  AudioParamTimeline mGain;
};

GainNode::GainNode(AudioContext* aContext)
  : AudioNode(aContext)
  , mGain(new AudioParam(this, SendGainToStream, 1.0f, 0.0f, 1.0f))
{
  GainNodeEngine* engine = new GainNodeEngine(aContext->Destination());
  mStream = aContext->Graph()->CreateAudioNodeStream(engine);
  engine->SetSourceStream(static_cast<AudioNodeStream*> (mStream.get()));
}

GainNode::~GainNode()
{
  DestroyMediaStream();
}

JSObject*
GainNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return GainNodeBinding::Wrap(aCx, aScope, this);
}

void
GainNode::SendGainToStream(AudioNode* aNode)
{
  GainNode* This = static_cast<GainNode*>(aNode);
  AudioNodeStream* ns = static_cast<AudioNodeStream*>(This->mStream.get());
  ns->SetTimelineParameter(GainNodeEngine::GAIN, *This->mGain);
}

}
}

