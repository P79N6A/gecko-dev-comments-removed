



#ifndef nsAnimationManager_h_
#define nsAnimationManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/ContentEvents.h"
#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"
#include "mozilla/dom/AnimationPlayer.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"

class nsCSSKeyframesRule;
class nsStyleContext;

namespace mozilla {
namespace css {
class Declaration;
} 

struct AnimationEventInfo {
  nsRefPtr<mozilla::dom::Element> mElement;
  mozilla::InternalAnimationEvent mEvent;

  AnimationEventInfo(mozilla::dom::Element *aElement,
                     const nsSubstring& aAnimationName,
                     uint32_t aMessage,
                     const mozilla::StickyTimeDuration& aElapsedTime,
                     const nsAString& aPseudoElement)
    : mElement(aElement), mEvent(true, aMessage)
  {
    
    mEvent.animationName = aAnimationName;
    mEvent.elapsedTime = aElapsedTime.ToSeconds();
    mEvent.pseudoElement = aPseudoElement;
  }

  
  
  AnimationEventInfo(const AnimationEventInfo &aOther)
    : mElement(aOther.mElement), mEvent(true, aOther.mEvent.message)
  {
    mEvent.AssignAnimationEventData(aOther.mEvent, false);
  }
};

typedef InfallibleTArray<AnimationEventInfo> EventArray;

class CSSAnimationPlayer MOZ_FINAL : public dom::AnimationPlayer
{
public:
 explicit CSSAnimationPlayer(dom::AnimationTimeline* aTimeline)
    : dom::AnimationPlayer(aTimeline)
    , mIsStylePaused(false)
    , mPauseShouldStick(false)
    , mLastNotification(LAST_NOTIFICATION_NONE)
  {
  }

  virtual CSSAnimationPlayer*
  AsCSSAnimationPlayer() MOZ_OVERRIDE { return this; }

  virtual void Play(UpdateFlags aUpdateFlags) MOZ_OVERRIDE;
  virtual void Pause(UpdateFlags aUpdateFlags) MOZ_OVERRIDE;

  void PlayFromStyle();
  void PauseFromStyle();

  bool IsStylePaused() const { return mIsStylePaused; }

  void QueueEvents(EventArray& aEventsToDispatch);

protected:
  virtual ~CSSAnimationPlayer() { }

  static nsString PseudoTypeAsString(nsCSSPseudoElements::Type aPseudoType);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mIsStylePaused;
  bool mPauseShouldStick;

  enum {
    LAST_NOTIFICATION_NONE = uint64_t(-1),
    LAST_NOTIFICATION_END = uint64_t(-2)
  };
  
  
  uint64_t mLastNotification;
};

} 

class nsAnimationManager MOZ_FINAL
  : public mozilla::css::CommonAnimationManager
{
public:
  explicit nsAnimationManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext)
    , mObservingRefreshDriver(false)
  {
  }

  static mozilla::AnimationPlayerCollection*
  GetAnimationsForCompositor(nsIContent* aContent, nsCSSProperty aProperty)
  {
    return mozilla::css::CommonAnimationManager::GetAnimationsForCompositor(
      aContent, nsGkAtoms::animationsProperty, aProperty);
  }

  
  static bool ContentOrAncestorHasAnimation(nsIContent* aContent) {
    do {
      if (aContent->GetProperty(nsGkAtoms::animationsProperty)) {
        return true;
      }
    } while ((aContent = aContent->GetParent()));

    return false;
  }

  void UpdateStyleAndEvents(mozilla::AnimationPlayerCollection* aEA,
                            mozilla::TimeStamp aRefreshTime,
                            mozilla::EnsureStyleRuleFlags aFlags);
  void QueueEvents(mozilla::AnimationPlayerCollection* aEA,
                   mozilla::EventArray &aEventsToDispatch);

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) MOZ_OVERRIDE;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) MOZ_OVERRIDE;
#endif
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  
  virtual void WillRefresh(mozilla::TimeStamp aTime) MOZ_OVERRIDE;

  void FlushAnimations(FlushFlags aFlags);

  










  nsIStyleRule* CheckAnimationRule(nsStyleContext* aStyleContext,
                                   mozilla::dom::Element* aElement);

  






  void DispatchEvents() {
    
    if (!mPendingEvents.IsEmpty()) {
      DoDispatchEvents();
    }
  }

  mozilla::AnimationPlayerCollection*
  GetAnimationPlayers(mozilla::dom::Element *aElement,
                      nsCSSPseudoElements::Type aPseudoType,
                      bool aCreateIfNeeded);

protected:
  virtual void ElementCollectionRemoved() MOZ_OVERRIDE
  {
    CheckNeedsRefresh();
  }
  virtual void
  AddElementCollection(mozilla::AnimationPlayerCollection* aData) MOZ_OVERRIDE;

  


  void CheckNeedsRefresh();

private:
  void BuildAnimations(nsStyleContext* aStyleContext,
                       mozilla::dom::Element* aTarget,
                       mozilla::dom::AnimationTimeline* aTimeline,
                       mozilla::AnimationPlayerPtrArray& aAnimations);
  bool BuildSegment(InfallibleTArray<mozilla::AnimationPropertySegment>&
                      aSegments,
                    nsCSSProperty aProperty,
                    const mozilla::StyleAnimation& aAnimation,
                    float aFromKey, nsStyleContext* aFromContext,
                    mozilla::css::Declaration* aFromDeclaration,
                    float aToKey, nsStyleContext* aToContext);
  nsIStyleRule* GetAnimationRule(mozilla::dom::Element* aElement,
                                 nsCSSPseudoElements::Type aPseudoType);

  
  void DoDispatchEvents();

  mozilla::EventArray mPendingEvents;

  bool mObservingRefreshDriver;
};

#endif 
