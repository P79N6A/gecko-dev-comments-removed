





#ifndef AudioParam_h_
#define AudioParam_h_

#include "AudioEventTimeline.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "AudioContext.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/Util.h"

struct JSContext;
class nsIDOMWindow;

namespace mozilla {

class ErrorResult;

namespace dom {

namespace detail {



class FloatArrayWrapper
{
public:
  FloatArrayWrapper() 
  {
  }
  FloatArrayWrapper(const Float32Array& array)
  {
    mArray.construct(array);
  }
  FloatArrayWrapper(const FloatArrayWrapper& rhs)
  {
    MOZ_ASSERT(!rhs.mArray.empty());
    mArray.construct(rhs.mArray.ref());
  }

  FloatArrayWrapper& operator=(const FloatArrayWrapper& rhs)
  {
    MOZ_ASSERT(!rhs.mArray.empty());
    mArray.destroyIfConstructed();
    mArray.construct(rhs.mArray.ref());
    return *this;
  }

  
  float* Data() const
  {
    MOZ_ASSERT(inited());
    return mArray.ref().Data();
  }
  uint32_t Length() const
  {
    MOZ_ASSERT(inited());
    return mArray.ref().Length();
  }
  bool inited() const
  {
    return !mArray.empty();
  }

private:
  Maybe<Float32Array> mArray;
};

}

typedef AudioEventTimeline<detail::FloatArrayWrapper, ErrorResult> AudioParamTimeline;

class AudioParam MOZ_FINAL : public nsWrapperCache,
                             public EnableWebAudioCheck,
                             public AudioParamTimeline
{
public:
  AudioParam(AudioContext* aContext,
             float aDefaultValue,
             float aMinValue,
             float aMaxValue);
  virtual ~AudioParam();

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AudioParam)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AudioParam)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope,
                               bool* aTriedToWrap);

  
  
  void SetValueCurveAtTime(JSContext* cx, const Float32Array& aValues, double aStartTime, double aDuration, ErrorResult& aRv)
  {
    AudioParamTimeline::SetValueCurveAtTime(detail::FloatArrayWrapper(aValues),
                                            aStartTime, aDuration, aRv);
  }

  float MinValue() const
  {
    return mMinValue;
  }

  float MaxValue() const
  {
    return mMaxValue;
  }

  float DefaultValue() const
  {
    return mDefaultValue;
  }

private:
  nsRefPtr<AudioContext> mContext;
  const float mDefaultValue;
  const float mMinValue;
  const float mMaxValue;
};

}
}

#endif

