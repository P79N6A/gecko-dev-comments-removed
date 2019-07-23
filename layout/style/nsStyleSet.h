













































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

class nsIURI;
class nsCSSFontFaceRule;

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

  
  void EnableQuirkStyleSheet(PRBool aEnable);

  
  already_AddRefed<nsStyleContext>
  ResolveStyleFor(nsIContent* aContent, nsStyleContext* aParentContext);

  
  already_AddRefed<nsStyleContext>
  ResolveStyleForRules(nsStyleContext* aParentContext, const nsCOMArray<nsIStyleRule> &rules);

  
  
  
  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolveStyleForNonElement(nsStyleContext* aParentContext);

  
  
  
  
  already_AddRefed<nsStyleContext>
  ResolvePseudoStyleFor(nsIContent* aParentContent,
                        nsIAtom* aPseudoTag,
                        nsStyleContext* aParentContext,
                        nsICSSPseudoComparator* aComparator = nsnull);

  
  
  
  already_AddRefed<nsStyleContext>
  ProbePseudoStyleFor(nsIContent* aParentContent,
                      nsIAtom* aPseudoTag,
                      nsStyleContext* aParentContext);

  
  
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
                                           PRUint32       aStateMask);

  




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
  
 private:
  
  nsStyleSet(const nsStyleSet& aCopy);
  nsStyleSet& operator=(const nsStyleSet& aCopy);

  
  PRBool BuildDefaultStyleData(nsPresContext* aPresContext);

  
  nsresult GatherRuleProcessors(sheetType aType);

  void AddImportantRules(nsRuleNode* aCurrLevelNode,
                         nsRuleNode* aLastPrevLevelNode);

  
  
  void WalkRestrictionRule(nsIAtom* aPseudoType);

#ifdef DEBUG
  
  
  
  void AssertNoImportantRules(nsRuleNode* aCurrLevelNode,
                              nsRuleNode* aLastPrevLevelNode);
  
  
  
  
  void AssertNoCSSRules(nsRuleNode* aCurrLevelNode,
                        nsRuleNode* aLastPrevLevelNode);
#endif
  
  
  
  void FileRules(nsIStyleRuleProcessor::EnumFunc aCollectorFunc,
                 RuleProcessorData* aData);

  
  
  void WalkRuleProcessors(nsIStyleRuleProcessor::EnumFunc aFunc,
                          RuleProcessorData* aData);

  already_AddRefed<nsStyleContext> GetContext(nsPresContext* aPresContext,
                                              nsStyleContext* aParentContext,
                                              nsIAtom* aPseudoTag);

  nsPresContext* PresContext() { return mRuleTree->GetPresContext(); }

  
  
  nsCOMArray<nsIStyleSheet> mSheets[eSheetTypeCount];

  nsCOMPtr<nsIStyleRuleProcessor> mRuleProcessors[eSheetTypeCount];

  
  nsCOMPtr<nsIStyleSheet> mQuirkStyleSheet;

  nsRefPtr<nsBindingManager> mBindingManager;

  
  
  
  nsCachedStyleData mDefaultStyleData;

  nsRuleNode* mRuleTree; 
                         
                         
  nsRuleWalker* mRuleWalker; 
                             

  PRInt32 mDestroyedCount; 
  nsTArray<nsStyleContext*> mRoots; 

  
  
  nsRefPtr<nsEmptyStyleRule> mFirstLineRule, mFirstLetterRule;

  PRUint16 mBatching;

  nsRuleNode* mOldRuleTree; 
                            

  unsigned mInShutdown : 1;
  unsigned mAuthorStyleDisabled: 1;
  unsigned mDirty : 7;  

};

#endif
