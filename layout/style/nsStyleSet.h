










#ifndef nsStyleSet_h_
#define nsStyleSet_h_

#include "mozilla/Attributes.h"

#include "nsIStyleRuleProcessor.h"
#include "nsCSSStyleSheet.h"
#include "nsBindingManager.h"
#include "nsRuleNode.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"
#include "nsIStyleRule.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSAnonBoxes.h"
#include "mozilla/Attributes.h"

class nsIURI;
class nsCSSFontFaceRule;
class nsCSSKeyframesRule;
class nsCSSPageRule;
class nsRuleWalker;
struct ElementDependentRuleProcessorData;
struct TreeMatchContext;

class nsEmptyStyleRule MOZ_FINAL : public nsIStyleRule
{
  NS_DECL_ISUPPORTS
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif
};

class nsInitialStyleRule MOZ_FINAL : public nsIStyleRule
{
  NS_DECL_ISUPPORTS
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif
};





class nsStyleSet
{
 public:
  nsStyleSet();

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  
  

  nsresult Init(nsPresContext *aPresContext);

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

  
  struct RuleAndLevel
  {
    nsIStyleRule* mRule;
    uint8_t mLevel;
  };

  
  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleForRules(nsStyleContext* aParentContext,
                       nsStyleContext* aOldStyle,
                       const nsTArray<RuleAndLevel>& aRules);

  
  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleByAddingRules(nsStyleContext* aBaseContext,
                            const nsCOMArray<nsIStyleRule> &aRules);

  
  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleForNonElement(nsStyleContext* aParentContext);

  
  
  
  already_AddRefed<nsStyleContext>
  ResolvePseudoElementStyle(mozilla::dom::Element* aParentElement,
                            nsCSSPseudoElements::Type aType,
                            nsStyleContext* aParentContext);

  
  
  
  already_AddRefed<nsStyleContext>
  ProbePseudoElementStyle(mozilla::dom::Element* aParentElement,
                          nsCSSPseudoElements::Type aType,
                          nsStyleContext* aParentContext);
  already_AddRefed<nsStyleContext>
  ProbePseudoElementStyle(mozilla::dom::Element* aParentElement,
                          nsCSSPseudoElements::Type aType,
                          nsStyleContext* aParentContext,
                          TreeMatchContext& aTreeMatchContext);
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveAnonymousBoxStyle(nsIAtom* aPseudoTag, nsStyleContext* aParentContext);

#ifdef MOZ_XUL
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveXULTreePseudoStyle(mozilla::dom::Element* aParentElement,
                            nsIAtom* aPseudoTag,
                            nsStyleContext* aParentContext,
                            nsICSSPseudoComparator* aComparator);
#endif

  
  
  bool AppendFontFaceRules(nsPresContext* aPresContext,
                             nsTArray<nsFontFaceRuleContainer>& aArray);

  
  
  bool AppendKeyframesRules(nsPresContext* aPresContext,
                              nsTArray<nsCSSKeyframesRule*>& aArray);

  
  
  bool AppendPageRules(nsPresContext* aPresContext,
                       nsTArray<nsCSSPageRule*>& aArray);

  
  
  void BeginShutdown(nsPresContext* aPresContext);

  
  void Shutdown(nsPresContext* aPresContext);

  
  void NotifyStyleContextDestroyed(nsPresContext* aPresContext,
                                   nsStyleContext* aStyleContext);

  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ReparentStyleContext(nsStyleContext* aStyleContext,
                       nsStyleContext* aNewParentContext,
                       mozilla::dom::Element* aElement);

  
  bool HasDocumentStateDependentStyle(nsPresContext* aPresContext,
                                        nsIContent*    aContent,
                                        nsEventStates  aStateMask);

  
  nsRestyleHint HasStateDependentStyle(nsPresContext* aPresContext,
                                       mozilla::dom::Element* aElement,
                                       nsEventStates aStateMask);

  
  nsRestyleHint HasAttributeDependentStyle(nsPresContext* aPresContext,
                                           mozilla::dom::Element* aElement,
                                           nsIAtom*       aAttribute,
                                           int32_t        aModType,
                                           bool           aAttrHasChanged);

  




  bool MediumFeaturesChanged(nsPresContext* aPresContext);

  
  
  void SetBindingManager(nsBindingManager* aBindingManager)
  {
    mBindingManager = aBindingManager;
  }

  
  
  enum sheetType {
    eAgentSheet, 
    eUserSheet, 
    ePresHintSheet,
    eDocSheet, 
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

  
  bool GetAuthorStyleDisabled();
  nsresult SetAuthorStyleDisabled(bool aStyleDisabled);

  int32_t SheetCount(sheetType aType) const {
    return mSheets[aType].Count();
  }

  nsIStyleSheet* StyleSheetAt(sheetType aType, int32_t aIndex) const {
    return mSheets[aType].ObjectAt(aIndex);
  }

  nsresult AddDocStyleSheet(nsIStyleSheet* aSheet, nsIDocument* aDocument);

  void     BeginUpdate();
  nsresult EndUpdate();

  
  
  
  nsresult BeginReconstruct();
  
  void EndReconstruct();

  
  
  
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

  nsCSSStyleSheet::EnsureUniqueInnerResult EnsureUniqueInnerOnCSSSheets();

  nsIStyleRule* InitialStyleRule();

 private:
  nsStyleSet(const nsStyleSet& aCopy) MOZ_DELETE;
  nsStyleSet& operator=(const nsStyleSet& aCopy) MOZ_DELETE;

  
  void GCRuleTrees();

  
  nsresult GatherRuleProcessors(sheetType aType);

  void AddImportantRules(nsRuleNode* aCurrLevelNode,
                         nsRuleNode* aLastPrevLevelNode,
                         nsRuleWalker* aRuleWalker);

  
  
  void WalkRestrictionRule(nsCSSPseudoElements::Type aPseudoType,
                           nsRuleWalker* aRuleWalker);

#ifdef DEBUG
  
  
  
  void AssertNoImportantRules(nsRuleNode* aCurrLevelNode,
                              nsRuleNode* aLastPrevLevelNode);
  
  
  
  
  void AssertNoCSSRules(nsRuleNode* aCurrLevelNode,
                        nsRuleNode* aLastPrevLevelNode);
#endif
  
  
  
  
  
  
  void FileRules(nsIStyleRuleProcessor::EnumFunc aCollectorFunc,
                 RuleProcessorData* aData, nsIContent* aContent,
                 nsRuleWalker* aRuleWalker);

  
  
  void WalkRuleProcessors(nsIStyleRuleProcessor::EnumFunc aFunc,
                          ElementDependentRuleProcessorData* aData,
                          bool aWalkAllXBLStylesheets);

  already_AddRefed<nsStyleContext>
  GetContext(nsStyleContext* aParentContext,
             nsRuleNode* aRuleNode,
             nsRuleNode* aVisitedRuleNode,
             bool aIsLink,
             bool aIsVisitedLink,
             nsIAtom* aPseudoTag,
             nsCSSPseudoElements::Type aPseudoType,
             bool aDoAnimation,
             mozilla::dom::Element* aElementForAnimation);

  nsPresContext* PresContext() { return mRuleTree->GetPresContext(); }

  
  
  nsCOMArray<nsIStyleSheet> mSheets[eSheetTypeCount];

  nsCOMPtr<nsIStyleRuleProcessor> mRuleProcessors[eSheetTypeCount];

  
  nsCOMPtr<nsIStyleSheet> mQuirkStyleSheet;

  nsRefPtr<nsBindingManager> mBindingManager;

  nsRuleNode* mRuleTree; 
                         
                         

  uint16_t mBatching;

  unsigned mInShutdown : 1;
  unsigned mAuthorStyleDisabled: 1;
  unsigned mInReconstruct : 1;
  unsigned mDirty : 8;  

  uint32_t mUnusedRuleNodeCount; 
  nsTArray<nsStyleContext*> mRoots; 

  
  
  nsRefPtr<nsEmptyStyleRule> mFirstLineRule, mFirstLetterRule, mPlaceholderRule;

  
  
  nsRefPtr<nsInitialStyleRule> mInitialStyleRule;

  
  
  
  nsTArray<nsRuleNode*> mOldRuleTrees;
};

#ifdef _IMPL_NS_LAYOUT
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
