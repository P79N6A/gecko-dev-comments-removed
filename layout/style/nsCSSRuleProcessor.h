











































#ifndef nsCSSRuleProcessor_h_
#define nsCSSRuleProcessor_h_

#include "nsIStyleRuleProcessor.h"
#include "nsCSSStyleSheet.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsCSSRules.h"

struct RuleCascadeData;
struct nsCSSSelectorList;
struct CascadeEnumData;












class nsCSSRuleProcessor: public nsIStyleRuleProcessor {
public:
  typedef nsTArray<nsRefPtr<nsCSSStyleSheet> > sheet_array_type;

  nsCSSRuleProcessor(const sheet_array_type& aSheets, PRUint8 aSheetType);
  virtual ~nsCSSRuleProcessor();

  NS_DECL_ISUPPORTS

public:
  nsresult ClearRuleCascades();

  static nsresult Startup();
  static void Shutdown();
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

  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData);

  virtual PRBool HasDocumentStateDependentStyle(StateRuleProcessorData* aData);

  virtual nsRestyleHint
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
  static PRBool CascadeSheet(nsCSSStyleSheet* aSheet, CascadeEnumData* aData);

  RuleCascadeData* GetRuleCascade(nsPresContext* aPresContext);
  void RefreshRuleCascade(nsPresContext* aPresContext);

  
  sheet_array_type mSheets;

  
  RuleCascadeData* mRuleCascades;

  
  nsPresContext *mLastPresContext;
  
  
  PRUint8 mSheetType;  
};

#endif 
