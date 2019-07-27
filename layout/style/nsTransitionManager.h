






#ifndef nsTransitionManager_h_
#define nsTransitionManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Animation.h"
#include "mozilla/dom/AnimationPlayer.h"
#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"

class nsStyleContext;
class nsPresContext;
class nsCSSPropertySet;
struct ElementDependentRuleProcessorData;

namespace mozilla {
struct StyleTransition;
}





namespace mozilla {

struct ElementPropertyTransition : public dom::Animation
{
  ElementPropertyTransition(nsIDocument* aDocument,
                            dom::Element* aTarget,
                            nsCSSPseudoElements::Type aPseudoType,
                            const AnimationTiming &aTiming)
    : dom::Animation(aDocument, aTarget, aPseudoType, aTiming, EmptyString())
  { }

  virtual ElementPropertyTransition* AsTransition() { return this; }
  virtual const ElementPropertyTransition* AsTransition() const { return this; }

  
  
  
  
  
  mozilla::StyleAnimationValue mStartForReversingTest;
  
  
  
  
  
  
  
  
  
  double mReversePortion;

  
  
  
  double CurrentValuePortion() const;
};

class CSSTransitionPlayer final : public dom::AnimationPlayer
{
public:
 explicit CSSTransitionPlayer(dom::AnimationTimeline* aTimeline)
    : dom::AnimationPlayer(aTimeline)
  {
  }

  virtual CSSTransitionPlayer*
  AsCSSTransitionPlayer() override { return this; }

  virtual dom::AnimationPlayState PlayStateFromJS() const override;
  virtual void PlayFromJS() override;

  
  
  void PlayFromStyle() { DoPlay(AnimationPlayer::LimitBehavior::Continue); }

protected:
  virtual ~CSSTransitionPlayer() { }

  virtual css::CommonAnimationManager* GetAnimationManager() const override;
};

} 

class nsTransitionManager final
  : public mozilla::css::CommonAnimationManager
{
public:
  explicit nsTransitionManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext)
    , mInAnimationOnlyStyleUpdate(false)
  {
  }

  typedef mozilla::AnimationPlayerCollection AnimationPlayerCollection;

  static AnimationPlayerCollection*
  GetAnimationsForCompositor(nsIContent* aContent, nsCSSProperty aProperty)
  {
    return mozilla::css::CommonAnimationManager::GetAnimationsForCompositor(
      aContent, nsGkAtoms::transitionsProperty, aProperty);
  }

  













  void StyleContextChanged(mozilla::dom::Element *aElement,
                           nsStyleContext *aOldStyleContext,
                           nsRefPtr<nsStyleContext>* aNewStyleContext );

  void UpdateCascadeResultsWithTransitions(
         AnimationPlayerCollection* aTransitions);
  void UpdateCascadeResultsWithAnimations(
         AnimationPlayerCollection* aAnimations);
  void UpdateCascadeResultsWithAnimationsToBeDestroyed(
         const AnimationPlayerCollection* aAnimations);
  void UpdateCascadeResults(AnimationPlayerCollection* aTransitions,
                            AnimationPlayerCollection* aAnimations);

  void SetInAnimationOnlyStyleUpdate(bool aInAnimationOnlyUpdate) {
    mInAnimationOnlyStyleUpdate = aInAnimationOnlyUpdate;
  }

  bool InAnimationOnlyStyleUpdate() const {
    return mInAnimationOnlyStyleUpdate;
  }

  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    MOZ_MUST_OVERRIDE override;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    MOZ_MUST_OVERRIDE override;

  
  virtual void WillRefresh(mozilla::TimeStamp aTime) override;

  void FlushTransitions(FlushFlags aFlags);

protected:
  virtual nsIAtom* GetAnimationsAtom() override {
    return nsGkAtoms::transitionsProperty;
  }
  virtual nsIAtom* GetAnimationsBeforeAtom() override {
    return nsGkAtoms::transitionsOfBeforeProperty;
  }
  virtual nsIAtom* GetAnimationsAfterAtom() override {
    return nsGkAtoms::transitionsOfAfterProperty;
  }

private:
  void
  ConsiderStartingTransition(nsCSSProperty aProperty,
                             const mozilla::StyleTransition& aTransition,
                             mozilla::dom::Element* aElement,
                             AnimationPlayerCollection*& aElementTransitions,
                             nsStyleContext* aOldStyleContext,
                             nsStyleContext* aNewStyleContext,
                             bool* aStartedAny,
                             nsCSSPropertySet* aWhichStarted);

  bool mInAnimationOnlyStyleUpdate;
};

#endif 
