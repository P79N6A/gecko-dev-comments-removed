



#ifndef nsAnimationManager_h_
#define nsAnimationManager_h_

#include "mozilla/Attributes.h"
#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"
#include "nsStyleContext.h"
#include "nsDataHashtable.h"
#include "nsGUIEvent.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Preferences.h"
#include "nsThreadUtils.h"

class nsCSSKeyframesRule;

namespace mozilla {
namespace css {
class Declaration;
}
}

struct AnimationEventInfo {
  nsRefPtr<mozilla::dom::Element> mElement;
  nsAnimationEvent mEvent;

  AnimationEventInfo(mozilla::dom::Element *aElement,
                     const nsString& aAnimationName,
                     uint32_t aMessage, mozilla::TimeDuration aElapsedTime)
    : mElement(aElement),
      mEvent(true, aMessage, aAnimationName, aElapsedTime.ToSeconds())
  {
  }

  
  
  AnimationEventInfo(const AnimationEventInfo &aOther)
    : mElement(aOther.mElement),
      mEvent(true, aOther.mEvent.message,
             aOther.mEvent.animationName, aOther.mEvent.elapsedTime)
  {
  }
};

typedef InfallibleTArray<AnimationEventInfo> EventArray;

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
    : mLastNotification(LAST_NOTIFICATION_NONE)
  {
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

  virtual bool HasAnimationOfProperty(nsCSSProperty aProperty) const;
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

  enum {
    LAST_NOTIFICATION_NONE = uint32_t(-1),
    LAST_NOTIFICATION_END = uint32_t(-2)
  };
  
  
  uint32_t mLastNotification;

  InfallibleTArray<AnimationProperty> mProperties;
};




struct ElementAnimations MOZ_FINAL
  : public mozilla::css::CommonElementAnimationData
{
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  ElementAnimations(mozilla::dom::Element *aElement, nsIAtom *aElementProperty,
                    nsAnimationManager *aAnimationManager);

  
  
  
  
  
  
  
  
  
  
  
  
  static double GetPositionInIteration(TimeDuration aElapsedDuration,
                                       TimeDuration aIterationDuration,
                                       double aIterationCount,
                                       uint32_t aDirection,
                                       bool aIsForElement = true,
                                       ElementAnimation* aAnimation = nullptr,
                                       ElementAnimations* aEa = nullptr,
                                       EventArray* aEventsToDispatch = nullptr);

  void EnsureStyleRuleFor(TimeStamp aRefreshTime,
                          EventArray &aEventsToDispatch,
                          bool aIsThrottled);

  bool IsForElement() const { 
    return mElementProperty == nsGkAtoms::animationsProperty;
  }

  void PostRestyleForAnimation(nsPresContext *aPresContext) {
    nsRestyleHint styleHint = IsForElement() ? eRestyle_Self : eRestyle_Subtree;
    aPresContext->PresShell()->RestyleForAnimation(mElement, styleHint);
  }

  
  virtual bool CanPerformOnCompositorThread(CanAnimateFlags aFlags) const MOZ_OVERRIDE;
  virtual bool HasAnimationOfProperty(nsCSSProperty aProperty) const MOZ_OVERRIDE;

  
  
  
  bool mNeedsRefreshes;

  InfallibleTArray<ElementAnimation> mAnimations;
};

class nsAnimationManager : public mozilla::css::CommonAnimationManager
{
public:
  nsAnimationManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext)
    , mKeyframesListIsDirty(true)
  {
    mKeyframesRules.Init(16); 
  }

  static ElementAnimations* GetAnimationsForCompositor(nsIContent* aContent,
                                                       nsCSSProperty aProperty)
  {
    if (!aContent->MayHaveAnimations())
      return nullptr;
    ElementAnimations* animations = static_cast<ElementAnimations*>(
      aContent->GetProperty(nsGkAtoms::animationsProperty));
    if (!animations)
      return nullptr;
    bool propertyMatches = animations->HasAnimationOfProperty(aProperty);
    return (propertyMatches &&
            animations->CanPerformOnCompositorThread(
              mozilla::css::CommonElementAnimationData::CanAnimate_AllowPartial))
           ? animations
           : nullptr;
  }

  
  static bool ContentOrAncestorHasAnimation(nsIContent* aContent) {
    do {
      if (aContent->GetProperty(nsGkAtoms::animationsProperty)) {
        return true;
      }
    } while ((aContent = aContent->GetParent()));

    return false;
  }

  void EnsureStyleRuleFor(ElementAnimations* aET);

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) MOZ_OVERRIDE;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) MOZ_OVERRIDE;
#endif
  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  
  virtual void WillRefresh(mozilla::TimeStamp aTime) MOZ_OVERRIDE;

  void FlushAnimations(FlushFlags aFlags);

  










  nsIStyleRule* CheckAnimationRule(nsStyleContext* aStyleContext,
                                   mozilla::dom::Element* aElement);

  void KeyframesListIsDirty() {
    mKeyframesListIsDirty = true;
  }

  






  void DispatchEvents() {
    
    if (!mPendingEvents.IsEmpty()) {
      DoDispatchEvents();
    }
  }

  ElementAnimations* GetElementAnimations(mozilla::dom::Element *aElement,
                                          nsCSSPseudoElements::Type aPseudoType,
                                          bool aCreateIfNeeded);

private:
  void BuildAnimations(nsStyleContext* aStyleContext,
                       InfallibleTArray<ElementAnimation>& aAnimations);
  bool BuildSegment(InfallibleTArray<AnimationPropertySegment>& aSegments,
                    nsCSSProperty aProperty, const nsAnimation& aAnimation,
                    float aFromKey, nsStyleContext* aFromContext,
                    mozilla::css::Declaration* aFromDeclaration,
                    float aToKey, nsStyleContext* aToContext);
  nsIStyleRule* GetAnimationRule(mozilla::dom::Element* aElement,
                                 nsCSSPseudoElements::Type aPseudoType);

  nsCSSKeyframesRule* KeyframesRuleFor(const nsSubstring& aName);

  
  void DoDispatchEvents();

  bool mKeyframesListIsDirty;
  nsDataHashtable<nsStringHashKey, nsCSSKeyframesRule*> mKeyframesRules;

  EventArray mPendingEvents;
};

#endif 
