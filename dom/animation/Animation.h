




#ifndef mozilla_dom_Animation_h
#define mozilla_dom_Animation_h

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDocument.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/Nullable.h"
#include "nsSMILKeySpline.h"
#include "nsStyleStruct.h" 

struct JSContext;

namespace mozilla {

class ComputedTimingFunction
{
public:
  typedef nsTimingFunction::Type Type;
  void Init(const nsTimingFunction &aFunction);
  double GetValue(double aPortion) const;
  const nsSMILKeySpline* GetFunction() const {
    NS_ASSERTION(mType == nsTimingFunction::Function, "Type mismatch");
    return &mTimingFunction;
  }
  Type GetType() const { return mType; }
  uint32_t GetSteps() const { return mSteps; }

private:
  Type mType;
  nsSMILKeySpline mTimingFunction;
  uint32_t mSteps;
};

struct AnimationPropertySegment
{
  float mFromKey, mToKey;
  StyleAnimationValue mFromValue, mToValue;
  ComputedTimingFunction mTimingFunction;
};

struct AnimationProperty
{
  nsCSSProperty mProperty;
  InfallibleTArray<AnimationPropertySegment> mSegments;
};

namespace dom {

class Animation MOZ_FINAL : public nsWrapperCache
{
public:
  explicit Animation(nsIDocument* aDocument)
    : mDocument(aDocument)
  {
    SetIsDOMBinding();
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(Animation)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(Animation)

  nsIDocument* GetParentObject() const { return mDocument; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void SetParentTime(Nullable<TimeDuration> aParentTime);

  bool HasAnimationOfProperty(nsCSSProperty aProperty) const;
  const InfallibleTArray<AnimationProperty>& Properties() const {
    return mProperties;
  }
  InfallibleTArray<AnimationProperty>& Properties() {
    return mProperties;
  }

protected:
  virtual ~Animation() { }

  
  
  nsRefPtr<nsIDocument> mDocument;
  Nullable<TimeDuration> mParentTime;

  InfallibleTArray<AnimationProperty> mProperties;
};

} 
} 

#endif 
