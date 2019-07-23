













































#ifndef nsStyleSet_h_
#define nsStyleSet_h_

#include "nsIStyleRuleProcessor.h"
#include "nsICSSStyleSheet.h"
#include "nsBindingManager.h"
#include "nsRuleNode.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"
#include "nsIStyleRule.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSAnonBoxes.h"

class nsIURI;
class nsCSSFontFaceRule;
class nsRuleWalker;
struct RuleProcessorData;

class nsEmptyStyleRule : public nsIStyleRule
{
  NS_DECL_ISUPPORTS
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif
};





class nsStyleSet
{
 public:
  nsStyleSet();

  
  

  nsresult Init(nsPresContext *aPresContext);

  
  
  nsCachedStyleData* DefaultStyleData() { return &mDefaultStyleData; }

  nsRuleNode* GetRuleTree() { return mRuleTree; }

  
  void EnableQuirkStyleSheet(PRBool aEnable);

  
  already_AddRefed<nsStyleContext>
  ResolveStyleFor(nsIContent* aContent, nsStyleContext* aParentContext);

  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleForRules(nsStyleContext* aParentContext,
                       nsIAtom* aPseudoTag,
                       nsCSSPseudoElements::Type aPseudoType,
                       nsRuleNode *aRuleNode,
                       const nsCOMArray<nsIStyleRule> &aRules);

  
  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleForNonElement(nsStyleContext* aParentContext);

  
  
  
  already_AddRefed<nsStyleContext>
  ResolvePseudoElementStyle(nsIContent* aParentContent,
                            nsCSSPseudoElements::Type aType,
                            nsStyleContext* aParentContext);

  
  
  
  already_AddRefed<nsStyleContext>
  ProbePseudoElementStyle(nsIContent* aParentContent,
                          nsCSSPseudoElements::Type aType,
                          nsStyleContext* aParentContext);
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveAnonymousBoxStyle(nsIAtom* aPseudoTag, nsStyleContext* aParentContext);

#ifdef MOZ_XUL
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveXULTreePseudoStyle(nsIContent* aParentContent,
                            nsIAtom* aPseudoTag,
                            nsStyleContext* aParentContext,
                            nsICSSPseudoComparator* aComparator) {
    NS_PRECONDITION(nsCSSAnonBoxes::IsTreePseudoElement(aPseudoTag),
                    "Unexpected pseudo");
    return ResolvePseudoStyleFor(aParentContent, aPseudoTag,
                                 nsCSSPseudoElements::ePseudo_XULTree,
                                 aParentContext, aComparator);
  }
#endif

private:
  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolvePseudoStyleFor(nsIContent* aParentContent,
                        nsIAtom* aPseudoTag,
                        nsCSSPseudoElements::Type aPseudoType,
                        nsStyleContext* aParentContext,
                        nsICSSPseudoComparator* aComparator = nsnull);
public:
  
  
  PRBool AppendFontFaceRules(nsPresContext* aPresContext,
                             nsTArray<nsFontFaceRuleContainer>& aArray);

  
  
  void BeginShutdown(nsPresContext* aPresContext);

  
  void Shutdown(nsPresContext* aPresContext);

  
  void NotifyStyleContextDestroyed(nsPresContext* aPresContext,
                                   nsStyleContext* aStyleContext);

  
  
  
  already_AddRefed<nsStyleContext>
    ReParentStyleContext(nsPresContext* aPresContext,
                         nsStyleContext* aStyleContext,
                         nsStyleContext* aNewParentContext);

  
  nsReStyleHint HasStateDependentStyle(nsPresContext* aPresContext,
                                       nsIContent*     aContent,
                                       PRInt32         aStateMask);

  
  nsReStyleHint HasAttributeDependentStyle(nsPresContext* aPresContext,
                                           nsIContent*    aContent,
                                           nsIAtom*       aAttribute,
                                           PRInt32        aModType,
                                           PRBool         aAttrHasChanged);

  




  PRBool MediumFeaturesChanged(nsPresContext* aPresContext);

  
  
  void SetBindingManager(nsBindingManager* aBindingManager)
  {
    mBindingManager = aBindingManager;
  }

  
  
  enum sheetType {
    eAgentSheet, 
    ePresHintSheet,
    eUserSheet, 
    eHTMLPresHintSheet,
    eDocSheet, 
    eStyleAttrSheet,
    eOverrideSheet, 
    eTransitionSheet,
    eSheetTypeCount
    
    
    
  };

  
  
  nsresult AppendStyleSheet(sheetType aType, nsIStyleSheet *aSheet);
  nsresult PrependStyleSheet(sheetType aType, nsIStyleSheet *aSheet);
  nsresult RemoveStyleSheet(sheetType aType, nsIStyleSheet *aSheet);
  nsresult ReplaceSheets(sheetType aType,
                         const nsCOMArray<nsIStyleSheet> &aNewSheets);

  
  PRBool GetAuthorStyleDisabled();
  nsresult SetAuthorStyleDisabled(PRBool aStyleDisabled);

  PRInt32 SheetCount(sheetType aType) const {
    return mSheets[aType].Count();
  }

  nsIStyleSheet* StyleSheetAt(sheetType aType, PRInt32 aIndex) const {
    return mSheets[aType].ObjectAt(aIndex);
  }

  nsresult AddDocStyleSheet(nsIStyleSheet* aSheet, nsIDocument* aDocument);

  void     BeginUpdate();
  nsresult EndUpdate();

  
  
  
  nsresult BeginReconstruct();
  
  void EndReconstruct();

  
  
  
  void SetQuirkStyleSheet(nsIStyleSheet* aQuirkStyleSheet);

  
  
  
  
  
  
  PRBool HasCachedStyleData() const {
    return (mRuleTree && mRuleTree->TreeHasCachedData()) || !mRoots.IsEmpty();
  }

  
  
  void RuleNodeUnused() {
    ++mUnusedRuleNodeCount;
  }

  
  void RuleNodeInUse() {
    --mUnusedRuleNodeCount;
  }
  
 private:
  
  nsStyleSet(const nsStyleSet& aCopy);
  nsStyleSet& operator=(const nsStyleSet& aCopy);

  
  PRBool BuildDefaultStyleData(nsPresContext* aPresContext);

  
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
                 void* aData, nsIContent* aContent, nsRuleWalker* aRuleWalker);

  
  
  void WalkRuleProcessors(nsIStyleRuleProcessor::EnumFunc aFunc,
                          RuleProcessorData* aData);

  already_AddRefed<nsStyleContext> GetContext(nsPresContext* aPresContext,
                                              nsStyleContext* aParentContext,
                                              nsRuleNode* aRuleNode,
                                              nsIAtom* aPseudoTag,
                                              nsCSSPseudoElements::Type aPseudoType);

  nsPresContext* PresContext() { return mRuleTree->GetPresContext(); }

  
  
  nsCOMArray<nsIStyleSheet> mSheets[eSheetTypeCount];

  nsCOMPtr<nsIStyleRuleProcessor> mRuleProcessors[eSheetTypeCount];

  
  nsCOMPtr<nsIStyleSheet> mQuirkStyleSheet;

  nsRefPtr<nsBindingManager> mBindingManager;

  
  
  
  nsCachedStyleData mDefaultStyleData;

  nsRuleNode* mRuleTree; 
                         
                         

  PRUint32 mUnusedRuleNodeCount; 
  nsTArray<nsStyleContext*> mRoots; 

  
  
  nsRefPtr<nsEmptyStyleRule> mFirstLineRule, mFirstLetterRule;

  PRUint16 mBatching;

  
  
  
  nsTArray<nsRuleNode*> mOldRuleTrees;

  unsigned mInShutdown : 1;
  unsigned mAuthorStyleDisabled: 1;
  unsigned mInReconstruct : 1;
  unsigned mDirty : 8;  

};

inline
NS_HIDDEN_(void) nsRuleNode::AddRef()
{
  if (mRefCnt++ == 0 && !IsRoot()) {
    mPresContext->StyleSet()->RuleNodeInUse();
  }
}

inline
NS_HIDDEN_(void) nsRuleNode::Release()
{
  if (--mRefCnt == 0 && !IsRoot()) {
    mPresContext->StyleSet()->RuleNodeUnused();
  }
}
#endif
