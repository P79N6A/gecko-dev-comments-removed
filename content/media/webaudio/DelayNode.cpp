





#include "DelayNode.h"
#include "mozilla/dom/DelayNodeBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(DelayNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DelayNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDelay)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DelayNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDelay)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DelayNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(DelayNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(DelayNode, AudioNode)

DelayNode::DelayNode(AudioContext* aContext, float aMaxDelay)
  : AudioNode(aContext)
  , mDelay(new AudioParam(aContext, 0.0f, 0.0f, aMaxDelay))
{
}

JSObject*
DelayNode::WrapObject(JSContext* aCx, JSObject* aScope,
                      bool* aTriedToWrap)
{
  return DelayNodeBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

}
}

