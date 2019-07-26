





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
  GainNodeEngine(AudioNode* aNode, AudioDestinationNode* aDestination)
    : AudioNodeEngine(aNode)
    , mSource(nullptr)
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
  void SetTimelineParameter(uint32_t aIndex,
                            const AudioParamTimeline& aValue,
                            TrackRate aSampleRate) MOZ_OVERRIDE
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

  virtual void ProcessBlock(AudioNodeStream* aStream,
                            const AudioChunk& aInput,
                            AudioChunk* aOutput,
                            bool* aFinished)
  {
    MOZ_ASSERT(mSource == aStream, "Invalid source stream");

    if (aInput.IsNull()) {
      
      aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
    } else if (mGain.HasSimpleValue()) {
      
      float gain = mGain.GetValue();
      if (gain == 0.0f) {
        aOutput->SetNull(WEBAUDIO_BLOCK_SIZE);
      } else {
        *aOutput = aInput;
        aOutput->mVolume *= gain;
      }
    } else {
      
      
      
      AllocateAudioBlock(aInput.mChannelData.Length(), aOutput);

      
      
      float computedGain[WEBAUDIO_BLOCK_SIZE];
      for (size_t counter = 0; counter < WEBAUDIO_BLOCK_SIZE; ++counter) {
        TrackTicks tick = aStream->GetCurrentPosition();
        computedGain[counter] = mGain.GetValueAtTime(tick, counter) * aInput.mVolume;
      }

      
      for (size_t channel = 0; channel < aOutput->mChannelData.Length(); ++channel) {
        const float* inputBuffer = static_cast<const float*> (aInput.mChannelData[channel]);
        float* buffer = static_cast<float*> (const_cast<void*>
                          (aOutput->mChannelData[channel]));
        AudioBlockCopyChannelWithScale(inputBuffer, computedGain, buffer);
      }
    }
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    
    
    
    
    return AudioNodeEngine::SizeOfExcludingThis(aMallocSizeOf);
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  AudioNodeStream* mSource;
  AudioNodeStream* mDestination;
  AudioParamTimeline mGain;
};

GainNode::GainNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Max,
              ChannelInterpretation::Speakers)
  , mGain(new AudioParam(MOZ_THIS_IN_INITIALIZER_LIST(),
                         SendGainToStream, 1.0f))
{
  GainNodeEngine* engine = new GainNodeEngine(this, aContext->Destination());
  mStream = aContext->Graph()->CreateAudioNodeStream(engine, MediaStreamGraph::INTERNAL_STREAM);
  engine->SetSourceStream(static_cast<AudioNodeStream*> (mStream.get()));
}

size_t
GainNode::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = AudioNode::SizeOfExcludingThis(aMallocSizeOf);
  amount += mGain->SizeOfIncludingThis(aMallocSizeOf);
  return amount;
}

size_t
GainNode::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

JSObject*
GainNode::WrapObject(JSContext* aCx)
{
  return GainNodeBinding::Wrap(aCx, this);
}

void
GainNode::SendGainToStream(AudioNode* aNode)
{
  GainNode* This = static_cast<GainNode*>(aNode);
  SendTimelineParameterToStream(This, GainNodeEngine::GAIN, *This->mGain);
}

}
}

