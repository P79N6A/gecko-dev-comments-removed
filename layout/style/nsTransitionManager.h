






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





struct ElementPropertyTransition : public mozilla::StyleAnimation
{
  
  
  
  
  
  nsStyleAnimation::Value mStartForReversingTest;
  
  
  
  
  
  
  
  
  
  double mReversePortion;

  
  
  
  double ValuePortionFor(mozilla::TimeStamp aRefreshTime) const;

  bool IsRemovedSentinel() const
  {
    
    
    return mStartTime.IsNull();
  }

  void SetRemovedSentinel()
  {
    
    mStartTime = mozilla::TimeStamp();
  }
};

struct ElementTransitions MOZ_FINAL
  : public mozilla::css::CommonElementAnimationData 
{
  ElementTransitions(mozilla::dom::Element *aElement, nsIAtom *aElementProperty,
                     nsTransitionManager *aTransitionManager,
                     mozilla::TimeStamp aNow);

  void EnsureStyleRuleFor(mozilla::TimeStamp aRefreshTime);

  virtual bool HasAnimationOfProperty(nsCSSProperty aProperty) const MOZ_OVERRIDE;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool CanPerformOnCompositorThread(CanAnimateFlags aFlags) const MOZ_OVERRIDE;

  typedef InfallibleTArray<nsRefPtr<ElementPropertyTransition> >
          TransitionPtrArray;
  
  TransitionPtrArray mPropertyTransitions;
};



class nsTransitionManager MOZ_FINAL
  : public mozilla::css::CommonAnimationManager
{
public:
  nsTransitionManager(nsPresContext *aPresContext)
    : mozilla::css::CommonAnimationManager(aPresContext)
  {
  }

  static ElementTransitions* GetTransitions(nsIContent* aContent) {
    return static_cast<ElementTransitions*>
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

  static ElementTransitions*
    GetTransitionsForCompositor(nsIContent* aContent,
                                nsCSSProperty aProperty)
  {
    if (!aContent->MayHaveAnimations()) {
      return nullptr;
    }
    ElementTransitions* transitions = GetTransitions(aContent);
    if (!transitions ||
        !transitions->HasAnimationOfProperty(aProperty) ||
        !transitions->CanPerformOnCompositorThread(
          CommonElementAnimationData::CanAnimate_AllowPartial)) {
      return nullptr;
    }
    return transitions;
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

  ElementTransitions* GetElementTransitions(mozilla::dom::Element *aElement,
                                          nsCSSPseudoElements::Type aPseudoType,
                                          bool aCreateIfNeeded);

protected:
  virtual void ElementDataRemoved() MOZ_OVERRIDE;
  virtual void AddElementData(mozilla::css::CommonElementAnimationData* aData) MOZ_OVERRIDE;

private:
  void ConsiderStartingTransition(nsCSSProperty aProperty,
                                  const nsTransition& aTransition,
                                  mozilla::dom::Element *aElement,
                                  ElementTransitions *&aElementTransitions,
                                  nsStyleContext *aOldStyleContext,
                                  nsStyleContext *aNewStyleContext,
                                  bool *aStartedAny,
                                  nsCSSPropertySet *aWhichStarted);
  void WalkTransitionRule(ElementDependentRuleProcessorData* aData,
                          nsCSSPseudoElements::Type aPseudoType);
  
  
  
  void UpdateThrottledStylesForSubtree(nsIContent* aContent,
                                       nsStyleContext* aParentStyle,
                                       nsStyleChangeList &aChangeList);
  void UpdateAllThrottledStylesInternal();
};

#endif 
