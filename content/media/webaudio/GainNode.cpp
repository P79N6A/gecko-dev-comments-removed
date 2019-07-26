





#include "GainNode.h"
#include "mozilla/dom/GainNodeBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(GainNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(GainNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGain)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(GainNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(tmp->mGain, AudioParam, "gain value")
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

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
GainNode::WrapObject(JSContext* aCx, JSObject* aScope,
                     bool* aTriedToWrap)
{
  return GainNodeBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

}
}

