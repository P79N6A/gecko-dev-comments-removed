





#include "GainNode.h"
#include "mozilla/dom/GainNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "WebAudioUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(GainNode, AudioNode,
                                     mGain)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(GainNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(GainNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(GainNode, AudioNode)

class GainNodeEngine : public AudioNodeEngine
{
public:
  explicit GainNodeEngine(AudioDestinationNode* aDestination)
    : mSource(nullptr)
    , mDestination(static_cast<AudioNodeStream*> (aDestination->Stream()))
    
    , mGain(1.f)
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
      WebAudioUtils::ConvertAudioParamToTicks(mGain, mSource, mDestination);
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
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);
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

