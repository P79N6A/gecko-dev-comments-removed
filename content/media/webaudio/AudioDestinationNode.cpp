





#include "AudioDestinationNode.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(AudioDestinationNode, AudioNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(AudioDestinationNode, AudioNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(AudioDestinationNode, AudioNode)              \
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(AudioDestinationNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext)
  : AudioNode(aContext)
{
  SetIsDOMBinding();
}

JSObject*
AudioDestinationNode::WrapObject(JSContext* aCx, JSObject* aScope,
                                 bool* aTriedToWrap)
{
  return AudioDestinationNodeBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

}
}
