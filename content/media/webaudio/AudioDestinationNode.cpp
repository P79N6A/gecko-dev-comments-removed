





#include "AudioDestinationNode.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "MediaStreamGraph.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(AudioDestinationNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(AudioDestinationNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(AudioDestinationNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioDestinationNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_CYCLE_COLLECTING_ADDREF(AudioDestinationNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AudioDestinationNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext, MediaStreamGraph* aGraph)
  : AudioNode(aContext)
{
  mStream = aGraph->CreateAudioNodeStream(new AudioNodeEngine(),
                                          MediaStreamGraph::EXTERNAL_STREAM);
  SetIsDOMBinding();
}

JSObject*
AudioDestinationNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return AudioDestinationNodeBinding::Wrap(aCx, aScope, this);
}

}
}
