





#ifndef PeriodicWave_h_
#define PeriodicWave_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "AudioContext.h"
#include "AudioNodeEngine.h"

namespace mozilla {

namespace dom {

class PeriodicWave final : public nsWrapperCache
{
public:
  PeriodicWave(AudioContext* aContext,
               const float* aRealData,
               const float* aImagData,
               const uint32_t aLength,
               ErrorResult& aRv);

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(PeriodicWave)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(PeriodicWave)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  uint32_t DataLength() const
  {
    return mLength;
  }

  ThreadSharedFloatArrayBufferList* GetThreadSharedBuffer() const
  {
    return mCoefficients;
  }

  size_t SizeOfExcludingThisIfNotShared(MallocSizeOf aMallocSizeOf) const;
  size_t SizeOfIncludingThisIfNotShared(MallocSizeOf aMallocSizeOf) const;

private:
  ~PeriodicWave() {}

  nsRefPtr<AudioContext> mContext;
  nsRefPtr<ThreadSharedFloatArrayBufferList> mCoefficients;
  uint32_t mLength;
};

}
}

#endif
