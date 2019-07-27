






#ifndef nsTransitionManager_h_
#define nsTransitionManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Animation.h"
#include "mozilla/dom/KeyframeEffect.h"
#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"

class nsIGlobalObject;
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
    : dom::KeyframeEffectReadOnly(aDocument, aTarget, aPseudoType, aTiming)
  { }

  ElementPropertyTransition* AsTransition() override { return this; }
  const ElementPropertyTransition* AsTransition() const override
  {
    return this;
  }

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

namespace dom {

class CSSTransition final : public Animation
{
public:
 explicit CSSTransition(nsIGlobalObject* aGlobal)
    : dom::Animation(aGlobal)
  {
  }

  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  CSSTransition* AsCSSTransition() override { return this; }
  const CSSTransition* AsCSSTransition() const override { return this; }

  
  void GetTransitionProperty(nsString& aRetVal) const;

  
  virtual AnimationPlayState PlayStateFromJS() const override;
  virtual void PlayFromJS(ErrorResult& aRv) override;

  
  
  void PlayFromStyle()
  {
    ErrorResult rv;
    DoPlay(rv, Animation::LimitBehavior::Continue);
    
    MOZ_ASSERT(!rv.Failed(), "Unexpected exception playing transition");
  }

  void CancelFromStyle() override
  {
    mOwningElement = OwningElementRef();
    Animation::CancelFromStyle();
    MOZ_ASSERT(mSequenceNum == kUnsequenced);
  }

  nsCSSProperty TransitionProperty() const;

  bool HasLowerCompositeOrderThan(const Animation& aOther) const override;
  bool IsUsingCustomCompositeOrder() const override
  {
    return mOwningElement.IsSet();
  }

  void SetCreationSequence(uint64_t aIndex)
  {
    MOZ_ASSERT(IsUsingCustomCompositeOrder());
    mSequenceNum = aIndex;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const OwningElementRef& OwningElement() const { return mOwningElement; }

  
  
  
  
  void SetOwningElement(const OwningElementRef& aElement)
  {
    mOwningElement = aElement;
  }

protected:
  virtual ~CSSTransition()
  {
    MOZ_ASSERT(!mOwningElement.IsSet(), "Owning element should be cleared "
                                        "before a CSS transition is destroyed");
  }

  virtual css::CommonAnimationManager* GetAnimationManager() const override;

  
  
  OwningElementRef mOwningElement;
};

} 
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
