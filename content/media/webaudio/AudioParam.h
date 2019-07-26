





#ifndef AudioParam_h_
#define AudioParam_h_

#include "AudioParamTimeline.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "AudioNode.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/Util.h"
#include "WebAudioUtils.h"

struct JSContext;
class nsIDOMWindow;

namespace mozilla {

namespace dom {

class AudioParam MOZ_FINAL : public nsWrapperCache,
                             public EnableWebAudioCheck,
                             public AudioParamTimeline
{
public:
  typedef void (*CallbackType)(AudioNode*);

  AudioParam(AudioNode* aNode,
             CallbackType aCallback,
             float aDefaultValue,
             float aMinValue,
             float aMaxValue);
  virtual ~AudioParam();

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AudioParam)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AudioParam)

  AudioContext* GetParentObject() const
  {
    return mNode->Context();
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  
  
  void SetValueCurveAtTime(JSContext* cx, const Float32Array& aValues, double aStartTime, double aDuration, ErrorResult& aRv)
  {
    AudioParamTimeline::SetValueCurveAtTime(aValues.Data(), aValues.Length(),
                                            aStartTime, aDuration, aRv);
    mCallback(mNode);
  }

  
  
  void SetValue(float aValue)
  {
    
    if (HasSimpleValue() &&
        WebAudioUtils::FuzzyEqual(GetValue(), aValue)) {
      return;
    }
    AudioParamTimeline::SetValue(aValue);
    mCallback(mNode);
  }
  void SetValueAtTime(float aValue, double aStartTime, ErrorResult& aRv)
  {
    AudioParamTimeline::SetValueAtTime(aValue, aStartTime, aRv);
    mCallback(mNode);
  }
  void LinearRampToValueAtTime(float aValue, double aEndTime, ErrorResult& aRv)
  {
    AudioParamTimeline::LinearRampToValueAtTime(aValue, aEndTime, aRv);
    mCallback(mNode);
  }
  void ExponentialRampToValueAtTime(float aValue, double aEndTime, ErrorResult& aRv)
  {
    AudioParamTimeline::ExponentialRampToValueAtTime(aValue, aEndTime, aRv);
    mCallback(mNode);
  }
  void SetTargetAtTime(float aTarget, double aStartTime, double aTimeConstant, ErrorResult& aRv)
  {
    AudioParamTimeline::SetTargetAtTime(aTarget, aStartTime, aTimeConstant, aRv);
    mCallback(mNode);
  }
  void CancelScheduledValues(double aStartTime)
  {
    AudioParamTimeline::CancelScheduledValues(aStartTime);
    mCallback(mNode);
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
  nsRefPtr<AudioNode> mNode;
  CallbackType mCallback;
  const float mDefaultValue;
  const float mMinValue;
  const float mMaxValue;
};

}
}

#endif

