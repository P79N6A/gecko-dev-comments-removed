





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
                           const float* aImagData,
                           const uint32_t aLength,
                           ErrorResult& aRv)
  : mContext(aContext)
{
  MOZ_ASSERT(aContext);
  SetIsDOMBinding();

  
  MOZ_ASSERT(aLength > 0);
  MOZ_ASSERT(aLength <= 4096);
  mLength = aLength;

  
  mCoefficients = new ThreadSharedFloatArrayBufferList(2);
  float* buffer = static_cast<float*>(malloc(aLength*sizeof(float)*2));
  if (buffer == nullptr) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }
  PodCopy(buffer, aRealData, aLength);
  mCoefficients->SetData(0, buffer, buffer);
  PodCopy(buffer+aLength, aImagData, aLength);
  mCoefficients->SetData(1, nullptr, buffer+aLength);
}

JSObject*
PeriodicWave::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return PeriodicWaveBinding::Wrap(aCx, aScope, this);
}

} 
} 

