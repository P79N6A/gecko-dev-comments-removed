





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
  , mOrientation(0.f, 0.f, -1.f)
  , mUpVector(0.f, 1.f, 0.f)
  , mVelocity()
  , mDopplerFactor(1.f)
  , mSpeedOfSound(343.3f) 
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

