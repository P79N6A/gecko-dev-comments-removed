





#include "GainNode.h"
#include "mozilla/dom/GainNodeBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(GainNode, AudioNode,
                                     mGain)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(GainNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(GainNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(GainNode, AudioNode)

GainNode::GainNode(AudioContext* aContext)
  : AudioNode(aContext)
  , mGain(new AudioParam(aContext, 1.0f, 0.0f, 1.0f))
{
}

JSObject*
GainNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return GainNodeBinding::Wrap(aCx, aScope, this);
}

}
}

