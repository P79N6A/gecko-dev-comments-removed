





#include "PeriodicWave.h"
#include "AudioContext.h"
#include "mozilla/dom/PeriodicWaveBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(PeriodicWave, mContext)

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
  mCoefficients->SetData(0, buffer, free, buffer);
  PodCopy(buffer+aLength, aImagData, aLength);
  mCoefficients->SetData(1, nullptr, free, buffer+aLength);
}

size_t
PeriodicWave::SizeOfExcludingThisIfNotShared(MallocSizeOf aMallocSizeOf) const
{
  
  
  size_t amount = 0;
  if (!mCoefficients->IsShared()) {
    amount += mCoefficients->SizeOfIncludingThis(aMallocSizeOf);
  }

  return amount;
}

size_t
PeriodicWave::SizeOfIncludingThisIfNotShared(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThisIfNotShared(aMallocSizeOf);
}

JSObject*
PeriodicWave::WrapObject(JSContext* aCx)
{
  return PeriodicWaveBinding::Wrap(aCx, this);
}

} 
} 

