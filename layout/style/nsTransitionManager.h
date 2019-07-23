






































#ifndef nsTransitionManager_h_
#define nsTransitionManager_h_

#include "prclist.h"
#include "nsCSSProperty.h"
#include "nsIStyleRuleProcessor.h"
#include "nsRefreshDriver.h"
#include "nsCSSPseudoElements.h"

class nsStyleContext;
class nsPresContext;
class nsCSSPropertySet;
struct nsTransition;
struct ElementTransitions;





class nsTransitionManager : public nsIStyleRuleProcessor,
                            public nsARefreshObserver {
public:
  nsTransitionManager(nsPresContext *aPresContext);
  ~nsTransitionManager();

  













  already_AddRefed<nsIStyleRule>
    StyleContextChanged(nsIContent *aElement,
                        nsStyleContext *aOldStyleContext,
                        nsStyleContext *aNewStyleContext);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData);
  NS_IMETHOD RulesMatching(PseudoElementRuleProcessorData* aData);
  NS_IMETHOD RulesMatching(AnonBoxRuleProcessorData* aData);
  NS_IMETHOD RulesMatching(PseudoRuleProcessorData* aData);
  NS_IMETHOD HasStateDependentStyle(StateRuleProcessorData* aData,
                                    nsReStyleHint* aResult);
  virtual nsReStyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);
  NS_IMETHOD MediumFeaturesChanged(nsPresContext* aPresContext,
                                   PRBool* aRulesChanged);

  
  virtual void WillRefresh(mozilla::TimeStamp aTime);

private:
  friend class ElementTransitions; 

  void ConsiderStartingTransition(nsCSSProperty aProperty,
                                  const nsTransition& aTransition,
                                  nsIContent *aElement,
                                  ElementTransitions *&aElementTransitions,
                                  nsStyleContext *aOldStyleContext,
                                  nsStyleContext *aNewStyleContext,
                                  PRBool *aStartedAny,
                                  nsCSSPropertySet *aWhichStarted);
  ElementTransitions* GetElementTransitions(nsIContent *aElement,
                                            nsCSSPseudoElements::Type aPseudoType,
                                            PRBool aCreateIfNeeded);
  void AddElementTransitions(ElementTransitions* aElementTransitions);
  void TransitionsRemoved();
  nsresult WalkTransitionRule(RuleProcessorData* aData,
			      nsCSSPseudoElements::Type aPseudoType);

  PRCList mElementTransitions;
  nsPresContext *mPresContext;
};

#endif 
