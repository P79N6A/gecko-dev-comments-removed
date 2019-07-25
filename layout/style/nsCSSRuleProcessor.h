











































#ifndef nsCSSRuleProcessor_h_
#define nsCSSRuleProcessor_h_

#include "nsIStyleRuleProcessor.h"
#include "nsCSSStyleSheet.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsCSSRules.h"
#include "nsRuleWalker.h"

struct RuleCascadeData;
struct nsCSSSelectorList;
struct CascadeEnumData;
struct TreeMatchContext;
class nsCSSKeyframesRule;












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
  static bool HasSystemMetric(nsIAtom* aMetric);

  






  static bool SelectorListMatches(const mozilla::dom::Element* aElement,
                                    TreeMatchContext& aTreeMatchContext,
                                    const nsCSSSelectorList* aSelectorList);

  



  static nsEventStates GetContentState(const mozilla::dom::Element* aElement);

  


  static nsEventStates GetContentStateForVisitedHandling(
             const mozilla::dom::Element* aElement,
             nsRuleWalker::VisitedHandlingType aVisitedHandling,
             bool aIsRelevantLink);

  


  static bool IsLink(const mozilla::dom::Element* aElement);

  
  virtual void RulesMatching(ElementRuleProcessorData* aData);

  virtual void RulesMatching(PseudoElementRuleProcessorData* aData);

  virtual void RulesMatching(AnonBoxRuleProcessorData* aData);

#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData);
#endif

  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData);

  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData);

  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);

  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext);

  virtual PRInt64 SizeOf() const;

  
  
  bool AppendFontFaceRules(nsPresContext* aPresContext,
                             nsTArray<nsFontFaceRuleContainer>& aArray);

  bool AppendKeyframesRules(nsPresContext* aPresContext,
                              nsTArray<nsCSSKeyframesRule*>& aArray);

#ifdef DEBUG
  void AssertQuirksChangeOK() {
    NS_ASSERTION(!mRuleCascades, "can't toggle quirks style sheet without "
                                 "clearing rule cascades");
  }
#endif

#ifdef XP_WIN
  
  static PRUint8 GetWindowsThemeIdentifier();
  static void SetWindowsThemeIdentifier(PRUint8 aId) { 
    sWinThemeId = aId;
  }
#endif

private:
  static bool CascadeSheet(nsCSSStyleSheet* aSheet, CascadeEnumData* aData);

  RuleCascadeData* GetRuleCascade(nsPresContext* aPresContext);
  void RefreshRuleCascade(nsPresContext* aPresContext);

  
  sheet_array_type mSheets;

  
  RuleCascadeData* mRuleCascades;

  
  nsPresContext *mLastPresContext;
  
  
  PRUint8 mSheetType;  

#ifdef XP_WIN
  static PRUint8 sWinThemeId;
#endif
};

#endif 
