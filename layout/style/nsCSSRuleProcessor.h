










#ifndef nsCSSRuleProcessor_h_
#define nsCSSRuleProcessor_h_

#include "mozilla/Attributes.h"
#include "mozilla/EventStates.h"
#include "mozilla/MemoryReporting.h"
#include "nsIStyleRuleProcessor.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsRuleWalker.h"

struct CascadeEnumData;
struct nsCSSSelector;
struct nsCSSSelectorList;
struct RuleCascadeData;
struct TreeMatchContext;
class nsCSSKeyframesRule;
class nsCSSPageRule;
class nsCSSFontFeatureValuesRule;
class nsCSSStyleSheet;
class nsCSSCounterStyleRule;












class nsCSSRuleProcessor: public nsIStyleRuleProcessor {
public:
  typedef nsTArray<nsRefPtr<nsCSSStyleSheet> > sheet_array_type;

  
  
  nsCSSRuleProcessor(const sheet_array_type& aSheets,
                     uint8_t aSheetType,
                     mozilla::dom::Element* aScopeElement);
  virtual ~nsCSSRuleProcessor();

  NS_DECL_ISUPPORTS

public:
  nsresult ClearRuleCascades();

  static nsresult Startup();
  static void Shutdown();
  static void FreeSystemMetrics();
  static bool HasSystemMetric(nsIAtom* aMetric);

  






  static bool SelectorListMatches(mozilla::dom::Element* aElement,
                                    TreeMatchContext& aTreeMatchContext,
                                    nsCSSSelectorList* aSelectorList);

  



  static mozilla::EventStates GetContentState(
                                mozilla::dom::Element* aElement,
                                const TreeMatchContext& aTreeMatchContext);

  


  static mozilla::EventStates GetContentStateForVisitedHandling(
             mozilla::dom::Element* aElement,
             const TreeMatchContext& aTreeMatchContext,
             nsRuleWalker::VisitedHandlingType aVisitedHandling,
             bool aIsRelevantLink);

  


  static bool IsLink(mozilla::dom::Element* aElement);

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) MOZ_OVERRIDE;

  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) MOZ_OVERRIDE;

  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) MOZ_OVERRIDE;

#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) MOZ_OVERRIDE;
#endif

  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) MOZ_OVERRIDE;

  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;

  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) MOZ_OVERRIDE;

  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) MOZ_OVERRIDE;

  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  
  
  bool AppendFontFaceRules(nsPresContext* aPresContext,
                           nsTArray<nsFontFaceRuleContainer>& aArray);

  nsCSSKeyframesRule* KeyframesRuleForName(nsPresContext* aPresContext,
                                           const nsString& aName);

  nsCSSCounterStyleRule* CounterStyleRuleForName(nsPresContext* aPresContext,
                                                 const nsAString& aName);

  bool AppendPageRules(nsPresContext* aPresContext,
                       nsTArray<nsCSSPageRule*>& aArray);

  bool AppendFontFeatureValuesRules(nsPresContext* aPresContext,
                              nsTArray<nsCSSFontFeatureValuesRule*>& aArray);

  




  mozilla::dom::Element* GetScopeElement() const { return mScopeElement; }

#ifdef DEBUG
  void AssertQuirksChangeOK() {
    NS_ASSERTION(!mRuleCascades, "can't toggle quirks style sheet without "
                                 "clearing rule cascades");
  }
#endif

#ifdef XP_WIN
  
  static uint8_t GetWindowsThemeIdentifier();
  static void SetWindowsThemeIdentifier(uint8_t aId) { 
    sWinThemeId = aId;
  }
#endif

  struct StateSelector {
    StateSelector(mozilla::EventStates aStates, nsCSSSelector* aSelector)
      : mStates(aStates),
        mSelector(aSelector)
    {}

    mozilla::EventStates mStates;
    nsCSSSelector* mSelector;
  };

private:
  static bool CascadeSheet(nsCSSStyleSheet* aSheet, CascadeEnumData* aData);

  RuleCascadeData* GetRuleCascade(nsPresContext* aPresContext);
  void RefreshRuleCascade(nsPresContext* aPresContext);

  nsRestyleHint HasStateDependentStyle(ElementDependentRuleProcessorData* aData,
                                       mozilla::dom::Element* aStatefulElement,
                                       nsCSSPseudoElements::Type aPseudoType,
                                       mozilla::EventStates aStateMask);

  
  sheet_array_type mSheets;

  
  RuleCascadeData* mRuleCascades;

  
  nsPresContext *mLastPresContext;

  
  
  nsRefPtr<mozilla::dom::Element> mScopeElement;

  
  uint8_t mSheetType;  

#ifdef XP_WIN
  static uint8_t sWinThemeId;
#endif
};

#endif 
