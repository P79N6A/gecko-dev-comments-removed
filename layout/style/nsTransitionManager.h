






































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

  


  void Disconnect();

  















  already_AddRefed<nsIStyleRule>
    StyleContextChanged(mozilla::dom::Element *aElement,
                        nsStyleContext *aOldStyleContext,
                        nsStyleContext *aNewStyleContext);

  
  NS_DECL_ISUPPORTS

  
  virtual void RulesMatching(ElementRuleProcessorData* aData);
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData);
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData);
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData);
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData);
  virtual PRBool HasDocumentStateDependentStyle(StateRuleProcessorData* aData);
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);
  virtual PRBool MediumFeaturesChanged(nsPresContext* aPresContext);

  
  virtual void WillRefresh(mozilla::TimeStamp aTime);

private:
  friend class ElementTransitions; 

  void ConsiderStartingTransition(nsCSSProperty aProperty,
                                  const nsTransition& aTransition,
                                  mozilla::dom::Element *aElement,
                                  ElementTransitions *&aElementTransitions,
                                  nsStyleContext *aOldStyleContext,
                                  nsStyleContext *aNewStyleContext,
                                  PRBool *aStartedAny,
                                  nsCSSPropertySet *aWhichStarted);
  ElementTransitions* GetElementTransitions(mozilla::dom::Element *aElement,
                                            nsCSSPseudoElements::Type aPseudoType,
                                            PRBool aCreateIfNeeded);
  void AddElementTransitions(ElementTransitions* aElementTransitions);
  void TransitionsRemoved();
  void WalkTransitionRule(RuleProcessorData* aData,
                          nsCSSPseudoElements::Type aPseudoType);

  void RemoveAllTransitions();

  PRCList mElementTransitions;
  nsPresContext *mPresContext; 
};

#endif 
