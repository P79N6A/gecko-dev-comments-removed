





#include "BiquadFilterNode.h"
#include "mozilla/dom/BiquadFilterNodeBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(BiquadFilterNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BiquadFilterNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFrequency)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mQ)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGain)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BiquadFilterNode, AudioNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(tmp->mFrequency, AudioParam, "frequency value")
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(tmp->mQ, AudioParam, "Q value")
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(tmp->mGain, AudioParam, "gain value")
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BiquadFilterNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(BiquadFilterNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(BiquadFilterNode, AudioNode)

static float
Nyquist(AudioContext* aContext)
{
  
  
  return 0.5f * 44100;
}

BiquadFilterNode::BiquadFilterNode(AudioContext* aContext)
  : AudioNode(aContext)
  , mType(BiquadTypeEnum::LOWPASS)
  , mFrequency(new AudioParam(aContext, 350.f, 10.f, Nyquist(aContext)))
  , mQ(new AudioParam(aContext, 1.f, 0.0001f, 1000.f))
  , mGain(new AudioParam(aContext, 0.f, -40.f, 40.f))
{
}

JSObject*
BiquadFilterNode::WrapObject(JSContext* aCx, JSObject* aScope,
                             bool* aTriedToWrap)
{
  return BiquadFilterNodeBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

}
}

