











































#ifndef nsCSSRuleProcessor_h_
#define nsCSSRuleProcessor_h_

#include "nsIStyleRuleProcessor.h"
#include "nsCSSStyleSheet.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsCSSRules.h"

struct RuleCascadeData;
struct nsCSSSelectorList;












class nsCSSRuleProcessor: public nsIStyleRuleProcessor {
public:
  nsCSSRuleProcessor(const nsCOMArray<nsICSSStyleSheet>& aSheets, 
                     PRUint8 aSheetType);
  virtual ~nsCSSRuleProcessor();

  NS_DECL_ISUPPORTS

public:
  nsresult ClearRuleCascades();

  static void Startup();
  static void FreeSystemMetrics();
  static PRBool HasSystemMetric(nsIAtom* aMetric);

  






  static PRBool SelectorListMatches(RuleProcessorData& aData,
                                    nsCSSSelectorList* aSelectorList);

  
  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData);

  NS_IMETHOD RulesMatching(PseudoElementRuleProcessorData* aData);

  NS_IMETHOD RulesMatching(AnonBoxRuleProcessorData* aData);

#ifdef MOZ_XUL
  NS_IMETHOD RulesMatching(XULTreeRuleProcessorData* aData);
#endif

  virtual nsReStyleHint HasStateDependentStyle(StateRuleProcessorData* aData);

  virtual nsReStyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);

  NS_IMETHOD MediumFeaturesChanged(nsPresContext* aPresContext,
                                   PRBool* aRulesChanged);

  
  
  PRBool AppendFontFaceRules(nsPresContext* aPresContext,
                             nsTArray<nsFontFaceRuleContainer>& aArray);

#ifdef DEBUG
  void AssertQuirksChangeOK() {
    NS_ASSERTION(!mRuleCascades, "can't toggle quirks style sheet without "
                                 "clearing rule cascades");
  }
#endif

private:
  static PRBool CascadeSheetEnumFunc(nsICSSStyleSheet* aSheet, void* aData);

  RuleCascadeData* GetRuleCascade(nsPresContext* aPresContext);
  void RefreshRuleCascade(nsPresContext* aPresContext);

  
  nsCOMArray<nsICSSStyleSheet> mSheets;

  
  RuleCascadeData* mRuleCascades;

  
  nsPresContext *mLastPresContext;
  
  
  PRUint8 mSheetType;  
};

#endif 
