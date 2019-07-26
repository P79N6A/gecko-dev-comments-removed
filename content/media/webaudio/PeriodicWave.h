





#ifndef PeriodicWave_h_
#define PeriodicWave_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "EnableWebAudioCheck.h"
#include "AudioContext.h"
#include "nsAutoPtr.h"

namespace mozilla {

namespace dom {

class PeriodicWave MOZ_FINAL : public nsWrapperCache,
                               public EnableWebAudioCheck
{
public:
  PeriodicWave(AudioContext* aContext,
               const float* aRealData,
               uint32_t aRealDataLength,
               const float* aImagData,
               uint32_t aImagDataLength);

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(PeriodicWave)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(PeriodicWave)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

private:
  nsRefPtr<AudioContext> mContext;
};

}
}

#endif

