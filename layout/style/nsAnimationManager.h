



#ifndef nsAnimationManager_h_
#define nsAnimationManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/ContentEvents.h"
#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"
#include "mozilla/dom/Animation.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"

class nsCSSKeyframesRule;
class nsStyleContext;

namespace mozilla {
namespace css {
class Declaration;
} 
namespace dom {
class Promise;
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

class CSSAnimation final : public dom::Animation
{
public:
 explicit CSSAnimation(dom::DocumentTimeline* aTimeline)
    : dom::Animation(aTimeline)
    , mIsStylePaused(false)
    , mPauseShouldStick(false)
    , mPreviousPhaseOrIteration(PREVIOUS_PHASE_BEFORE)
  {
  }

  virtual CSSAnimation* AsCSSAnimation() override { return this; }

  virtual dom::Promise* GetReady(ErrorResult& aRv) override;
  virtual void Play(LimitBehavior aLimitBehavior) override;
  virtual void Pause() override;

  virtual dom::AnimationPlayState PlayStateFromJS() const override;
  virtual void PlayFromJS() override;

  void PlayFromStyle();
  void PauseFromStyle();

  bool IsStylePaused() const { return mIsStylePaused; }

  void QueueEvents(EventArray& aEventsToDispatch);

  
  
  
  
  
  bool mInEffectForCascadeResults;

protected:
  virtual ~CSSAnimation() { }
  virtual css::CommonAnimationManager* GetAnimationManager() const override;

  static nsString PseudoTypeAsString(nsCSSPseudoElements::Type aPseudoType);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mIsStylePaused;
  bool mPauseShouldStick;

  enum {
    PREVIOUS_PHASE_BEFORE = uint64_t(-1),
    PREVIOUS_PHASE_AFTER = uint64_t(-2)
  };
  
  
  uint64_t mPreviousPhaseOrIteration;
};

} 

class nsAnimationManager final
  : public mozilla::css::CommonAnimationManager
{
public:
  explicit nsAnimationManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext)
  {
  }

  static mozilla::AnimationCollection*
  GetAnimationsForCompositor(nsIContent* aContent, nsCSSProperty aProperty)
  {
    return mozilla::css::CommonAnimationManager::GetAnimationsForCompositor(
      aContent, nsGkAtoms::animationsProperty, aProperty);
  }

  void UpdateStyleAndEvents(mozilla::AnimationCollection* aEA,
                            mozilla::TimeStamp aRefreshTime,
                            mozilla::EnsureStyleRuleFlags aFlags);
  void QueueEvents(mozilla::AnimationCollection* aEA,
                   mozilla::EventArray &aEventsToDispatch);

  void MaybeUpdateCascadeResults(mozilla::AnimationCollection* aCollection);

  
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;

  
  virtual void WillRefresh(mozilla::TimeStamp aTime) override;

  void FlushAnimations(FlushFlags aFlags);

  










  nsIStyleRule* CheckAnimationRule(nsStyleContext* aStyleContext,
                                   mozilla::dom::Element* aElement);

  






  void DispatchEvents() {
    
    if (!mPendingEvents.IsEmpty()) {
      DoDispatchEvents();
    }
  }

protected:
  virtual nsIAtom* GetAnimationsAtom() override {
    return nsGkAtoms::animationsProperty;
  }
  virtual nsIAtom* GetAnimationsBeforeAtom() override {
    return nsGkAtoms::animationsOfBeforeProperty;
  }
  virtual nsIAtom* GetAnimationsAfterAtom() override {
    return nsGkAtoms::animationsOfAfterProperty;
  }
  virtual bool IsAnimationManager() override {
    return true;
  }

private:
  void BuildAnimations(nsStyleContext* aStyleContext,
                       mozilla::dom::Element* aTarget,
                       mozilla::dom::DocumentTimeline* aTimeline,
                       mozilla::AnimationPtrArray& aAnimations);
  bool BuildSegment(InfallibleTArray<mozilla::AnimationPropertySegment>&
                      aSegments,
                    nsCSSProperty aProperty,
                    const mozilla::StyleAnimation& aAnimation,
                    float aFromKey, nsStyleContext* aFromContext,
                    mozilla::css::Declaration* aFromDeclaration,
                    float aToKey, nsStyleContext* aToContext);

  static void UpdateCascadeResults(nsStyleContext* aStyleContext,
                                   mozilla::AnimationCollection*
                                     aElementAnimations);

  
  void DoDispatchEvents();

  mozilla::EventArray mPendingEvents;
};

#endif 
