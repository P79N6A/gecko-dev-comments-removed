





#include "AudioDestinationNode.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED0(AudioDestinationNode, AudioNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext)
  : AudioNode(aContext)
{
}

JSObject*
AudioDestinationNode::WrapObject(JSContext* aCx, JSObject* aScope,
                                 bool* aTriedToWrap)
{
  return AudioDestinationNodeBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

}
}

