




#ifndef mozilla_dom_Animation_h
#define mozilla_dom_Animation_h

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCSSPseudoElements.h"
#include "nsIDocument.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"
#include "mozilla/StickyTimeDuration.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/TimeStamp.h"
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
};





struct ComputedTiming
{
  ComputedTiming()
    : mTimeFraction(kNullTimeFraction)
    , mCurrentIteration(0)
    , mPhase(AnimationPhase_Null)
  { }

  static const double kNullTimeFraction;

  
  
  
  StickyTimeDuration mActiveDuration;

  
  
  double mTimeFraction;

  
  
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

struct ElementPropertyTransition;

namespace dom {

class AnimationEffect;

class Animation : public nsWrapperCache
{
public:
  Animation(nsIDocument* aDocument,
            Element* aTarget,
            nsCSSPseudoElements::Type aPseudoType,
            const AnimationTiming &aTiming,
            const nsSubstring& aName)
    : mDocument(aDocument)
    , mTarget(aTarget)
    , mTiming(aTiming)
    , mName(aName)
    , mIsFinishedTransition(false)
    , mLastNotification(LAST_NOTIFICATION_NONE)
    , mPseudoType(aPseudoType)
  {
    MOZ_ASSERT(aTarget, "null animation target is not yet supported");
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(Animation)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(Animation)

  nsIDocument* GetParentObject() const { return mDocument; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  
  
  virtual ElementPropertyTransition* AsTransition() { return nullptr; }
  virtual const ElementPropertyTransition* AsTransition() const {
    return nullptr;
  }

  
  
  
  already_AddRefed<AnimationEffect> GetEffect();
  Element* GetTarget() const {
    
    
    
    MOZ_ASSERT(mPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement,
               "Requesting the target of an Animation that targets a"
               " pseudo-element is not yet supported.");
    return mTarget;
  }

  void SetParentTime(Nullable<TimeDuration> aParentTime);

  const AnimationTiming& Timing() const {
    return mTiming;
  }
  AnimationTiming& Timing() {
    return mTiming;
  }

  const nsString& Name() const {
    return mName;
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

  void SetIsFinishedTransition() {
    MOZ_ASSERT(AsTransition(),
               "Calling SetIsFinishedTransition but it's not a transition");
    mIsFinishedTransition = true;
  }

  bool IsCurrent() const;
  bool IsInEffect() const;

  enum {
    LAST_NOTIFICATION_NONE = uint64_t(-1),
    LAST_NOTIFICATION_END = uint64_t(-2)
  };
  uint64_t LastNotification() const { return mLastNotification; }
  void SetLastNotification(uint64_t aLastNotification) {
    mLastNotification = aLastNotification;
  }

  bool HasAnimationOfProperty(nsCSSProperty aProperty) const;
  const InfallibleTArray<AnimationProperty>& Properties() const {
    return mProperties;
  }
  InfallibleTArray<AnimationProperty>& Properties() {
    return mProperties;
  }

  
  
  
  
  void ComposeStyle(nsRefPtr<css::AnimValuesStyleRule>& aStyleRule,
                    nsCSSPropertySet& aSetProperties);

protected:
  virtual ~Animation() { }

  
  
  nsCOMPtr<nsIDocument> mDocument;
  nsCOMPtr<Element> mTarget;
  Nullable<TimeDuration> mParentTime;

  AnimationTiming mTiming;
  nsString mName;
  
  
  bool mIsFinishedTransition;
  
  
  uint64_t mLastNotification;
  nsCSSPseudoElements::Type mPseudoType;

  InfallibleTArray<AnimationProperty> mProperties;
};

} 
} 

#endif 
