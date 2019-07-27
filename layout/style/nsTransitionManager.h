






#ifndef nsTransitionManager_h_
#define nsTransitionManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
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

struct ElementPropertyTransition : public mozilla::ElementAnimation
{
  explicit ElementPropertyTransition(mozilla::dom::AnimationTimeline* aTimeline)
    : mozilla::ElementAnimation(aTimeline) { }

  virtual ElementPropertyTransition* AsTransition() { return this; }
  virtual const ElementPropertyTransition* AsTransition() const { return this; }

  
  
  
  
  
  mozilla::StyleAnimationValue mStartForReversingTest;
  
  
  
  
  
  
  
  
  
  double mReversePortion;

  
  
  
  double CurrentValuePortion() const;
};

} 

class nsTransitionManager MOZ_FINAL
  : public mozilla::css::CommonAnimationManager
{
public:
  nsTransitionManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext)
    , mInAnimationOnlyStyleUpdate(false)
  {
  }

  typedef mozilla::ElementAnimationCollection ElementAnimationCollection;

  static ElementAnimationCollection*
  GetTransitions(nsIContent* aContent) {
    return static_cast<ElementAnimationCollection*>
      (aContent->GetProperty(nsGkAtoms::transitionsProperty));
  }

  
  static bool ContentOrAncestorHasTransition(nsIContent* aContent) {
    do {
      if (GetTransitions(aContent)) {
        return true;
      }
    } while ((aContent = aContent->GetParent()));

    return false;
  }

  static ElementAnimationCollection*
  GetAnimationsForCompositor(nsIContent* aContent, nsCSSProperty aProperty)
  {
    return mozilla::css::CommonAnimationManager::GetAnimationsForCompositor(
      aContent, nsGkAtoms::transitionsProperty, aProperty);
  }

  















  already_AddRefed<nsIStyleRule>
    StyleContextChanged(mozilla::dom::Element *aElement,
                        nsStyleContext *aOldStyleContext,
                        nsStyleContext *aNewStyleContext);

  void SetInAnimationOnlyStyleUpdate(bool aInAnimationOnlyUpdate) {
    mInAnimationOnlyStyleUpdate = aInAnimationOnlyUpdate;
  }

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) MOZ_OVERRIDE;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) MOZ_OVERRIDE;
#endif
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  
  virtual void WillRefresh(mozilla::TimeStamp aTime) MOZ_OVERRIDE;

  void FlushTransitions(FlushFlags aFlags);

  ElementAnimationCollection* GetElementTransitions(
    mozilla::dom::Element *aElement,
    nsCSSPseudoElements::Type aPseudoType,
    bool aCreateIfNeeded);

protected:
  virtual void ElementCollectionRemoved() MOZ_OVERRIDE;
  virtual void
  AddElementCollection(ElementAnimationCollection* aCollection) MOZ_OVERRIDE;

private:
  void
  ConsiderStartingTransition(nsCSSProperty aProperty,
                             const mozilla::StyleTransition& aTransition,
                             mozilla::dom::Element* aElement,
                             ElementAnimationCollection*& aElementTransitions,
                             nsStyleContext* aOldStyleContext,
                             nsStyleContext* aNewStyleContext,
                             bool* aStartedAny,
                             nsCSSPropertySet* aWhichStarted);
  void WalkTransitionRule(ElementDependentRuleProcessorData* aData,
                          nsCSSPseudoElements::Type aPseudoType);

  bool mInAnimationOnlyStyleUpdate;
};

#endif 
