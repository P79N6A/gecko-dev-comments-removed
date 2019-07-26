





#include "GainNode.h"
#include "mozilla/dom/GainNodeBinding.h"
#include "AudioNodeEngine.h"
#include "GainProcessor.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(GainNode, AudioNode,
                                     mGain)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(GainNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(GainNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(GainNode, AudioNode)

class GainNodeEngine : public AudioNodeEngine,
                       public GainProcessor
{
public:
  GainNodeEngine(AudioNode* aNode, AudioDestinationNode* aDestination)
    : AudioNodeEngine(aNode)
    , GainProcessor(aDestination)
  {
  }

  enum Parameters {
    GAIN
  };
  void SetTimelineParameter(uint32_t aIndex,
                            const AudioParamTimeline& aValue,
                            TrackRate aSampleRate) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case GAIN:
      SetGainParameter(aValue);
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

    if (aInput.IsNull()) {
      
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
    } else {
      if (mGain.HasSimpleValue()) {
        
        
        *aOutput = aInput;
      } else {
        
        AllocateAudioBlock(aInput.mChannelData.Length(), aOutput);
      }
      ProcessGain(aStream, aInput.mVolume, aInput.mChannelData, aOutput);
    }
  }
};

GainNode::GainNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mGain(new AudioParam(this, SendGainToStream, 1.0f))
{
  GainNodeEngine* engine = new GainNodeEngine(this, aContext->Destination());
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);
  engine->SetSourceStream(static_cast<AudioNodeStream*> (mStream.get()));
}

JSObject*
GainNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return GainNodeBinding::Wrap(aCx, aScope, this);
}

void
GainNode::SendGainToStream(AudioNode* aNode)
{
  GainNode* This = static_cast<GainNode*>(aNode);
  SendTimelineParameterToStream(This, GainNodeEngine::GAIN, *This->mGain);
}

}
}

