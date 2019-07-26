




#ifndef mozilla_css_AnimationCommon_h
#define mozilla_css_AnimationCommon_h

#include "nsIStyleRuleProcessor.h"
#include "nsIStyleRule.h"
#include "nsRefreshDriver.h"
#include "prclist.h"
#include "nsStyleAnimation.h"
#include "nsCSSProperty.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Element.h"
#include "nsSMILKeySpline.h"
#include "nsStyleStruct.h"
#include "mozilla/Attributes.h"
#include "nsCSSPseudoElements.h"

class nsPresContext;
class nsIFrame;
class ElementPropertyTransition;


namespace mozilla {
namespace css {

bool IsGeometricProperty(nsCSSProperty aProperty);

struct CommonElementAnimationData;

class CommonAnimationManager : public nsIStyleRuleProcessor,
                               public nsARefreshObserver {
public:
  CommonAnimationManager(nsPresContext *aPresContext);
  virtual ~CommonAnimationManager();

  
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

  enum FlushFlags {
    Can_Throttle,
    Cannot_Throttle
  };

  static bool ExtractComputedValueForTransition(
                  nsCSSProperty aProperty,
                  nsStyleContext* aStyleContext,
                  nsStyleAnimation::Value& aComputedValue);
protected:
  friend struct CommonElementAnimationData; 

  virtual void AddElementData(CommonElementAnimationData* aData) = 0;
  virtual void ElementDataRemoved() = 0;
  void RemoveAllElementData();

  
  
  
  nsStyleContext* UpdateThrottledStyle(mozilla::dom::Element* aElement,
                                       nsStyleContext* aParentStyle,
                                       nsStyleChangeList &aChangeList);
  
  already_AddRefed<nsStyleContext> ReparentContent(nsIContent* aContent,
                                                  nsStyleContext* aParentStyle);
  
  static void ReparentBeforeAndAfter(dom::Element* aElement,
                                     nsIFrame* aPrimaryFrame,
                                     nsStyleContext* aNewStyle,
                                     nsStyleSet* aStyleSet);

  PRCList mElementData;
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
  PRCList *next = PR_LIST_HEAD(&mElementData);                                 \
  while (next != &mElementData) {                                              \
    CommonElementAnimationData* ea =                                           \
      static_cast<CommonElementAnimationData*>(next);                          \
    next = PR_NEXT_LINK(next);                                                 \
                                                                               \
    if (ea->mFlushGeneration == now) {                                         \
                                     \
      continue;                                                                \
    }                                                                          \
                                                                               \
    

                         \
    dom::Element* element = ea->mElement;                                      \
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

  void AddValue(nsCSSProperty aProperty, nsStyleAnimation::Value &aStartValue)
  {
    PropertyValuePair v = { aProperty, aStartValue };
    mPropertyValuePairs.AppendElement(v);
  }

  
  nsStyleAnimation::Value* AddEmptyValue(nsCSSProperty aProperty)
  {
    PropertyValuePair *p = mPropertyValuePairs.AppendElement();
    p->mProperty = aProperty;
    return &p->mValue;
  }

  struct PropertyValuePair {
    nsCSSProperty mProperty;
    nsStyleAnimation::Value mValue;
  };

private:
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
  nsStyleAnimation::Value mFromValue, mToValue;
  mozilla::css::ComputedTimingFunction mTimingFunction;
};

struct AnimationProperty
{
  nsCSSProperty mProperty;
  InfallibleTArray<AnimationPropertySegment> mSegments;
};





struct ElementAnimation
{
  ElementAnimation()
    : mIsRunningOnCompositor(false)
    , mLastNotification(LAST_NOTIFICATION_NONE)
  {
  }

  
  
  
  virtual ~ElementAnimation() { }
  virtual ElementPropertyTransition* AsTransition() { return nullptr; }
  virtual const ElementPropertyTransition* AsTransition() const {
    return nullptr;
  }

  nsString mName; 
  float mIterationCount; 
  uint8_t mDirection;
  uint8_t mFillMode;
  uint8_t mPlayState;

  bool FillsForwards() const {
    return mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BOTH ||
           mFillMode == NS_STYLE_ANIMATION_FILL_MODE_FORWARDS;
  }
  bool FillsBackwards() const {
    return mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BOTH ||
           mFillMode == NS_STYLE_ANIMATION_FILL_MODE_BACKWARDS;
  }

  bool IsPaused() const {
    return mPlayState == NS_STYLE_ANIMATION_PLAY_STATE_PAUSED;
  }

  bool HasAnimationOfProperty(nsCSSProperty aProperty) const;
  bool IsRunningAt(mozilla::TimeStamp aTime) const;

  
  
  mozilla::TimeDuration ElapsedDurationAt(mozilla::TimeStamp aTime) const {
    NS_ABORT_IF_FALSE(!IsPaused() || aTime >= mPauseStart,
                      "if paused, aTime must be at least mPauseStart");
    return (IsPaused() ? mPauseStart : aTime) - mStartTime - mDelay;
  }

  
  
  
  mozilla::TimeStamp mStartTime;
  mozilla::TimeStamp mPauseStart;
  mozilla::TimeDuration mDelay;
  mozilla::TimeDuration mIterationDuration;
  bool mIsRunningOnCompositor;

  enum {
    LAST_NOTIFICATION_NONE = uint32_t(-1),
    LAST_NOTIFICATION_END = uint32_t(-2)
  };
  
  
  uint32_t mLastNotification;

  InfallibleTArray<AnimationProperty> mProperties;

  NS_INLINE_DECL_REFCOUNTING(ElementAnimation)
};

typedef InfallibleTArray<nsRefPtr<ElementAnimation> > ElementAnimationPtrArray;

namespace css {

struct CommonElementAnimationData : public PRCList
{
  CommonElementAnimationData(dom::Element *aElement, nsIAtom *aElementProperty,
                             CommonAnimationManager *aManager, TimeStamp aNow)
    : mElement(aElement)
    , mElementProperty(aElementProperty)
    , mManager(aManager)
    , mAnimationGeneration(0)
    , mFlushGeneration(aNow)
#ifdef DEBUG
    , mCalledPropertyDtor(false)
#endif
  {
    MOZ_COUNT_CTOR(CommonElementAnimationData);
    PR_INIT_CLIST(this);
  }
  ~CommonElementAnimationData()
  {
    NS_ABORT_IF_FALSE(mCalledPropertyDtor,
                      "must call destructor through element property dtor");
    MOZ_COUNT_DTOR(CommonElementAnimationData);
    PR_REMOVE_LINK(this);
    mManager->ElementDataRemoved();
  }

  void Destroy()
  {
    
    mElement->DeleteProperty(mElementProperty);
  }

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

  
  
  
  virtual bool CanPerformOnCompositorThread(CanAnimateFlags aFlags) const = 0;
  virtual bool HasAnimationOfProperty(nsCSSProperty aProperty) const = 0;

  static void LogAsyncAnimationFailure(nsCString& aMessage,
                                       const nsIContent* aContent = nullptr);

  dom::Element *mElement;

  
  
  nsIAtom *mElementProperty;

  CommonAnimationManager *mManager;

  
  
  
  
  
  
  
  nsRefPtr<mozilla::css::AnimValuesStyleRule> mStyleRule;

  
  
  
  
  
  
  uint64_t mAnimationGeneration;
  
  void UpdateAnimationGeneration(nsPresContext* aPresContext);

  
  TimeStamp mStyleRuleRefreshTime;

  
  
  
  TimeStamp mFlushGeneration;

#ifdef DEBUG
  bool mCalledPropertyDtor;
#endif
};

}
}

#endif 
