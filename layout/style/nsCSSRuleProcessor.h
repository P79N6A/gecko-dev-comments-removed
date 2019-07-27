










#ifndef nsCSSRuleProcessor_h_
#define nsCSSRuleProcessor_h_

#include "mozilla/Attributes.h"
#include "mozilla/EventStates.h"
#include "mozilla/MemoryReporting.h"
#include "nsIStyleRuleProcessor.h"
#include "nsIMediaList.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsExpirationTracker.h"
#include "nsRuleWalker.h"
#include "mozilla/RefCountType.h"
#include "mozilla/UniquePtr.h"

struct CascadeEnumData;
struct ElementDependentRuleProcessorData;
struct nsCSSSelector;
struct nsCSSSelectorList;
struct nsFontFaceRuleContainer;
struct RuleCascadeData;
struct TreeMatchContext;
class nsCSSKeyframesRule;
class nsCSSPageRule;
class nsCSSFontFeatureValuesRule;
class nsCSSCounterStyleRule;

namespace mozilla {
class CSSStyleSheet;
namespace css {
class DocumentRule;
} 
} 












class nsCSSRuleProcessor: public nsIStyleRuleProcessor {
public:
  typedef nsTArray<nsRefPtr<mozilla::CSSStyleSheet>> sheet_array_type;

  
  
  
  
  nsCSSRuleProcessor(const sheet_array_type& aSheets,
                     uint8_t aSheetType,
                     mozilla::dom::Element* aScopeElement,
                     nsCSSRuleProcessor* aPreviousCSSRuleProcessor,
                     bool aIsShared = false);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsCSSRuleProcessor)

public:
  nsresult ClearRuleCascades();

  static void Startup();
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

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) override;

  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) override;

  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) override;

#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) override;
#endif

  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) override;

  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) override;

  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) override;

  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) override;

  



  mozilla::UniquePtr<nsMediaQueryResultCacheKey> CloneMQCacheKey();

  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
    const MOZ_MUST_OVERRIDE override;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf)
    const MOZ_MUST_OVERRIDE override;

  
  
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

  void TakeDocumentRulesAndCacheKey(
      nsPresContext* aPresContext,
      nsTArray<mozilla::css::DocumentRule*>& aDocumentRules,
      nsDocumentRuleResultCacheKey& aDocumentRuleResultCacheKey);

  bool IsShared() const { return mIsShared; }

  nsExpirationState* GetExpirationState() { return &mExpirationState; }
  void AddStyleSetRef();
  void ReleaseStyleSetRef();
  void SetInRuleProcessorCache(bool aVal) {
    MOZ_ASSERT(mIsShared);
    mInRuleProcessorCache = aVal;
  }
  bool IsInRuleProcessorCache() const { return mInRuleProcessorCache; }
  bool IsUsedByMultipleStyleSets() const { return mStyleSetRefCnt > 1; }

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

protected:
  virtual ~nsCSSRuleProcessor();

private:
  static bool CascadeSheet(mozilla::CSSStyleSheet* aSheet,
                           CascadeEnumData* aData);

  RuleCascadeData* GetRuleCascade(nsPresContext* aPresContext);
  void RefreshRuleCascade(nsPresContext* aPresContext);

  nsRestyleHint HasStateDependentStyle(ElementDependentRuleProcessorData* aData,
                                       mozilla::dom::Element* aStatefulElement,
                                       nsCSSPseudoElements::Type aPseudoType,
                                       mozilla::EventStates aStateMask);

  void ClearSheets();

  
  sheet_array_type mSheets;

  
  RuleCascadeData* mRuleCascades;

  
  
  
  mozilla::UniquePtr<nsMediaQueryResultCacheKey> mPreviousCacheKey;

  
  nsPresContext *mLastPresContext;

  
  
  nsRefPtr<mozilla::dom::Element> mScopeElement;

  nsTArray<mozilla::css::DocumentRule*> mDocumentRules;
  nsDocumentRuleResultCacheKey mDocumentCacheKey;

  nsExpirationState mExpirationState;
  MozRefCountType mStyleSetRefCnt;

  
  uint8_t mSheetType;  

  const bool mIsShared;

  
  
  
  
  bool mMustGatherDocumentRules;

  bool mInRuleProcessorCache;

#ifdef DEBUG
  bool mDocumentRulesAndCacheKeyValid;
#endif

#ifdef XP_WIN
  static uint8_t sWinThemeId;
#endif
};

#endif 
