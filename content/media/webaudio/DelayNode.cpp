





#include "DelayNode.h"
#include "mozilla/dom/DelayNodeBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(DelayNode, AudioNode,
                                     mDelay)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DelayNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(DelayNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(DelayNode, AudioNode)

DelayNode::DelayNode(AudioContext* aContext, double aMaxDelay)
  : AudioNode(aContext)
  , mDelay(new AudioParam(aContext, 0.0f, 0.0f, float(aMaxDelay)))
{
}

JSObject*
DelayNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return DelayNodeBinding::Wrap(aCx, aScope, this);
}

}
}

