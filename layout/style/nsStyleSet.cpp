













































#include "nsStyleSet.h"
#include "nsNetUtil.h"
#include "nsICSSStyleSheet.h"
#include "nsIDocument.h"
#include "nsRuleWalker.h"
#include "nsStyleContext.h"
#include "nsICSSStyleRule.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSRuleProcessor.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsContentUtils.h"
#include "nsRuleProcessorData.h"

NS_IMPL_ISUPPORTS1(nsEmptyStyleRule, nsIStyleRule)

NS_IMETHODIMP
nsEmptyStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsEmptyStyleRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}
#endif

static const nsStyleSet::sheetType gCSSSheetTypes[] = {
  nsStyleSet::eAgentSheet,
  nsStyleSet::eUserSheet,
  nsStyleSet::eDocSheet,
  nsStyleSet::eOverrideSheet
};

nsStyleSet::nsStyleSet()
  : mRuleTree(nsnull),
    mRuleWalker(nsnull),
    mDestroyedCount(0),
    mBatching(0),
    mInShutdown(PR_FALSE),
    mAuthorStyleDisabled(PR_FALSE),
    mInReconstruct(PR_FALSE),
    mDirty(0)
{
}

nsresult
nsStyleSet::Init(nsPresContext *aPresContext)
{
  mFirstLineRule = new nsEmptyStyleRule;
  mFirstLetterRule = new nsEmptyStyleRule;
  if (!mFirstLineRule || !mFirstLetterRule) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!BuildDefaultStyleData(aPresContext)) {
    mDefaultStyleData.Destroy(0, aPresContext);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mRuleTree = nsRuleNode::CreateRootNode(aPresContext);
  if (!mRuleTree) {
    mDefaultStyleData.Destroy(0, aPresContext);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mRuleWalker = new nsRuleWalker(mRuleTree);
  if (!mRuleWalker) {
    mRuleTree->Destroy();
    mDefaultStyleData.Destroy(0, aPresContext);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

nsresult
nsStyleSet::BeginReconstruct()
{
  NS_ASSERTION(!mInReconstruct, "Unmatched begin/end?");
  NS_ASSERTION(mRuleTree, "Reconstructing before first construction?");

  
  nsRuleNode* newTree =
    nsRuleNode::CreateRootNode(mRuleTree->GetPresContext());
  if (!newTree)
    return NS_ERROR_OUT_OF_MEMORY;
  nsRuleWalker* ruleWalker = new nsRuleWalker(newTree);
  if (!ruleWalker) {
    newTree->Destroy();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  if (!mOldRuleTrees.AppendElement(mRuleTree)) {
    delete ruleWalker;
    newTree->Destroy();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  delete mRuleWalker;

  
  
  

  mInReconstruct = PR_TRUE;
  mRuleTree = newTree;
  mRuleWalker = ruleWalker;

  return NS_OK;
}

void
nsStyleSet::EndReconstruct()
{
  NS_ASSERTION(mInReconstruct, "Unmatched begin/end?");
  mInReconstruct = PR_FALSE;
#ifdef DEBUG
  for (PRInt32 i = mRoots.Length() - 1; i >= 0; --i) {
    nsRuleNode *n = mRoots[i]->GetRuleNode();
    while (n->GetParent()) {
      n = n->GetParent();
    }
    
    
    
    
    
    

    NS_ASSERTION(n == mRuleTree, "style context has old rule node");
  }
#endif
  
  
  
  GCRuleTrees();
}

void
nsStyleSet::SetQuirkStyleSheet(nsIStyleSheet* aQuirkStyleSheet)
{
  NS_ASSERTION(aQuirkStyleSheet, "Must have quirk sheet if this is called");
  NS_ASSERTION(!mQuirkStyleSheet, "Multiple calls to SetQuirkStyleSheet?");
  NS_ASSERTION(mSheets[eAgentSheet].IndexOf(aQuirkStyleSheet) != -1,
               "Quirk style sheet not one of our agent sheets?");
  mQuirkStyleSheet = aQuirkStyleSheet;
}

nsresult
nsStyleSet::GatherRuleProcessors(sheetType aType)
{
  mRuleProcessors[aType] = nsnull;
  if (mAuthorStyleDisabled && (aType == eDocSheet || 
                               aType == ePresHintSheet ||
                               aType == eHTMLPresHintSheet ||
                               aType == eStyleAttrSheet)) {
    
    return NS_OK;
  }
  if (mSheets[aType].Count()) {
    switch (aType) {
      case eAgentSheet:
      case eUserSheet:
      case eDocSheet:
      case eOverrideSheet: {
        
        nsCOMArray<nsIStyleSheet>& sheets = mSheets[aType];
        nsCOMArray<nsICSSStyleSheet> cssSheets(sheets.Count());
        for (PRInt32 i = 0, i_end = sheets.Count(); i < i_end; ++i) {
          nsCOMPtr<nsICSSStyleSheet> cssSheet = do_QueryInterface(sheets[i]);
          NS_ASSERTION(cssSheet, "not a CSS sheet");
          cssSheets.AppendObject(cssSheet);
        }
        mRuleProcessors[aType] = new nsCSSRuleProcessor(cssSheets, 
                                                        PRUint8(aType));
      } break;

      default:
        
        NS_ASSERTION(mSheets[aType].Count() == 1, "only one sheet per level");
        mRuleProcessors[aType] = do_QueryInterface(mSheets[aType][0]);
        break;
    }
  }

  return NS_OK;
}

#ifdef DEBUG
#define CHECK_APPLICABLE \
PR_BEGIN_MACRO \
  PRBool applicable = PR_TRUE; \
  aSheet->GetApplicable(applicable); \
  NS_ASSERTION(applicable, "Inapplicable sheet being placed in style set"); \
PR_END_MACRO
#else
#define CHECK_APPLICABLE
#endif

nsresult
nsStyleSet::AppendStyleSheet(sheetType aType, nsIStyleSheet *aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  CHECK_APPLICABLE;
  mSheets[aType].RemoveObject(aSheet);
  if (!mSheets[aType].AppendObject(aSheet))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mBatching)
    return GatherRuleProcessors(aType);

  mDirty |= 1 << aType;
  return NS_OK;
}

nsresult
nsStyleSet::PrependStyleSheet(sheetType aType, nsIStyleSheet *aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  CHECK_APPLICABLE;
  mSheets[aType].RemoveObject(aSheet);
  if (!mSheets[aType].InsertObjectAt(aSheet, 0))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mBatching)
    return GatherRuleProcessors(aType);

  mDirty |= 1 << aType;
  return NS_OK;
}

nsresult
nsStyleSet::RemoveStyleSheet(sheetType aType, nsIStyleSheet *aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
#ifdef DEBUG
  PRBool complete = PR_TRUE;
  aSheet->GetComplete(complete);
  NS_ASSERTION(complete, "Incomplete sheet being removed from style set");
#endif
  mSheets[aType].RemoveObject(aSheet);
  if (!mBatching)
    return GatherRuleProcessors(aType);

  mDirty |= 1 << aType;
  return NS_OK;
}

nsresult
nsStyleSet::ReplaceSheets(sheetType aType,
                          const nsCOMArray<nsIStyleSheet> &aNewSheets)
{
  mSheets[aType].Clear();
  if (!mSheets[aType].AppendObjects(aNewSheets))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mBatching)
    return GatherRuleProcessors(aType);

  mDirty |= 1 << aType;
  return NS_OK;
}

PRBool
nsStyleSet::GetAuthorStyleDisabled()
{
  return mAuthorStyleDisabled;
}

nsresult
nsStyleSet::SetAuthorStyleDisabled(PRBool aStyleDisabled)
{
  if (aStyleDisabled == !mAuthorStyleDisabled) {
    mAuthorStyleDisabled = aStyleDisabled;
    BeginUpdate();
    mDirty |= 1 << eDocSheet |
              1 << ePresHintSheet |
              1 << eHTMLPresHintSheet |
              1 << eStyleAttrSheet;
    return EndUpdate();
  }
  return NS_OK;
}



nsresult
nsStyleSet::AddDocStyleSheet(nsIStyleSheet* aSheet, nsIDocument* aDocument)
{
  NS_PRECONDITION(aSheet && aDocument, "null arg");
  CHECK_APPLICABLE;

  nsCOMArray<nsIStyleSheet>& docSheets = mSheets[eDocSheet];

  docSheets.RemoveObject(aSheet);
  
  PRInt32 newDocIndex = aDocument->GetIndexOfStyleSheet(aSheet);
  PRInt32 count = docSheets.Count();
  PRInt32 index;
  for (index = 0; index < count; index++) {
    nsIStyleSheet* sheet = docSheets.ObjectAt(index);
    PRInt32 sheetDocIndex = aDocument->GetIndexOfStyleSheet(sheet);
    if (sheetDocIndex > newDocIndex)
      break;
  }
  if (!docSheets.InsertObjectAt(aSheet, index))
    return NS_ERROR_OUT_OF_MEMORY;
  if (!mBatching)
    return GatherRuleProcessors(eDocSheet);

  mDirty |= 1 << eDocSheet;
  return NS_OK;
}

#undef CHECK_APPLICABLE


void
nsStyleSet::BeginUpdate()
{
  ++mBatching;
}

nsresult
nsStyleSet::EndUpdate()
{
  NS_ASSERTION(mBatching > 0, "Unbalanced EndUpdate");
  if (--mBatching) {
    
    return NS_OK;
  }

  for (int i = 0; i < eSheetTypeCount; ++i) {
    if (mDirty & (1 << i)) {
      nsresult rv = GatherRuleProcessors(sheetType(i));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  mDirty = 0;
  return NS_OK;
}

void
nsStyleSet::EnableQuirkStyleSheet(PRBool aEnable)
{
#ifdef DEBUG
  PRBool oldEnabled;
  {
    nsCOMPtr<nsIDOMCSSStyleSheet> domSheet =
      do_QueryInterface(mQuirkStyleSheet);
    domSheet->GetDisabled(&oldEnabled);
    oldEnabled = !oldEnabled;
  }
#endif
  mQuirkStyleSheet->SetEnabled(aEnable);
#ifdef DEBUG
  
  
  
  
  if (mRuleProcessors[eAgentSheet] && aEnable != oldEnabled) {
    static_cast<nsCSSRuleProcessor*>(static_cast<nsIStyleRuleProcessor*>(
      mRuleProcessors[eAgentSheet]))->AssertQuirksChangeOK();
  }
#endif
}

static PRBool
EnumRulesMatching(nsIStyleRuleProcessor* aProcessor, void* aData)
{
  ElementRuleProcessorData* data =
    static_cast<ElementRuleProcessorData*>(aData);

  aProcessor->RulesMatching(data);
  return PR_TRUE;
}







already_AddRefed<nsStyleContext>
nsStyleSet::GetContext(nsPresContext* aPresContext, 
                       nsStyleContext* aParentContext, 
                       nsIAtom* aPseudoTag)
{
  nsStyleContext* result = nsnull;
  nsRuleNode* ruleNode = mRuleWalker->GetCurrentNode();
      
  if (aParentContext)
    result = aParentContext->FindChildWithRules(aPseudoTag, ruleNode).get();

#ifdef NOISY_DEBUG
  if (result)
    fprintf(stdout, "--- SharedSC %d ---\n", ++gSharedCount);
  else
    fprintf(stdout, "+++ NewSC %d +++\n", ++gNewCount);
#endif

  if (!result) {
    result = NS_NewStyleContext(aParentContext, aPseudoTag, ruleNode,
                                aPresContext).get();
    if (!aParentContext && result)
      mRoots.AppendElement(result);
  }

  return result;
}

void
nsStyleSet::AddImportantRules(nsRuleNode* aCurrLevelNode,
                              nsRuleNode* aLastPrevLevelNode)
{
  if (!aCurrLevelNode)
    return;

  nsAutoTArray<nsCOMPtr<nsIStyleRule>, 16> importantRules;
  for (nsRuleNode *node = aCurrLevelNode; node != aLastPrevLevelNode;
       node = node->GetParent()) {
    nsIStyleRule *rule = node->GetRule();
    nsCOMPtr<nsICSSStyleRule> cssRule(do_QueryInterface(rule));
    if (cssRule) {
      nsCOMPtr<nsIStyleRule> impRule = cssRule->GetImportantRule();
      if (impRule)
        importantRules.AppendElement(impRule);
    }
  }

  for (PRUint32 i = importantRules.Length(); i-- != 0; ) {
    mRuleWalker->Forward(importantRules[i]);
  }
}

#ifdef DEBUG
void
nsStyleSet::AssertNoImportantRules(nsRuleNode* aCurrLevelNode,
                                   nsRuleNode* aLastPrevLevelNode)
{
  if (!aCurrLevelNode)
    return;

  for (nsRuleNode *node = aCurrLevelNode; node != aLastPrevLevelNode;
       node = node->GetParent()) {
    nsIStyleRule *rule = node->GetRule();
    nsCOMPtr<nsICSSStyleRule> cssRule(do_QueryInterface(rule));
    if (cssRule) {
      nsCOMPtr<nsIStyleRule> impRule = cssRule->GetImportantRule();
      NS_ASSERTION(!impRule, "Unexpected important rule");
    }
  }
}

void
nsStyleSet::AssertNoCSSRules(nsRuleNode* aCurrLevelNode,
                             nsRuleNode* aLastPrevLevelNode)
{
  if (!aCurrLevelNode)
    return;

  for (nsRuleNode *node = aCurrLevelNode; node != aLastPrevLevelNode;
       node = node->GetParent()) {
    nsIStyleRule *rule = node->GetRule();
    nsCOMPtr<nsICSSStyleRule> cssRule(do_QueryInterface(rule));
    NS_ASSERTION(!cssRule || !cssRule->Selector(), "Unexpected CSS rule");
  }
}
#endif


void
nsStyleSet::FileRules(nsIStyleRuleProcessor::EnumFunc aCollectorFunc, 
                      RuleProcessorData* aData)
{
  
  
  
  
  
  
  
  
  
  
  
  
  

  NS_PRECONDITION(SheetCount(ePresHintSheet) == 0 ||
                  SheetCount(eHTMLPresHintSheet) == 0,
                  "Can't have both types of preshint sheets at once!");
  
  mRuleWalker->SetLevel(eAgentSheet, PR_FALSE);
  if (mRuleProcessors[eAgentSheet])
    (*aCollectorFunc)(mRuleProcessors[eAgentSheet], aData);
  nsRuleNode* lastAgentRN = mRuleWalker->GetCurrentNode();

  mRuleWalker->SetLevel(ePresHintSheet, PR_FALSE);
  if (mRuleProcessors[ePresHintSheet])
    (*aCollectorFunc)(mRuleProcessors[ePresHintSheet], aData);
  nsRuleNode* lastPresHintRN = mRuleWalker->GetCurrentNode();

  mRuleWalker->SetLevel(eUserSheet, PR_FALSE);
  PRBool skipUserStyles =
    aData->mContent && aData->mContent->IsInNativeAnonymousSubtree();
  if (!skipUserStyles && mRuleProcessors[eUserSheet]) 
    (*aCollectorFunc)(mRuleProcessors[eUserSheet], aData);
  nsRuleNode* lastUserRN = mRuleWalker->GetCurrentNode();

  mRuleWalker->SetLevel(eHTMLPresHintSheet, PR_FALSE);
  if (mRuleProcessors[eHTMLPresHintSheet])
    (*aCollectorFunc)(mRuleProcessors[eHTMLPresHintSheet], aData);
  nsRuleNode* lastHTMLPresHintRN = mRuleWalker->GetCurrentNode();
  
  mRuleWalker->SetLevel(eDocSheet, PR_FALSE);
  PRBool cutOffInheritance = PR_FALSE;
  if (mBindingManager) {
    
    mBindingManager->WalkRules(aCollectorFunc, aData, &cutOffInheritance);
  }
  if (!skipUserStyles && !cutOffInheritance &&
      mRuleProcessors[eDocSheet]) 
    (*aCollectorFunc)(mRuleProcessors[eDocSheet], aData);
  mRuleWalker->SetLevel(eStyleAttrSheet, PR_FALSE);
  if (mRuleProcessors[eStyleAttrSheet])
    (*aCollectorFunc)(mRuleProcessors[eStyleAttrSheet], aData);
  nsRuleNode* lastDocRN = mRuleWalker->GetCurrentNode();

  mRuleWalker->SetLevel(eOverrideSheet, PR_FALSE);
  if (mRuleProcessors[eOverrideSheet])
    (*aCollectorFunc)(mRuleProcessors[eOverrideSheet], aData);
  nsRuleNode* lastOvrRN = mRuleWalker->GetCurrentNode();

  mRuleWalker->SetLevel(eDocSheet, PR_TRUE);
  AddImportantRules(lastDocRN, lastHTMLPresHintRN);  
  mRuleWalker->SetLevel(eOverrideSheet, PR_TRUE);
  AddImportantRules(lastOvrRN, lastDocRN);  
#ifdef DEBUG
  AssertNoCSSRules(lastHTMLPresHintRN, lastUserRN);
  AssertNoImportantRules(lastHTMLPresHintRN, lastUserRN); 
#endif
  mRuleWalker->SetLevel(eUserSheet, PR_TRUE);
  AddImportantRules(lastUserRN, lastPresHintRN); 
#ifdef DEBUG
  AssertNoCSSRules(lastPresHintRN, lastAgentRN);
  AssertNoImportantRules(lastPresHintRN, lastAgentRN); 
#endif
  mRuleWalker->SetLevel(eAgentSheet, PR_TRUE);
  AddImportantRules(lastAgentRN, nsnull);     

}



void
nsStyleSet::WalkRuleProcessors(nsIStyleRuleProcessor::EnumFunc aFunc,
                               RuleProcessorData* aData)
{
  NS_PRECONDITION(SheetCount(ePresHintSheet) == 0 ||
                  SheetCount(eHTMLPresHintSheet) == 0,
                  "Can't have both types of preshint sheets at once!");
  
  if (mRuleProcessors[eAgentSheet])
    (*aFunc)(mRuleProcessors[eAgentSheet], aData);
  if (mRuleProcessors[ePresHintSheet])
    (*aFunc)(mRuleProcessors[ePresHintSheet], aData);

  PRBool skipUserStyles =
    aData->mContent && aData->mContent->IsInNativeAnonymousSubtree();
  if (!skipUserStyles && mRuleProcessors[eUserSheet]) 
    (*aFunc)(mRuleProcessors[eUserSheet], aData);

  if (mRuleProcessors[eHTMLPresHintSheet])
    (*aFunc)(mRuleProcessors[eHTMLPresHintSheet], aData);
  
  PRBool cutOffInheritance = PR_FALSE;
  if (mBindingManager) {
    
    mBindingManager->WalkRules(aFunc, aData, &cutOffInheritance);
  }
  if (!skipUserStyles && !cutOffInheritance &&
      mRuleProcessors[eDocSheet]) 
    (*aFunc)(mRuleProcessors[eDocSheet], aData);
  if (mRuleProcessors[eStyleAttrSheet])
    (*aFunc)(mRuleProcessors[eStyleAttrSheet], aData);
  if (mRuleProcessors[eOverrideSheet])
    (*aFunc)(mRuleProcessors[eOverrideSheet], aData);
}

PRBool nsStyleSet::BuildDefaultStyleData(nsPresContext* aPresContext)
{
  NS_ASSERTION(!mDefaultStyleData.mResetData &&
               !mDefaultStyleData.mInheritedData,
               "leaking default style data");
  mDefaultStyleData.mResetData = new (aPresContext) nsResetStyleData;
  if (!mDefaultStyleData.mResetData)
    return PR_FALSE;
  mDefaultStyleData.mInheritedData = new (aPresContext) nsInheritedStyleData;
  if (!mDefaultStyleData.mInheritedData)
    return PR_FALSE;

#define SSARG_PRESCONTEXT aPresContext

#define CREATE_DATA(name, type, args) \
  if (!(mDefaultStyleData.m##type##Data->m##name##Data = \
          new (aPresContext) nsStyle##name args)) \
    return PR_FALSE;

#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args) \
  CREATE_DATA(name, Inherited, ctor_args)
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args) \
  CREATE_DATA(name, Reset, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET
#undef SSARG_PRESCONTEXT

  return PR_TRUE;
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleFor(nsIContent* aContent,
                            nsStyleContext* aParentContext)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);
  
  nsStyleContext*  result = nsnull;
  nsPresContext* presContext = PresContext();

  NS_ASSERTION(aContent, "must have content");
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eELEMENT),
               "content must be element");

  if (aContent && presContext) {
    NS_ASSERTION(mRuleWalker->AtRoot(), "rule walker must be at root");
    ElementRuleProcessorData data(presContext, aContent, mRuleWalker);
    FileRules(EnumRulesMatching, &data);
    result = GetContext(presContext, aParentContext, nsnull).get();

    
    mRuleWalker->Reset();
  }

  return result;
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleForRules(nsStyleContext* aParentContext, const nsCOMArray<nsIStyleRule> &rules)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);
  nsStyleContext* result = nsnull;
  nsPresContext *presContext = PresContext();

  if (presContext) {
    NS_ASSERTION(mRuleWalker->AtRoot(), "rule walker must be at root");
    mRuleWalker->SetLevel(eDocSheet, PR_FALSE);
    for (PRInt32 i = 0; i < rules.Count(); i++) {
      mRuleWalker->Forward(rules.ObjectAt(i));
    }
    result = GetContext(presContext, aParentContext, nsnull).get();

    
    mRuleWalker->Reset();
  }
  return result;
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleForNonElement(nsStyleContext* aParentContext)
{
  nsStyleContext* result = nsnull;
  nsPresContext *presContext = PresContext();

  if (presContext) {
    NS_ASSERTION(mRuleWalker->AtRoot(), "rule walker must be at root");
    result = GetContext(presContext, aParentContext,
                        nsCSSAnonBoxes::mozNonElement).get();
    NS_ASSERTION(mRuleWalker->AtRoot(), "rule walker must be at root");
  }

  return result;
}

void
nsStyleSet::WalkRestrictionRule(nsIAtom* aPseudoType)
{
  
  if (aPseudoType) {
    if (aPseudoType == nsCSSPseudoElements::firstLetter)
      mRuleWalker->Forward(mFirstLetterRule);
    else if (aPseudoType == nsCSSPseudoElements::firstLine)
      mRuleWalker->Forward(mFirstLineRule);
  }
}

static PRBool
EnumPseudoRulesMatching(nsIStyleRuleProcessor* aProcessor, void* aData)
{
  PseudoRuleProcessorData* data =
    static_cast<PseudoRuleProcessorData*>(aData);

  aProcessor->RulesMatching(data);
  return PR_TRUE;
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolvePseudoStyleFor(nsIContent* aParentContent,
                                  nsIAtom* aPseudoTag,
                                  nsStyleContext* aParentContext,
                                  nsICSSPseudoComparator* aComparator)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);

  nsStyleContext*  result = nsnull;
  nsPresContext *presContext = PresContext();

  NS_ASSERTION(aPseudoTag, "must have pseudo tag");
  NS_ASSERTION(!aParentContent ||
               aParentContent->IsNodeOfType(nsINode::eELEMENT),
               "content (if non-null) must be element");
  NS_ASSERTION(aParentContent ||
               nsCSSAnonBoxes::IsAnonBox(aPseudoTag),
               "null content must correspond to anonymous box");
  NS_ASSERTION(nsCSSAnonBoxes::IsAnonBox(aPseudoTag) ||
               nsCSSPseudoElements::IsPseudoElement(aPseudoTag),
               "aPseudoTag must be pseudo-element or anonymous box");

  if (aPseudoTag && presContext) {
    NS_ASSERTION(mRuleWalker->AtRoot(), "rule walker must be at root");
    PseudoRuleProcessorData data(presContext, aParentContent, aPseudoTag,
                                 aComparator, mRuleWalker);
    WalkRestrictionRule(aPseudoTag);
    FileRules(EnumPseudoRulesMatching, &data);

    result = GetContext(presContext, aParentContext, aPseudoTag).get();

    
    mRuleWalker->Reset();
  }

  return result;
}

already_AddRefed<nsStyleContext>
nsStyleSet::ProbePseudoStyleFor(nsIContent* aParentContent,
                                nsIAtom* aPseudoTag,
                                nsStyleContext* aParentContext)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);
  
  nsStyleContext*  result = nsnull;
  nsPresContext *presContext = PresContext();

  NS_ASSERTION(aPseudoTag, "must have pseudo tag");
  NS_ASSERTION(aParentContent &&
               aParentContent->IsNodeOfType(nsINode::eELEMENT),
               "aParentContent must be element");
  
  
  NS_ASSERTION(aParentContent ||
               nsCSSAnonBoxes::IsAnonBox(aPseudoTag),
               "null content must correspond to anonymous box");
  NS_ASSERTION(nsCSSAnonBoxes::IsAnonBox(aPseudoTag) ||
               nsCSSPseudoElements::IsPseudoElement(aPseudoTag),
               "aPseudoTag must be pseudo-element or anonymous box");

  if (aPseudoTag && presContext) {
    NS_ASSERTION(mRuleWalker->AtRoot(), "rule walker must be at root");
    PseudoRuleProcessorData data(presContext, aParentContent, aPseudoTag,
                                 nsnull, mRuleWalker);
    WalkRestrictionRule(aPseudoTag);
    
    nsRuleNode *adjustedRoot = mRuleWalker->GetCurrentNode();
    FileRules(EnumPseudoRulesMatching, &data);

    if (mRuleWalker->GetCurrentNode() != adjustedRoot)
      result = GetContext(presContext, aParentContext, aPseudoTag).get();

    
    mRuleWalker->Reset();
  }

  
  
  
  if (result &&
      (aPseudoTag == nsCSSPseudoElements::before ||
       aPseudoTag == nsCSSPseudoElements::after)) {
    const nsStyleDisplay *display = result->GetStyleDisplay();
    const nsStyleContent *content = result->GetStyleContent();
    
    if (display->mDisplay == NS_STYLE_DISPLAY_NONE ||
        content->ContentCount() == 0) {
      result->Release();
      result = nsnull;
    }
  }
  
  return result;
}

PRBool
nsStyleSet::AppendFontFaceRules(nsPresContext* aPresContext,
                                nsTArray<nsFontFaceRuleContainer>& aArray)
{
  NS_ENSURE_FALSE(mInShutdown, PR_FALSE);

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gCSSSheetTypes); ++i) {
    nsCSSRuleProcessor *ruleProc = static_cast<nsCSSRuleProcessor*>
                                    (mRuleProcessors[gCSSSheetTypes[i]].get());
    if (ruleProc && !ruleProc->AppendFontFaceRules(aPresContext, aArray))
      return PR_FALSE;
  }
  return PR_TRUE;
}

void
nsStyleSet::BeginShutdown(nsPresContext* aPresContext)
{
  mInShutdown = 1;
  mRoots.Clear(); 
}

void
nsStyleSet::Shutdown(nsPresContext* aPresContext)
{
  delete mRuleWalker;
  mRuleWalker = nsnull;

  mRuleTree->Destroy();
  mRuleTree = nsnull;

  
  
  
  for (PRUint32 i = mOldRuleTrees.Length(); i > 0; ) {
    --i;
    mOldRuleTrees[i]->Destroy();
  }
  mOldRuleTrees.Clear();

  mDefaultStyleData.Destroy(0, aPresContext);
}

static const PRInt32 kGCInterval = 1000;

void
nsStyleSet::NotifyStyleContextDestroyed(nsPresContext* aPresContext,
                                        nsStyleContext* aStyleContext)
{
  if (mInShutdown)
    return;

  NS_ASSERTION(mRuleWalker->AtRoot(), "Rule walker should be at root");

  
  
  if (!aStyleContext->GetParent()) {
    mRoots.RemoveElement(aStyleContext);
  }

  if (mInReconstruct)
    return;

  if (++mDestroyedCount == kGCInterval) {
    GCRuleTrees();
  }
}

void
nsStyleSet::GCRuleTrees()
{
  mDestroyedCount = 0;

  
  
  
  
  for (PRInt32 i = mRoots.Length() - 1; i >= 0; --i) {
    mRoots[i]->Mark();
  }

  
#ifdef DEBUG
  PRBool deleted =
#endif
    mRuleTree->Sweep();
  NS_ASSERTION(!deleted, "Root node must not be gc'd");

  
  for (PRUint32 i = mOldRuleTrees.Length(); i > 0; ) {
    --i;
    if (mOldRuleTrees[i]->Sweep()) {
      
      mOldRuleTrees.RemoveElementAt(i);
    } else {
      NS_NOTREACHED("old rule tree still referenced");
    }
  }
}

already_AddRefed<nsStyleContext>
nsStyleSet::ReParentStyleContext(nsPresContext* aPresContext,
                                 nsStyleContext* aStyleContext, 
                                 nsStyleContext* aNewParentContext)
{
  NS_ASSERTION(aPresContext, "must have pres context");
  NS_ASSERTION(aStyleContext, "must have style context");

  if (aPresContext && aStyleContext) {
    if (aStyleContext->GetParent() == aNewParentContext) {
      aStyleContext->AddRef();
      return aStyleContext;
    }
    else {  
      nsIAtom* pseudoTag = aStyleContext->GetPseudoType();

      nsRuleNode* ruleNode = aStyleContext->GetRuleNode();
      mRuleWalker->SetCurrentNode(ruleNode);

      already_AddRefed<nsStyleContext> result =
          GetContext(aPresContext, aNewParentContext, pseudoTag);
      mRuleWalker->Reset();
      return result;
    }
  }
  return nsnull;
}

struct StatefulData : public StateRuleProcessorData {
  StatefulData(nsPresContext* aPresContext,
               nsIContent* aContent, PRInt32 aStateMask)
    : StateRuleProcessorData(aPresContext, aContent, aStateMask),
      mHint(nsReStyleHint(0))
  {}
  nsReStyleHint   mHint;
}; 

static PRBool SheetHasStatefulStyle(nsIStyleRuleProcessor* aProcessor,
                                    void *aData)
{
  StatefulData* data = (StatefulData*)aData;
  nsReStyleHint hint;
  aProcessor->HasStateDependentStyle(data, &hint);
  data->mHint = nsReStyleHint(data->mHint | hint);
  return PR_TRUE; 
}


nsReStyleHint
nsStyleSet::HasStateDependentStyle(nsPresContext* aPresContext,
                                   nsIContent*     aContent,
                                   PRInt32         aStateMask)
{
  nsReStyleHint result = nsReStyleHint(0);

  if (aContent->IsNodeOfType(nsINode::eELEMENT)) {
    StatefulData data(aPresContext, aContent, aStateMask);
    WalkRuleProcessors(SheetHasStatefulStyle, &data);
    result = data.mHint;
  }

  return result;
}

struct AttributeData : public AttributeRuleProcessorData {
  AttributeData(nsPresContext* aPresContext,
                nsIContent* aContent, nsIAtom* aAttribute, PRInt32 aModType,
                PRUint32 aStateMask)
    : AttributeRuleProcessorData(aPresContext, aContent, aAttribute, aModType,
                                 aStateMask),
      mHint(nsReStyleHint(0))
  {}
  nsReStyleHint   mHint;
}; 

static PRBool
SheetHasAttributeStyle(nsIStyleRuleProcessor* aProcessor, void *aData)
{
  AttributeData* data = (AttributeData*)aData;
  nsReStyleHint hint;
  aProcessor->HasAttributeDependentStyle(data, &hint);
  data->mHint = nsReStyleHint(data->mHint | hint);
  return PR_TRUE; 
}


nsReStyleHint
nsStyleSet::HasAttributeDependentStyle(nsPresContext* aPresContext,
                                       nsIContent*    aContent,
                                       nsIAtom*       aAttribute,
                                       PRInt32        aModType,
                                       PRUint32       aStateMask)
{
  nsReStyleHint result = nsReStyleHint(0);

  if (aContent->IsNodeOfType(nsINode::eELEMENT)) {
    AttributeData data(aPresContext, aContent, aAttribute, aModType,
                       aStateMask);
    WalkRuleProcessors(SheetHasAttributeStyle, &data);
    result = data.mHint;
  }

  return result;
}

PRBool
nsStyleSet::MediumFeaturesChanged(nsPresContext* aPresContext)
{
  
  PRBool stylesChanged = PR_FALSE;
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(mRuleProcessors); ++i) {
    nsIStyleRuleProcessor *processor = mRuleProcessors[i];
    if (!processor) {
      continue;
    }
    PRBool thisChanged = PR_FALSE;
    processor->MediumFeaturesChanged(aPresContext, &thisChanged);
    stylesChanged = stylesChanged || thisChanged;
  }

  if (mBindingManager) {
    PRBool thisChanged = PR_FALSE;
    mBindingManager->MediumFeaturesChanged(aPresContext, &thisChanged);
    stylesChanged = stylesChanged || thisChanged;
  }

  return stylesChanged;
}
