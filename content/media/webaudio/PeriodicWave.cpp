





#include "PeriodicWave.h"
#include "AudioContext.h"
#include "mozilla/dom/PeriodicWaveBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(PeriodicWave, mContext)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(PeriodicWave, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(PeriodicWave, Release)

PeriodicWave::PeriodicWave(AudioContext* aContext,
                           const float* aRealData,
                           uint32_t aRealDataLength,
                           const float* aImagData,
                           uint32_t aImagDataLength)
  : mContext(aContext)
{
  MOZ_ASSERT(aContext);
  SetIsDOMBinding();
}

JSObject*
PeriodicWave::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return PeriodicWaveBinding::Wrap(aCx, aScope, this);
}

}
}

