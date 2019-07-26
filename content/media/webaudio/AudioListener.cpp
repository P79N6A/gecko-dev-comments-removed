





#include "AudioListener.h"
#include "AudioContext.h"
#include "nsContentUtils.h"
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
AudioListener::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return AudioListenerBinding::Wrap(aCx, aScope, this);
}

}
}

