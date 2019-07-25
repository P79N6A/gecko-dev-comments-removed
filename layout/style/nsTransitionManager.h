






#ifndef nsTransitionManager_h_
#define nsTransitionManager_h_

#include "AnimationCommon.h"
#include "nsCSSPseudoElements.h"

class nsStyleContext;
class nsPresContext;
class nsCSSPropertySet;
struct nsTransition;





struct ElementPropertyTransition
{
  ElementPropertyTransition() {}

  nsCSSProperty mProperty;
  nsStyleAnimation::Value mStartValue, mEndValue;
  mozilla::TimeStamp mStartTime; 

  
  mozilla::TimeDuration mDuration;
  mozilla::css::ComputedTimingFunction mTimingFunction;

  
  
  
  
  
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

  bool CanPerformOnCompositor(mozilla::dom::Element* aElement,
                              mozilla::TimeStamp aTime) const;
};

struct ElementTransitions : public mozilla::css::CommonElementAnimationData
{
  ElementTransitions(mozilla::dom::Element *aElement, nsIAtom *aElementProperty,
                     nsTransitionManager *aTransitionManager);

  void EnsureStyleRuleFor(mozilla::TimeStamp aRefreshTime);


  bool HasTransitionOfProperty(nsCSSProperty aProperty) const;
  
  bool CanPerformOnCompositorThread() const;
  
  nsTArray<ElementPropertyTransition> mPropertyTransitions;

  
  
  
  
  
  
  nsRefPtr<mozilla::css::AnimValuesStyleRule> mStyleRule;
  
  mozilla::TimeStamp mStyleRuleRefreshTime;
};



class nsTransitionManager : public mozilla::css::CommonAnimationManager
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

  static ElementTransitions*
    GetTransitionsForCompositor(nsIContent* aContent,
                                nsCSSProperty aProperty)
  {
    if (!aContent->MayHaveAnimations())
      return nullptr;
    ElementTransitions* transitions = GetTransitions(aContent);
    if (!transitions ||
        !transitions->HasTransitionOfProperty(aProperty) ||
        !transitions->CanPerformOnCompositorThread()) {
      return nullptr;
    }
    return transitions;
  }

  















  already_AddRefed<nsIStyleRule>
    StyleContextChanged(mozilla::dom::Element *aElement,
                        nsStyleContext *aOldStyleContext,
                        nsStyleContext *aNewStyleContext);

  
  virtual void RulesMatching(ElementRuleProcessorData* aData);
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData);
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData);
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData);
#endif
  virtual NS_MUST_OVERRIDE size_t
    SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;
  virtual NS_MUST_OVERRIDE size_t
    SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;

  
  virtual void WillRefresh(mozilla::TimeStamp aTime);

private:
  void ConsiderStartingTransition(nsCSSProperty aProperty,
                                  const nsTransition& aTransition,
                                  mozilla::dom::Element *aElement,
                                  ElementTransitions *&aElementTransitions,
                                  nsStyleContext *aOldStyleContext,
                                  nsStyleContext *aNewStyleContext,
                                  bool *aStartedAny,
                                  nsCSSPropertySet *aWhichStarted);
  ElementTransitions* GetElementTransitions(mozilla::dom::Element *aElement,
                                            nsCSSPseudoElements::Type aPseudoType,
                                            bool aCreateIfNeeded);
  void WalkTransitionRule(RuleProcessorData* aData,
                          nsCSSPseudoElements::Type aPseudoType);
};

#endif 
