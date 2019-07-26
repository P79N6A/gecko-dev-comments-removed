





#include "AudioDestinationNode.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "MediaStreamGraph.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(AudioDestinationNode, AudioNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext, MediaStreamGraph* aGraph)
  : AudioNode(aContext)
{
  mStream = aGraph->CreateAudioNodeStream(new AudioNodeEngine(),
                                          MediaStreamGraph::EXTERNAL_STREAM);
}

JSObject*
AudioDestinationNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return AudioDestinationNodeBinding::Wrap(aCx, aScope, this);
}

}
}
