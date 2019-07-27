




#ifndef mozilla_css_AnimationCommon_h
#define mozilla_css_AnimationCommon_h

#include "nsIStyleRuleProcessor.h"
#include "nsIStyleRule.h"
#include "nsRefreshDriver.h"
#include "prclist.h"
#include "nsCSSProperty.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/StyleAnimationValue.h"
#include "mozilla/dom/AnimationTimeline.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Nullable.h"
#include "nsSMILKeySpline.h"
#include "nsStyleStruct.h"
#include "mozilla/Attributes.h"
#include "mozilla/FloatingPoint.h"
#include "nsCSSPseudoElements.h"
#include "nsCycleCollectionParticipant.h"

class nsIFrame;
class nsPresContext;
class nsStyleChangeList;


#ifdef CurrentTime
#undef CurrentTime
#endif

namespace mozilla {

class RestyleTracker;
class StyleAnimationValue;
struct ElementPropertyTransition;
struct ElementAnimationCollection;

namespace css {

bool IsGeometricProperty(nsCSSProperty aProperty);

class CommonAnimationManager : public nsIStyleRuleProcessor,
                               public nsARefreshObserver {
public:
  CommonAnimationManager(nsPresContext *aPresContext);

  
  NS_DECL_ISUPPORTS

  
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) MOZ_OVERRIDE;
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  


  void Disconnect();

  
  
  
  void AddStyleUpdatesTo(mozilla::RestyleTracker& aTracker);

  enum FlushFlags {
    Can_Throttle,
    Cannot_Throttle
  };

  static bool ExtractComputedValueForTransition(
                  nsCSSProperty aProperty,
                  nsStyleContext* aStyleContext,
                  mozilla::StyleAnimationValue& aComputedValue);
protected:
  virtual ~CommonAnimationManager();

  
  friend struct mozilla::ElementAnimationCollection;

  virtual void
  AddElementCollection(ElementAnimationCollection* aCollection) = 0;
  virtual void ElementCollectionRemoved() = 0;
  void RemoveAllElementCollections();

  
  
  static ElementAnimationCollection*
  GetAnimationsForCompositor(nsIContent* aContent,
                             nsIAtom* aElementProperty,
                             nsCSSProperty aProperty);

  
  
  
  nsStyleContext* UpdateThrottledStyle(mozilla::dom::Element* aElement,
                                       nsStyleContext* aParentStyle,
                                       nsStyleChangeList &aChangeList);
  
  already_AddRefed<nsStyleContext> ReparentContent(nsIContent* aContent,
                                                  nsStyleContext* aParentStyle);
  
  static void ReparentBeforeAndAfter(dom::Element* aElement,
                                     nsIFrame* aPrimaryFrame,
                                     nsStyleContext* aNewStyle,
                                     nsStyleSet* aStyleSet);

  PRCList mElementCollections;
  nsPresContext *mPresContext; 
};



#define IMPL_UPDATE_ALL_THROTTLED_STYLES_INTERNAL(class_, animations_getter_)  \
void                                                                           \
class_::UpdateAllThrottledStylesInternal()                                     \
{                                                                              \
  TimeStamp now = mPresContext->RefreshDriver()->MostRecentRefresh();          \
                                                                               \
  nsStyleChangeList changeList;                                                \
                                                                               \
  /* update each transitioning element by finding its root-most ancestor
     with a transition, and flushing the style on that ancestor and all
     its descendants*/                                                         \
  PRCList *next = PR_LIST_HEAD(&mElementCollections);                          \
  while (next != &mElementCollections) {                                       \
    ElementAnimationCollection* collection =                                   \
      static_cast<ElementAnimationCollection*>(next);                          \
    next = PR_NEXT_LINK(next);                                                 \
                                                                               \
    if (collection->mFlushGeneration == now) {                                 \
                                     \
      continue;                                                                \
    }                                                                          \
                                                                               \
    

                         \
    dom::Element* element = collection->mElement;                              \
                                                 \
    nsTArray<dom::Element*> ancestors;                                         \
    do {                                                                       \
      ancestors.AppendElement(element);                                        \
    } while ((element = element->GetParentElement()));                         \
                                                                               \
    \
    for (int32_t i = ancestors.Length() - 1; i >= 0; --i) {                    \
      if (animations_getter_(ancestors[i],                                     \
                            nsCSSPseudoElements::ePseudo_NotPseudoElement,     \
                            false)) {                                          \
        element = ancestors[i];                                                \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
                                                                               \
    nsIFrame* primaryFrame;                                                    \
    if (element &&                                                             \
        (primaryFrame = nsLayoutUtils::GetStyleFrame(element))) {              \
      UpdateThrottledStylesForSubtree(element,                                 \
        primaryFrame->StyleContext()->GetParent(), changeList);                \
    }                                                                          \
  }                                                                            \
                                                                               \
  RestyleManager* restyleManager = mPresContext->RestyleManager();             \
  restyleManager->ProcessRestyledFrames(changeList);                           \
  restyleManager->FlushOverflowChangedTracker();                               \
}




class AnimValuesStyleRule MOZ_FINAL : public nsIStyleRule
{
public:
  
  NS_DECL_ISUPPORTS

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData) MOZ_OVERRIDE;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  void AddValue(nsCSSProperty aProperty,
                mozilla::StyleAnimationValue &aStartValue)
  {
    PropertyValuePair v = { aProperty, aStartValue };
    mPropertyValuePairs.AppendElement(v);
  }

  
  mozilla::StyleAnimationValue* AddEmptyValue(nsCSSProperty aProperty)
  {
    PropertyValuePair *p = mPropertyValuePairs.AppendElement();
    p->mProperty = aProperty;
    return &p->mValue;
  }

  struct PropertyValuePair {
    nsCSSProperty mProperty;
    mozilla::StyleAnimationValue mValue;
  };

private:
  ~AnimValuesStyleRule() {}

  InfallibleTArray<PropertyValuePair> mPropertyValuePairs;
};

class ComputedTimingFunction {
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

} 

struct AnimationPropertySegment
{
  float mFromKey, mToKey;
  mozilla::StyleAnimationValue mFromValue, mToValue;
  mozilla::css::ComputedTimingFunction mTimingFunction;
};

struct AnimationProperty
{
  nsCSSProperty mProperty;
  InfallibleTArray<AnimationPropertySegment> mSegments;
};








struct AnimationTiming
{
  mozilla::TimeDuration mIterationDuration;
  mozilla::TimeDuration mDelay;
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

  
  
  TimeDuration mActiveDuration;

  
  
  double mTimeFraction;

  
  
  uint64_t mCurrentIteration;

  enum {
    
    AnimationPhase_Null,
    
    AnimationPhase_Before,
    
    AnimationPhase_Active,
    
    AnimationPhase_After
  } mPhase;
};





class ElementAnimation : public nsWrapperCache
{
protected:
  virtual ~ElementAnimation() { }

public:
  explicit ElementAnimation(dom::AnimationTimeline* aTimeline)
    : mIsRunningOnCompositor(false)
    , mIsFinishedTransition(false)
    , mLastNotification(LAST_NOTIFICATION_NONE)
    , mTimeline(aTimeline)
  {
    SetIsDOMBinding();
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(ElementAnimation)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(ElementAnimation)

  dom::AnimationTimeline* GetParentObject() const { return mTimeline; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  dom::AnimationTimeline* Timeline() const { return mTimeline; }
  double StartTime() const;
  double CurrentTime() const;

  
  
  
  virtual ElementPropertyTransition* AsTransition() { return nullptr; }
  virtual const ElementPropertyTransition* AsTransition() const {
    return nullptr;
  }

  bool IsPaused() const {
    return mPlayState == NS_STYLE_ANIMATION_PLAY_STATE_PAUSED;
  }

  
  
  
  bool IsFinishedTransition() const {
    return mIsFinishedTransition;
  }
  void SetFinishedTransition() {
    MOZ_ASSERT(AsTransition(),
               "Calling SetFinishedTransition but it's not a transition");
    mIsFinishedTransition = true;
  }

  bool HasAnimationOfProperty(nsCSSProperty aProperty) const;
  bool IsRunning() const;
  bool IsCurrent() const;

  
  
  
  
  
  Nullable<mozilla::TimeDuration> GetLocalTime() const {
    const mozilla::TimeStamp& timelineTime = mTimeline->GetCurrentTimeStamp();
    
    
    MOZ_ASSERT(timelineTime.IsNull() || !IsPaused() ||
               timelineTime >= mPauseStart,
               "if paused, any non-null value of aTime must be at least"
               " mPauseStart");

    Nullable<mozilla::TimeDuration> result; 
    if (!timelineTime.IsNull() && !mStartTime.IsNull()) {
      result.SetValue((IsPaused() ? mPauseStart : timelineTime) - mStartTime);
    }
    return result;
  }

  
  
  
  
  mozilla::TimeDuration InitialAdvance() const {
    return std::max(TimeDuration(), mTiming.mDelay * -1);
  }

  
  
  
  
  
  
  
  
  
  
  static ComputedTiming
  GetComputedTimingAt(const Nullable<mozilla::TimeDuration>& aLocalTime,
                      const AnimationTiming& aTiming);

  
  
  ComputedTiming GetComputedTiming(const AnimationTiming& aTiming) const {
    return GetComputedTimingAt(GetLocalTime(), aTiming);
  }

  
  static mozilla::TimeDuration ActiveDuration(const AnimationTiming& aTiming);

  nsString mName;
  AnimationTiming mTiming;
  
  mozilla::TimeStamp mStartTime;
  mozilla::TimeStamp mPauseStart;
  uint8_t mPlayState;
  bool mIsRunningOnCompositor;
  
  
  bool mIsFinishedTransition;

  enum {
    LAST_NOTIFICATION_NONE = uint64_t(-1),
    LAST_NOTIFICATION_END = uint64_t(-2)
  };
  
  
  uint64_t mLastNotification;

  InfallibleTArray<AnimationProperty> mProperties;

  nsRefPtr<dom::AnimationTimeline> mTimeline;
};

typedef InfallibleTArray<nsRefPtr<ElementAnimation> > ElementAnimationPtrArray;

enum EnsureStyleRuleFlags {
  EnsureStyleRule_IsThrottled,
  EnsureStyleRule_IsNotThrottled
};

struct ElementAnimationCollection : public PRCList
{
  ElementAnimationCollection(dom::Element *aElement, nsIAtom *aElementProperty,
                             mozilla::css::CommonAnimationManager *aManager,
                             TimeStamp aNow)
    : mElement(aElement)
    , mElementProperty(aElementProperty)
    , mManager(aManager)
    , mAnimationGeneration(0)
    , mFlushGeneration(aNow)
    , mNeedsRefreshes(true)
#ifdef DEBUG
    , mCalledPropertyDtor(false)
#endif
  {
    MOZ_COUNT_CTOR(ElementAnimationCollection);
    PR_INIT_CLIST(this);
  }
  ~ElementAnimationCollection()
  {
    NS_ABORT_IF_FALSE(mCalledPropertyDtor,
                      "must call destructor through element property dtor");
    MOZ_COUNT_DTOR(ElementAnimationCollection);
    PR_REMOVE_LINK(this);
    mManager->ElementCollectionRemoved();
  }

  void Destroy()
  {
    
    mElement->DeleteProperty(mElementProperty);
  }

  static void PropertyDtor(void *aObject, nsIAtom *aPropertyName,
                           void *aPropertyValue, void *aData);

  
  
  
  
  void EnsureStyleRuleFor(TimeStamp aRefreshTime, EnsureStyleRuleFlags aFlags);

  bool CanThrottleTransformChanges(mozilla::TimeStamp aTime);

  bool CanThrottleAnimation(mozilla::TimeStamp aTime);

  enum CanAnimateFlags {
    
    CanAnimate_HasGeometricProperty = 1,
    
    
    CanAnimate_AllowPartial = 2
  };

  static bool
  CanAnimatePropertyOnCompositor(const dom::Element *aElement,
                                 nsCSSProperty aProperty,
                                 CanAnimateFlags aFlags);

  static bool IsCompositorAnimationDisabledForFrame(nsIFrame* aFrame);

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool CanPerformOnCompositorThread(CanAnimateFlags aFlags) const;
  bool HasAnimationOfProperty(nsCSSProperty aProperty) const;

  bool IsForElement() const { 
    return mElementProperty == nsGkAtoms::animationsProperty ||
           mElementProperty == nsGkAtoms::transitionsProperty;
  }

  bool IsForTransitions() const {
    return mElementProperty == nsGkAtoms::transitionsProperty ||
           mElementProperty == nsGkAtoms::transitionsOfBeforeProperty ||
           mElementProperty == nsGkAtoms::transitionsOfAfterProperty;
  }

  bool IsForAnimations() const {
    return mElementProperty == nsGkAtoms::animationsProperty ||
           mElementProperty == nsGkAtoms::animationsOfBeforeProperty ||
           mElementProperty == nsGkAtoms::animationsOfAfterProperty;
  }

  nsString PseudoElement()
  {
    if (IsForElement()) {
      return EmptyString();
    } else if (mElementProperty == nsGkAtoms::animationsOfBeforeProperty ||
               mElementProperty == nsGkAtoms::transitionsOfBeforeProperty) {
      return NS_LITERAL_STRING("::before");
    } else {
      return NS_LITERAL_STRING("::after");
    }
  }

  void PostRestyleForAnimation(nsPresContext *aPresContext) {
    nsRestyleHint styleHint = IsForElement() ? eRestyle_Self : eRestyle_Subtree;
    aPresContext->PresShell()->RestyleForAnimation(mElement, styleHint);
  }

  static void LogAsyncAnimationFailure(nsCString& aMessage,
                                       const nsIContent* aContent = nullptr);

  dom::Element *mElement;

  
  
  nsIAtom *mElementProperty;

  mozilla::css::CommonAnimationManager *mManager;

  mozilla::ElementAnimationPtrArray mAnimations;

  
  
  
  
  
  
  
  nsRefPtr<mozilla::css::AnimValuesStyleRule> mStyleRule;

  
  
  
  
  
  
  uint64_t mAnimationGeneration;
  
  void UpdateAnimationGeneration(nsPresContext* aPresContext);

  
  
  bool HasCurrentAnimations();

  
  TimeStamp mStyleRuleRefreshTime;

  
  
  
  TimeStamp mFlushGeneration;

  
  
  
  bool mNeedsRefreshes;

#ifdef DEBUG
  bool mCalledPropertyDtor;
#endif
};

}

#endif 
