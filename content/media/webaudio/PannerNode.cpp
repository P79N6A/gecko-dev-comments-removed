





#include "PannerNode.h"
#include "mozilla/dom/PannerNodeBinding.h"

namespace mozilla {
namespace dom {

PannerNode::PannerNode(AudioContext* aContext)
  : AudioNode(aContext)
  , mPanningModel(PanningModelEnum::HRTF)
  , mDistanceModel(DistanceModelEnum::INVERSE_DISTANCE)
  , mPosition()
  , mOrientation(1.f, 0.f, 0.f)
  , mVelocity()
  , mRefDistance(1.f)
  , mMaxDistance(10000.f)
  , mRolloffFactor(1.f)
  , mConeInnerAngle(360.f)
  , mConeOuterAngle(360.f)
  , mConeOuterGain(0.f)
{
}

JSObject*
PannerNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return PannerNodeBinding::Wrap(aCx, aScope, this);
}

}
}

