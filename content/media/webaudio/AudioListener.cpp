





#include "AudioListener.h"
#include "AudioContext.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/AudioListenerBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(AudioListener, mContext)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AudioListener, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AudioListener, Release)

AudioListener::AudioListener(AudioContext* aContext)
  : mContext(aContext)
  , mPosition()
  , mOrientation(0., 0., -1.)
  , mUpVector(0., 1., 0.)
  , mVelocity()
  , mDopplerFactor(1.)
  , mSpeedOfSound(343.3) 
{
  MOZ_ASSERT(aContext);
  SetIsDOMBinding();
}

JSObject*
AudioListener::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return AudioListenerBinding::Wrap(aCx, aScope, this);
}

void
AudioListener::RegisterPannerNode(PannerNode* aPannerNode)
{
  mPanners.AppendElement(aPannerNode->asWeakPtr());

  
  aPannerNode->SendThreeDPointParameterToStream(PannerNode::LISTENER_POSITION, mPosition);
  aPannerNode->SendThreeDPointParameterToStream(PannerNode::LISTENER_ORIENTATION, mOrientation);
  aPannerNode->SendThreeDPointParameterToStream(PannerNode::LISTENER_UPVECTOR, mUpVector);
  aPannerNode->SendThreeDPointParameterToStream(PannerNode::LISTENER_VELOCITY, mVelocity);
  aPannerNode->SendDoubleParameterToStream(PannerNode::LISTENER_DOPPLER_FACTOR, mDopplerFactor);
  aPannerNode->SendDoubleParameterToStream(PannerNode::LISTENER_SPEED_OF_SOUND, mSpeedOfSound);
  UpdatePannersVelocity();
}

void AudioListener::UnregisterPannerNode(PannerNode* aPannerNode)
{
  mPanners.RemoveElement(aPannerNode);
}

void
AudioListener::SendDoubleParameterToStream(uint32_t aIndex, double aValue)
{
  for (uint32_t i = 0; i < mPanners.Length(); ++i) {
    if (mPanners[i]) {
      mPanners[i]->SendDoubleParameterToStream(aIndex, aValue);
    }
  }
}

void
AudioListener::SendThreeDPointParameterToStream(uint32_t aIndex, const ThreeDPoint& aValue)
{
  for (uint32_t i = 0; i < mPanners.Length(); ++i) {
    if (mPanners[i]) {
      mPanners[i]->SendThreeDPointParameterToStream(aIndex, aValue);
    }
  }
}

void AudioListener::UpdatePannersVelocity()
{
  for (uint32_t i = 0; i < mPanners.Length(); ++i) {
    if (mPanners[i]) {
      mPanners[i]->SendDopplerToSourcesIfNeeded();
    }
  }
}

}
}

