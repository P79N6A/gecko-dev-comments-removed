






#ifndef nsTransitionManager_h_
#define nsTransitionManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"

class nsStyleContext;
class nsPresContext;
class nsCSSPropertySet;
struct nsTransition;
struct ElementDependentRuleProcessorData;





struct ElementPropertyTransition : public mozilla::ElementAnimation
{
  virtual ElementPropertyTransition* AsTransition() { return this; }
  virtual const ElementPropertyTransition* AsTransition() const { return this; }

  
  
  
  
  
  mozilla::StyleAnimationValue mStartForReversingTest;
  
  
  
  
  
  
  
  
  
  double mReversePortion;

  
  
  
  double ValuePortionFor(mozilla::TimeStamp aRefreshTime) const;
};

class nsTransitionManager MOZ_FINAL
  : public mozilla::css::CommonAnimationManager
{
public:
  nsTransitionManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext)
  {
  }

  static mozilla::css::CommonElementAnimationData*
  GetTransitions(nsIContent* aContent) {
    return static_cast<CommonElementAnimationData*>
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

  typedef mozilla::css::CommonElementAnimationData CommonElementAnimationData;

  static CommonElementAnimationData*
  GetAnimationsForCompositor(nsIContent* aContent, nsCSSProperty aProperty)
  {
    return mozilla::css::CommonAnimationManager::GetAnimationsForCompositor(
      aContent, nsGkAtoms::transitionsProperty, aProperty);
  }

  















  already_AddRefed<nsIStyleRule>
    StyleContextChanged(mozilla::dom::Element *aElement,
                        nsStyleContext *aOldStyleContext,
                        nsStyleContext *aNewStyleContext);

  
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void UpdateAllThrottledStyles();

  CommonElementAnimationData* GetElementTransitions(
    mozilla::dom::Element *aElement,
    nsCSSPseudoElements::Type aPseudoType,
    bool aCreateIfNeeded);

protected:
  virtual void ElementDataRemoved() MOZ_OVERRIDE;
  virtual void AddElementData(mozilla::css::CommonElementAnimationData* aData) MOZ_OVERRIDE;

private:
  void ConsiderStartingTransition(nsCSSProperty aProperty,
                                  const nsTransition& aTransition,
                                  mozilla::dom::Element* aElement,
                                  CommonElementAnimationData*& aElementTransitions,
                                  nsStyleContext* aOldStyleContext,
                                  nsStyleContext* aNewStyleContext,
                                  bool* aStartedAny,
                                  nsCSSPropertySet* aWhichStarted);
  void WalkTransitionRule(ElementDependentRuleProcessorData* aData,
                          nsCSSPseudoElements::Type aPseudoType);
  
  
  
  void UpdateThrottledStylesForSubtree(nsIContent* aContent,
                                       nsStyleContext* aParentStyle,
                                       nsStyleChangeList &aChangeList);
  void UpdateAllThrottledStylesInternal();
};

#endif 
