





#include "AudioDestinationNode.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "MediaStreamGraph.h"

namespace mozilla {
namespace dom {

class OfflineDestinationNodeEngine : public AudioNodeEngine
{
public:
  OfflineDestinationNodeEngine(AudioDestinationNode* aNode,
                               uint32_t aNumberOfChannels,
                               uint32_t aLength,
                               float aSampleRate)
    : AudioNodeEngine(aNode)
    , mNumberOfChannels(aNumberOfChannels)
    , mLength(aLength)
    , mSampleRate(aSampleRate)
  {
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished) MOZ_OVERRIDE
  {
    *aOutput = aInput;
  }

  uint32_t mNumberOfChannels;
  uint32_t mLength;
  float mSampleRate;
};

NS_IMPL_ISUPPORTS_INHERITED0(AudioDestinationNode, AudioNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext,
                                           bool aIsOffline,
                                           uint32_t aNumberOfChannels,
                                           uint32_t aLength,
                                           float aSampleRate)
  : AudioNode(aContext,
              aIsOffline ? aNumberOfChannels : 2,
              ChannelCountMode::Explicit,
              ChannelInterpretation::Speakers)
  , mFramesToProduce(aLength)
{
  MediaStreamGraph* graph = aIsOffline ?
                            MediaStreamGraph::CreateNonRealtimeInstance() :
                            MediaStreamGraph::GetInstance();
  AudioNodeEngine* engine = aIsOffline ?
                            new OfflineDestinationNodeEngine(this, aNumberOfChannels,
                                                             aLength, aSampleRate) :
                            new AudioNodeEngine(this);
  mStream = graph->CreateAudioNodeStream(engine, MediaStreamGraph::EXTERNAL_STREAM);
}

JSObject*
AudioDestinationNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return AudioDestinationNodeBinding::Wrap(aCx, aScope, this);
}

void
AudioDestinationNode::StartRendering()
{
  mStream->Graph()->StartNonRealtimeProcessing(mFramesToProduce);
}

}
}
