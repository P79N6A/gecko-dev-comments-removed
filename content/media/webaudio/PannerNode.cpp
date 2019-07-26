





#include "PannerNode.h"

namespace mozilla {
namespace dom {

PannerNode::PannerNode(AudioContext* aContext)
  : AudioNode(aContext)
  , mPanningModel(PanningModelTypeValues::HRTF)
  , mDistanceModel(DistanceModelTypeValues::Inverse)
  , mPosition()
  , mOrientation(1., 0., 0.)
  , mVelocity()
  , mRefDistance(1.)
  , mMaxDistance(10000.)
  , mRolloffFactor(1.)
  , mConeInnerAngle(360.)
  , mConeOuterAngle(360.)
  , mConeOuterGain(0.)
{
}

JSObject*
PannerNode::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return PannerNodeBinding::Wrap(aCx, aScope, this);
}

}
}

