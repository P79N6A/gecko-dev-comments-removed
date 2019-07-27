





#ifndef mozilla_dom_KeyframeEffect_h
#define mozilla_dom_KeyframeEffect_h

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCSSPseudoElements.h"
#include "nsIDocument.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"
#include "mozilla/StickyTimeDuration.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/dom/AnimationEffectReadOnly.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Nullable.h"
#include "nsSMILKeySpline.h"
#include "nsStyleStruct.h" 

struct JSContext;
class nsCSSPropertySet;

namespace mozilla {
namespace css {
class AnimValuesStyleRule;
} 








struct AnimationTiming
{
  TimeDuration mIterationDuration;
  TimeDuration mDelay;
  float mIterationCount; 
  uint8_t mDirection;
  uint8_t mFillMode;

  bool FillsForwards() const {
    return mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BOTH ||
           mFillMode == NS_STYLE_ANIMATION_FILL_MODE_FORWARDS;
  }
  bool FillsBackwards() const {
    return mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BOTH ||
           mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BACKWARDS;
  }
  bool operator==(const AnimationTiming& aOther) const {
    return mIterationDuration == aOther.mIterationDuration &&
           mDelay == aOther.mDelay &&
           mIterationCount == aOther.mIterationCount &&
           mDirection == aOther.mDirection &&
           mFillMode == aOther.mFillMode;
  }
  bool operator!=(const AnimationTiming& aOther) const {
    return !(*this == aOther);
  }
};





struct ComputedTiming
{
  ComputedTiming()
    : mProgress(kNullProgress)
    , mCurrentIteration(0)
    , mPhase(AnimationPhase_Null)
  { }

  static const double kNullProgress;

  
  
  
  StickyTimeDuration mActiveDuration;

  
  
  
  
  double mProgress;

  
  uint64_t mCurrentIteration;

  enum {
    
    AnimationPhase_Null,
    
    AnimationPhase_Before,
    
    AnimationPhase_Active,
    
    AnimationPhase_After
  } mPhase;
};

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
  bool operator==(const ComputedTimingFunction& aOther) const {
    return mType == aOther.mType &&
           (mType == nsTimingFunction::Function ?
            mTimingFunction == aOther.mTimingFunction :
            mSteps == aOther.mSteps);
  }
  bool operator!=(const ComputedTimingFunction& aOther) const {
    return !(*this == aOther);
  }

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

  bool operator==(const AnimationPropertySegment& aOther) const {
    return mFromKey == aOther.mFromKey &&
           mToKey == aOther.mToKey &&
           mFromValue == aOther.mFromValue &&
           mToValue == aOther.mToValue &&
           mTimingFunction == aOther.mTimingFunction;
  }
  bool operator!=(const AnimationPropertySegment& aOther) const {
    return !(*this == aOther);
  }
};

struct AnimationProperty
{
  nsCSSProperty mProperty;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mWinsInCascade;

  InfallibleTArray<AnimationPropertySegment> mSegments;

  bool operator==(const AnimationProperty& aOther) const {
    return mProperty == aOther.mProperty &&
           mWinsInCascade == aOther.mWinsInCascade &&
           mSegments == aOther.mSegments;
  }
  bool operator!=(const AnimationProperty& aOther) const {
    return !(*this == aOther);
  }
};

struct ElementPropertyTransition;

namespace dom {

class KeyframeEffectReadOnly : public AnimationEffectReadOnly
{
public:
  KeyframeEffectReadOnly(nsIDocument* aDocument,
                         Element* aTarget,
                         nsCSSPseudoElements::Type aPseudoType,
                         const AnimationTiming &aTiming)
    : AnimationEffectReadOnly(aDocument)
    , mTarget(aTarget)
    , mTiming(aTiming)
    , mIsFinishedTransition(false)
    , mPseudoType(aPseudoType)
  {
    MOZ_ASSERT(aTarget, "null animation target is not yet supported");
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(KeyframeEffectReadOnly,
                                                        AnimationEffectReadOnly)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  virtual ElementPropertyTransition* AsTransition() { return nullptr; }
  virtual const ElementPropertyTransition* AsTransition() const {
    return nullptr;
  }

  
  Element* GetTarget() const {
    
    
    
    MOZ_ASSERT(mPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement,
               "Requesting the target of a KeyframeEffect that targets a"
               " pseudo-element is not yet supported.");
    return mTarget;
  }

  
  
  void GetTarget(Element*& aTarget,
                 nsCSSPseudoElements::Type& aPseudoType) const {
    aTarget = mTarget;
    aPseudoType = mPseudoType;
  }

  void SetParentTime(Nullable<TimeDuration> aParentTime);

  const AnimationTiming& Timing() const {
    return mTiming;
  }
  AnimationTiming& Timing() {
    return mTiming;
  }

  
  
  
  
  TimeDuration InitialAdvance() const {
    return std::max(TimeDuration(), mTiming.mDelay * -1);
  }

  Nullable<TimeDuration> GetLocalTime() const {
    
    
    return mParentTime;
  }

  
  
  
  
  
  
  
  
  
  
  static ComputedTiming
  GetComputedTimingAt(const Nullable<TimeDuration>& aLocalTime,
                      const AnimationTiming& aTiming);

  
  
  ComputedTiming GetComputedTiming(const AnimationTiming* aTiming
                                     = nullptr) const {
    return GetComputedTimingAt(GetLocalTime(), aTiming ? *aTiming : mTiming);
  }

  
  static StickyTimeDuration
  ActiveDuration(const AnimationTiming& aTiming);

  
  
  
  
  bool IsFinishedTransition() const {
    return mIsFinishedTransition;
  }

  void SetIsFinishedTransition(bool aIsFinished) {
    MOZ_ASSERT(AsTransition(),
               "Calling SetIsFinishedTransition but it's not a transition");
    mIsFinishedTransition = aIsFinished;
  }

  bool IsInPlay(const Animation& aAnimation) const;
  bool IsCurrent(const Animation& aAnimation) const;
  bool IsInEffect() const;

  const AnimationProperty*
  GetAnimationOfProperty(nsCSSProperty aProperty) const;
  bool HasAnimationOfProperty(nsCSSProperty aProperty) const {
    return GetAnimationOfProperty(aProperty) != nullptr;
  }
  bool HasAnimationOfProperties(const nsCSSProperty* aProperties,
                                size_t aPropertyCount) const;
  const InfallibleTArray<AnimationProperty>& Properties() const {
    return mProperties;
  }
  InfallibleTArray<AnimationProperty>& Properties() {
    return mProperties;
  }

  
  
  
  
  void ComposeStyle(nsRefPtr<css::AnimValuesStyleRule>& aStyleRule,
                    nsCSSPropertySet& aSetProperties);

protected:
  virtual ~KeyframeEffectReadOnly() { }

  nsCOMPtr<Element> mTarget;
  Nullable<TimeDuration> mParentTime;

  AnimationTiming mTiming;
  
  
  bool mIsFinishedTransition;
  nsCSSPseudoElements::Type mPseudoType;

  InfallibleTArray<AnimationProperty> mProperties;
};

} 
} 

#endif 
