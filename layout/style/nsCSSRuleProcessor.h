











































#ifndef nsCSSRuleProcessor_h_
#define nsCSSRuleProcessor_h_

#include "nsIStyleRuleProcessor.h"
#include "nsCSSStyleSheet.h"

struct RuleCascadeData;
struct nsCSSSelectorList;












class nsCSSRuleProcessor: public nsIStyleRuleProcessor {
public:
  nsCSSRuleProcessor(const nsCOMArray<nsICSSStyleSheet>& aSheets);
  virtual ~nsCSSRuleProcessor();

  NS_DECL_ISUPPORTS

public:
  nsresult ClearRuleCascades();

  static void Startup();
  static void FreeSystemMetrics();

  




  static PRBool SelectorListMatches(RuleProcessorData& aData,
                                    nsCSSSelectorList* aSelectorList);

  
  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData);

  NS_IMETHOD RulesMatching(PseudoRuleProcessorData* aData);

  NS_IMETHOD HasStateDependentStyle(StateRuleProcessorData* aData,
                                    nsReStyleHint* aResult);

  NS_IMETHOD HasAttributeDependentStyle(AttributeRuleProcessorData* aData,
                                        nsReStyleHint* aResult);

  NS_IMETHOD MediumFeaturesChanged(nsPresContext* aPresContext,
                                   PRBool* aRulesChanged);

#ifdef DEBUG
  void AssertQuirksChangeOK() {
    NS_ASSERTION(!mRuleCascades, "can't toggle quirks style sheet without "
                                 "clearing rule cascades");
  }
#endif

private:
  static PRBool CascadeSheetRulesInto(nsICSSStyleSheet* aSheet, void* aData);

  RuleCascadeData* GetRuleCascade(nsPresContext* aPresContext);
  void RefreshRuleCascade(nsPresContext* aPresContext);

  
  nsCOMArray<nsICSSStyleSheet> mSheets;

  
  RuleCascadeData* mRuleCascades;

  
  nsPresContext *mLastPresContext;
};

#endif 
