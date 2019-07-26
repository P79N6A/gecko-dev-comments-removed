





#include "BiquadFilterNode.h"
#include "mozilla/dom/BiquadFilterNodeBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_3(BiquadFilterNode, AudioNode,
                                     mFrequency, mQ, mGain)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BiquadFilterNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(BiquadFilterNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(BiquadFilterNode, AudioNode)

static float
Nyquist(AudioContext* aContext)
{
  return 0.5f * aContext->SampleRate();
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
BiquadFilterNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return BiquadFilterNodeBinding::Wrap(aCx, aScope, this);
}

}
}

