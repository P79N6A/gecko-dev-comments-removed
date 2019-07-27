










#ifndef nsStyleSet_h_
#define nsStyleSet_h_

#include "mozilla/Attributes.h"
#include "mozilla/CSSStyleSheet.h"
#include "mozilla/MemoryReporting.h"

#include "nsIStyleRuleProcessor.h"
#include "nsBindingManager.h"
#include "nsRuleNode.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"
#include "nsIStyleRule.h"
#include "nsCSSPseudoElements.h"

class gfxFontFeatureValueSet;
class nsCSSFontFaceRule;
class nsCSSKeyframesRule;
class nsCSSFontFeatureValuesRule;
class nsCSSPageRule;
class nsCSSCounterStyleRule;
class nsRuleWalker;
struct ElementDependentRuleProcessorData;
struct TreeMatchContext;

namespace mozilla {
class EventStates;
} 

class nsEmptyStyleRule final : public nsIStyleRule
{
private:
  ~nsEmptyStyleRule() {}

public:
  NS_DECL_ISUPPORTS
  virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const override;
#endif
};

class nsInitialStyleRule final : public nsIStyleRule
{
private:
  ~nsInitialStyleRule() {}

public:
  NS_DECL_ISUPPORTS
  virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const override;
#endif
};

class nsDisableTextZoomStyleRule final : public nsIStyleRule
{
private:
  ~nsDisableTextZoomStyleRule() {}

public:
  NS_DECL_ISUPPORTS
  virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const override;
#endif
};





class nsStyleSet
{
 public:
  nsStyleSet();

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  void Init(nsPresContext *aPresContext);

  nsRuleNode* GetRuleTree() { return mRuleTree; }

  
  void EnableQuirkStyleSheet(bool aEnable);

  
  already_AddRefed<nsStyleContext>
  ResolveStyleFor(mozilla::dom::Element* aElement,
                  nsStyleContext* aParentContext);

  already_AddRefed<nsStyleContext>
  ResolveStyleFor(mozilla::dom::Element* aElement,
                  nsStyleContext* aParentContext,
                  TreeMatchContext& aTreeMatchContext);

  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleForRules(nsStyleContext* aParentContext,
                       const nsTArray< nsCOMPtr<nsIStyleRule> > &aRules);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleByAddingRules(nsStyleContext* aBaseContext,
                            const nsCOMArray<nsIStyleRule> &aRules);

  
  
  
  
  
  enum { 
    
    eSkipStartingAnimations = (1<<0),
  };
  already_AddRefed<nsStyleContext>
  ResolveStyleWithReplacement(mozilla::dom::Element* aElement,
                              mozilla::dom::Element* aPseudoElement,
                              nsStyleContext* aNewParentContext,
                              nsStyleContext* aOldStyleContext,
                              nsRestyleHint aReplacements,
                              uint32_t aFlags = 0);

  
  
  
  
  already_AddRefed<nsStyleContext>
    ResolveStyleWithoutAnimation(mozilla::dom::Element* aElement,
                                 nsStyleContext* aStyleContext,
                                 nsRestyleHint aWhichToRemove);

  
  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleForNonElement(nsStyleContext* aParentContext,
                            bool aSuppressLineBreak = false);

  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolvePseudoElementStyle(mozilla::dom::Element* aParentElement,
                            nsCSSPseudoElements::Type aType,
                            nsStyleContext* aParentContext,
                            mozilla::dom::Element* aPseudoElement);

  
  
  
  already_AddRefed<nsStyleContext>
  ProbePseudoElementStyle(mozilla::dom::Element* aParentElement,
                          nsCSSPseudoElements::Type aType,
                          nsStyleContext* aParentContext);
  already_AddRefed<nsStyleContext>
  ProbePseudoElementStyle(mozilla::dom::Element* aParentElement,
                          nsCSSPseudoElements::Type aType,
                          nsStyleContext* aParentContext,
                          TreeMatchContext& aTreeMatchContext,
                          mozilla::dom::Element* aPseudoElement = nullptr);

  
  
  already_AddRefed<nsStyleContext>
  ResolveAnonymousBoxStyle(nsIAtom* aPseudoTag, nsStyleContext* aParentContext);

#ifdef MOZ_XUL
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveXULTreePseudoStyle(mozilla::dom::Element* aParentElement,
                            nsIAtom* aPseudoTag,
                            nsStyleContext* aParentContext,
                            nsICSSPseudoComparator* aComparator);
#endif

  
  
  bool AppendFontFaceRules(nsTArray<nsFontFaceRuleContainer>& aArray);

  
  nsCSSKeyframesRule* KeyframesRuleForName(const nsString& aName);

  
  nsCSSCounterStyleRule* CounterStyleRuleForName(const nsAString& aName);

  
  already_AddRefed<gfxFontFeatureValueSet> GetFontFeatureValuesLookup();

  
  
  bool AppendFontFeatureValuesRules(
                              nsTArray<nsCSSFontFeatureValuesRule*>& aArray);

  
  
  bool AppendPageRules(nsTArray<nsCSSPageRule*>& aArray);

  
  
  void BeginShutdown();

  
  void Shutdown();

  
  void NotifyStyleContextDestroyed(nsStyleContext* aStyleContext);

  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ReparentStyleContext(nsStyleContext* aStyleContext,
                       nsStyleContext* aNewParentContext,
                       mozilla::dom::Element* aElement);

  
  bool HasDocumentStateDependentStyle(nsIContent*    aContent,
                                      mozilla::EventStates aStateMask);

  
  nsRestyleHint HasStateDependentStyle(mozilla::dom::Element* aElement,
                                       mozilla::EventStates aStateMask);
  nsRestyleHint HasStateDependentStyle(mozilla::dom::Element* aElement,
                                       nsCSSPseudoElements::Type aPseudoType,
                                       mozilla::dom::Element* aPseudoElement,
                                       mozilla::EventStates aStateMask);

  
  nsRestyleHint HasAttributeDependentStyle(mozilla::dom::Element* aElement,
                                           nsIAtom*       aAttribute,
                                           int32_t        aModType,
                                           bool           aAttrHasChanged);

  




  bool MediumFeaturesChanged();

  
  
  void SetBindingManager(nsBindingManager* aBindingManager)
  {
    mBindingManager = aBindingManager;
  }

  
  
  enum sheetType {
    eAgentSheet, 
    eUserSheet, 
    ePresHintSheet,
    eSVGAttrAnimationSheet,
    eDocSheet, 
    eScopedDocSheet,
    eStyleAttrSheet,
    eOverrideSheet, 
    eAnimationSheet,
    eTransitionSheet,
    eSheetTypeCount
    
    
    
  };

  
  
  nsresult AppendStyleSheet(sheetType aType, nsIStyleSheet *aSheet);
  nsresult PrependStyleSheet(sheetType aType, nsIStyleSheet *aSheet);
  nsresult RemoveStyleSheet(sheetType aType, nsIStyleSheet *aSheet);
  nsresult ReplaceSheets(sheetType aType,
                         const nsCOMArray<nsIStyleSheet> &aNewSheets);
  nsresult InsertStyleSheetBefore(sheetType aType, nsIStyleSheet *aNewSheet,
                                  nsIStyleSheet *aReferenceSheet);

  nsresult DirtyRuleProcessors(sheetType aType);

  
  bool GetAuthorStyleDisabled();
  nsresult SetAuthorStyleDisabled(bool aStyleDisabled);

  int32_t SheetCount(sheetType aType) const {
    return mSheets[aType].Count();
  }

  nsIStyleSheet* StyleSheetAt(sheetType aType, int32_t aIndex) const {
    return mSheets[aType].ObjectAt(aIndex);
  }

  nsresult RemoveDocStyleSheet(nsIStyleSheet* aSheet);
  nsresult AddDocStyleSheet(nsIStyleSheet* aSheet, nsIDocument* aDocument);

  void     BeginUpdate();
  nsresult EndUpdate();

  
  
  
  nsresult BeginReconstruct();
  
  void EndReconstruct();

  bool IsInRuleTreeReconstruct() const {
    return mInReconstruct;
  }

  
  
  
  void SetQuirkStyleSheet(nsIStyleSheet* aQuirkStyleSheet);

  
  
  
  
  
  
  bool HasCachedStyleData() const {
    return (mRuleTree && mRuleTree->TreeHasCachedData()) || !mRoots.IsEmpty();
  }

  
  
  void RuleNodeUnused() {
    ++mUnusedRuleNodeCount;
  }

  
  void RuleNodeInUse() {
    --mUnusedRuleNodeCount;
  }

  mozilla::CSSStyleSheet::EnsureUniqueInnerResult EnsureUniqueInnerOnCSSSheets();

  nsIStyleRule* InitialStyleRule();

 private:
  nsStyleSet(const nsStyleSet& aCopy) = delete;
  nsStyleSet& operator=(const nsStyleSet& aCopy) = delete;

  
  void GCRuleTrees();

  
  nsresult GatherRuleProcessors(sheetType aType);

  void AddImportantRules(nsRuleNode* aCurrLevelNode,
                         nsRuleNode* aLastPrevLevelNode,
                         nsRuleWalker* aRuleWalker);

  
  
  void WalkRestrictionRule(nsCSSPseudoElements::Type aPseudoType,
                           nsRuleWalker* aRuleWalker);

  void WalkDisableTextZoomRule(mozilla::dom::Element* aElement,
                               nsRuleWalker* aRuleWalker);

#ifdef DEBUG
  
  
  
  void AssertNoImportantRules(nsRuleNode* aCurrLevelNode,
                              nsRuleNode* aLastPrevLevelNode);
  
  
  
  
  void AssertNoCSSRules(nsRuleNode* aCurrLevelNode,
                        nsRuleNode* aLastPrevLevelNode);
#endif
  
  
  
  
  
  
  void FileRules(nsIStyleRuleProcessor::EnumFunc aCollectorFunc,
                 RuleProcessorData* aData, mozilla::dom::Element* aElement,
                 nsRuleWalker* aRuleWalker);

  
  
  void WalkRuleProcessors(nsIStyleRuleProcessor::EnumFunc aFunc,
                          ElementDependentRuleProcessorData* aData,
                          bool aWalkAllXBLStylesheets);

  
  
  
  nsRuleNode* RuleNodeWithReplacement(mozilla::dom::Element* aElement,
                                      mozilla::dom::Element* aPseudoElement,
                                      nsRuleNode* aOldRuleNode,
                                      nsCSSPseudoElements::Type aPseudoType,
                                      nsRestyleHint aReplacements);

  


  enum {
    eNoFlags =          0,
    eIsLink =           1 << 0,
    eIsVisitedLink =    1 << 1,
    eDoAnimation =      1 << 2,

    
    
    
    
    
    eSkipParentDisplayBasedStyleFixup = 1 << 3,

    
    eSuppressLineBreak = 1 << 4
  };

  already_AddRefed<nsStyleContext>
  GetContext(nsStyleContext* aParentContext,
             nsRuleNode* aRuleNode,
             nsRuleNode* aVisitedRuleNode,
             nsIAtom* aPseudoTag,
             nsCSSPseudoElements::Type aPseudoType,
             mozilla::dom::Element* aElementForAnimation,
             uint32_t aFlags);

  nsPresContext* PresContext() { return mRuleTree->PresContext(); }

  
  
  
  
  
  nsCOMArray<nsIStyleSheet> mSheets[eSheetTypeCount];

  
  
  nsCOMPtr<nsIStyleRuleProcessor> mRuleProcessors[eSheetTypeCount];

  
  nsTArray<nsCOMPtr<nsIStyleRuleProcessor> > mScopedDocSheetRuleProcessors;

  
  nsCOMPtr<nsIStyleSheet> mQuirkStyleSheet;

  nsRefPtr<nsBindingManager> mBindingManager;

  nsRuleNode* mRuleTree; 
                         
                         

  uint16_t mBatching;

  unsigned mInShutdown : 1;
  unsigned mAuthorStyleDisabled: 1;
  unsigned mInReconstruct : 1;
  unsigned mInitFontFeatureValuesLookup : 1;
  unsigned mDirty : 10;  

  uint32_t mUnusedRuleNodeCount; 
  nsTArray<nsStyleContext*> mRoots; 

  
  
  nsRefPtr<nsEmptyStyleRule> mFirstLineRule, mFirstLetterRule, mPlaceholderRule;

  
  
  nsRefPtr<nsInitialStyleRule> mInitialStyleRule;

  
  
  nsRefPtr<nsDisableTextZoomStyleRule> mDisableTextZoomStyleRule;

  
  
  
  nsTArray<nsRuleNode*> mOldRuleTrees;

  
  nsRefPtr<gfxFontFeatureValueSet> mFontFeatureValuesLookup;
};

#ifdef MOZILLA_INTERNAL_API
inline
void nsRuleNode::AddRef()
{
  if (mRefCnt++ == 0 && !IsRoot()) {
    mPresContext->StyleSet()->RuleNodeInUse();
  }
}

inline
void nsRuleNode::Release()
{
  if (--mRefCnt == 0 && !IsRoot()) {
    mPresContext->StyleSet()->RuleNodeUnused();
  }
}
#endif

#endif
