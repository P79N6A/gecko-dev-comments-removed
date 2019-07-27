






#ifndef nsTransitionManager_h_
#define nsTransitionManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Animation.h"
#include "mozilla/dom/KeyframeEffect.h"
#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"

class nsStyleContext;
class nsPresContext;
class nsCSSPropertySet;

namespace mozilla {
struct StyleTransition;
}





namespace mozilla {

struct ElementPropertyTransition : public dom::KeyframeEffectReadOnly
{
  ElementPropertyTransition(nsIDocument* aDocument,
                            dom::Element* aTarget,
                            nsCSSPseudoElements::Type aPseudoType,
                            const AnimationTiming &aTiming)
    : dom::KeyframeEffectReadOnly(aDocument, aTarget, aPseudoType,
                                  aTiming, EmptyString())
  { }

  virtual ElementPropertyTransition* AsTransition() override { return this; }
  virtual const ElementPropertyTransition* AsTransition() const override { return this; }

  virtual const nsString& Name() const override;

  nsCSSProperty TransitionProperty() const {
    MOZ_ASSERT(Properties().Length() == 1,
               "Transitions should have exactly one animation property. "
               "Perhaps we are using an un-initialized transition?");
    return Properties()[0].mProperty;
  }

  
  
  
  
  
  mozilla::StyleAnimationValue mStartForReversingTest;
  
  
  
  
  
  
  
  
  
  double mReversePortion;

  
  
  
  double CurrentValuePortion() const;
};

class CSSTransition final : public dom::Animation
{
public:
 explicit CSSTransition(dom::DocumentTimeline* aTimeline)
    : dom::Animation(aTimeline)
  {
  }

  virtual CSSTransition* AsCSSTransition() override { return this; }

  virtual dom::AnimationPlayState PlayStateFromJS() const override;
  virtual void PlayFromJS() override;

  
  
  void PlayFromStyle() { DoPlay(Animation::LimitBehavior::Continue); }

protected:
  virtual ~CSSTransition() { }

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

  typedef mozilla::AnimationCollection AnimationCollection;

  static AnimationCollection*
  GetAnimationsForCompositor(nsIContent* aContent, nsCSSProperty aProperty)
  {
    return mozilla::css::CommonAnimationManager::GetAnimationsForCompositor(
      aContent, nsGkAtoms::transitionsProperty, aProperty);
  }

  













  void StyleContextChanged(mozilla::dom::Element *aElement,
                           nsStyleContext *aOldStyleContext,
                           nsRefPtr<nsStyleContext>* aNewStyleContext );

  







  void PruneCompletedTransitions(mozilla::dom::Element* aElement,
                                 nsCSSPseudoElements::Type aPseudoType,
                                 nsStyleContext* aNewStyleContext);

  void UpdateCascadeResultsWithTransitions(AnimationCollection* aTransitions);
  void UpdateCascadeResultsWithAnimations(AnimationCollection* aAnimations);
  void UpdateCascadeResultsWithAnimationsToBeDestroyed(
         const AnimationCollection* aAnimations);
  void UpdateCascadeResults(AnimationCollection* aTransitions,
                            AnimationCollection* aAnimations);

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
                             AnimationCollection*& aElementTransitions,
                             nsStyleContext* aOldStyleContext,
                             nsStyleContext* aNewStyleContext,
                             bool* aStartedAny,
                             nsCSSPropertySet* aWhichStarted);

  bool mInAnimationOnlyStyleUpdate;
};

#endif 
