










#include "mozilla/Util.h"

#include "nsStyleSet.h"
#include "nsNetUtil.h"
#include "nsCSSStyleSheet.h"
#include "nsIDocument.h"
#include "nsRuleWalker.h"
#include "nsStyleContext.h"
#include "mozilla/css/StyleRule.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSRuleProcessor.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsContentUtils.h"
#include "nsRuleData.h"
#include "nsRuleProcessorData.h"
#include "nsTransitionManager.h"
#include "nsAnimationManager.h"
#include "nsEventStates.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS1(nsEmptyStyleRule, nsIStyleRule)

 void
nsEmptyStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
}

#ifdef DEBUG
 void
nsEmptyStyleRule::List(FILE* out, PRInt32 aIndent) const
{
}
#endif

NS_IMPL_ISUPPORTS1(nsInitialStyleRule, nsIStyleRule)

 void
nsInitialStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  
  for (nsStyleStructID sid = nsStyleStructID(0);
       sid < nsStyleStructID_Length; sid = nsStyleStructID(sid + 1)) {
    if (aRuleData->mSIDs & (1 << sid)) {
      
      nsCSSValue * const value_start =
        aRuleData->mValueStorage + aRuleData->mValueOffsets[sid];
      for (nsCSSValue *value = value_start,
           *value_end = value + nsCSSProps::PropertyCountInStruct(sid);
           value != value_end; ++value) {
        
        
        if (sid == eStyleStruct_Font &&
            !aRuleData->mPresContext->Document()->GetMathMLEnabled()) {
          size_t index = value - value_start;
          if (index == nsCSSProps::PropertyIndexInStruct(
                          eCSSProperty_script_level) ||
              index == nsCSSProps::PropertyIndexInStruct(
                          eCSSProperty_script_size_multiplier) ||
              index == nsCSSProps::PropertyIndexInStruct(
                          eCSSProperty_script_min_size)) {
            continue;
          }
        }
        if (value->GetUnit() == eCSSUnit_Null) {
          value->SetInitialValue();
        }
      }
    }
  }
}

#ifdef DEBUG
 void
nsInitialStyleRule::List(FILE* out, PRInt32 aIndent) const
{
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
    mBatching(0),
    mInShutdown(false),
    mAuthorStyleDisabled(false),
    mInReconstruct(false),
    mDirty(0),
    mUnusedRuleNodeCount(0)
{
}

size_t
nsStyleSet::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);

  for (int i = 0; i < eSheetTypeCount; i++) {
    if (mRuleProcessors[i]) {
      n += mRuleProcessors[i]->SizeOfIncludingThis(aMallocSizeOf);
    }
  }

  return n;
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

  GatherRuleProcessors(eAnimationSheet);
  GatherRuleProcessors(eTransitionSheet);

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

  
  if (!mOldRuleTrees.AppendElement(mRuleTree)) {
    newTree->Destroy();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  

  mInReconstruct = true;
  mRuleTree = newTree;

  return NS_OK;
}

void
nsStyleSet::EndReconstruct()
{
  NS_ASSERTION(mInReconstruct, "Unmatched begin/end?");
  mInReconstruct = false;
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
                               aType == eStyleAttrSheet)) {
    
    return NS_OK;
  }
  if (aType == eAnimationSheet) {
    
    
    
    mRuleProcessors[aType] = PresContext()->AnimationManager();
    return NS_OK;
  }
  if (aType == eTransitionSheet) {
    
    
    
    mRuleProcessors[aType] = PresContext()->TransitionManager();
    return NS_OK;
  }
  if (mSheets[aType].Count()) {
    switch (aType) {
      case eAgentSheet:
      case eUserSheet:
      case eDocSheet:
      case eOverrideSheet: {
        
        nsCOMArray<nsIStyleSheet>& sheets = mSheets[aType];
        nsTArray<nsRefPtr<nsCSSStyleSheet> > cssSheets(sheets.Count());
        for (PRInt32 i = 0, i_end = sheets.Count(); i < i_end; ++i) {
          nsRefPtr<nsCSSStyleSheet> cssSheet = do_QueryObject(sheets[i]);
          NS_ASSERTION(cssSheet, "not a CSS sheet");
          cssSheets.AppendElement(cssSheet);
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

nsresult
nsStyleSet::AppendStyleSheet(sheetType aType, nsIStyleSheet *aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  NS_ASSERTION(aSheet->IsApplicable(),
               "Inapplicable sheet being placed in style set");
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
  NS_ASSERTION(aSheet->IsApplicable(),
               "Inapplicable sheet being placed in style set");
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
  NS_ASSERTION(aSheet->IsComplete(),
               "Incomplete sheet being removed from style set");
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

bool
nsStyleSet::GetAuthorStyleDisabled()
{
  return mAuthorStyleDisabled;
}

nsresult
nsStyleSet::SetAuthorStyleDisabled(bool aStyleDisabled)
{
  if (aStyleDisabled == !mAuthorStyleDisabled) {
    mAuthorStyleDisabled = aStyleDisabled;
    BeginUpdate();
    mDirty |= 1 << eDocSheet |
              1 << ePresHintSheet |
              1 << eStyleAttrSheet;
    return EndUpdate();
  }
  return NS_OK;
}



nsresult
nsStyleSet::AddDocStyleSheet(nsIStyleSheet* aSheet, nsIDocument* aDocument)
{
  NS_PRECONDITION(aSheet && aDocument, "null arg");
  NS_ASSERTION(aSheet->IsApplicable(),
               "Inapplicable sheet being placed in style set");

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
nsStyleSet::EnableQuirkStyleSheet(bool aEnable)
{
#ifdef DEBUG
  bool oldEnabled;
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

template<class T>
static bool
EnumRulesMatching(nsIStyleRuleProcessor* aProcessor, void* aData)
{
  T* data = static_cast<T*>(aData);
  aProcessor->RulesMatching(data);
  return true;
}

static inline bool
IsMoreSpecificThanAnimation(nsRuleNode *aRuleNode)
{
  return !aRuleNode->IsRoot() &&
         (aRuleNode->GetLevel() == nsStyleSet::eTransitionSheet ||
          (aRuleNode->IsImportantRule() &&
           (aRuleNode->GetLevel() == nsStyleSet::eAgentSheet ||
            aRuleNode->GetLevel() == nsStyleSet::eUserSheet)));
}

static nsIStyleRule*
GetAnimationRule(nsRuleNode *aRuleNode)
{
  nsRuleNode *n = aRuleNode;
  while (IsMoreSpecificThanAnimation(n)) {
    n = n->GetParent();
  }

  if (n->IsRoot() || n->GetLevel() != nsStyleSet::eAnimationSheet) {
    return nsnull;
  }

  return n->GetRule();
}

static nsRuleNode*
ReplaceAnimationRule(nsRuleNode *aOldRuleNode,
                     nsIStyleRule *aOldAnimRule,
                     nsIStyleRule *aNewAnimRule)
{
  nsTArray<nsRuleNode*> moreSpecificNodes;

  nsRuleNode *n = aOldRuleNode;
  while (IsMoreSpecificThanAnimation(n)) {
    moreSpecificNodes.AppendElement(n);
    n = n->GetParent();
  }

  if (aOldAnimRule) {
    NS_ABORT_IF_FALSE(n->GetRule() == aOldAnimRule, "wrong rule");
    NS_ABORT_IF_FALSE(n->GetLevel() == nsStyleSet::eAnimationSheet,
                      "wrong level");
    n = n->GetParent();
  }

  NS_ABORT_IF_FALSE(!IsMoreSpecificThanAnimation(n) &&
                    n->GetLevel() != nsStyleSet::eAnimationSheet,
                    "wrong level");

  if (aNewAnimRule) {
    n = n->Transition(aNewAnimRule, nsStyleSet::eAnimationSheet, false);
  }

  for (PRUint32 i = moreSpecificNodes.Length(); i-- != 0; ) {
    nsRuleNode *oldNode = moreSpecificNodes[i];
    n = n->Transition(oldNode->GetRule(), oldNode->GetLevel(),
                      oldNode->IsImportantRule());
  }

  return n;
}







already_AddRefed<nsStyleContext>
nsStyleSet::GetContext(nsStyleContext* aParentContext,
                       nsRuleNode* aRuleNode,
                       
                       
                       
                       
                       
                       nsRuleNode* aVisitedRuleNode,
                       bool aIsLink,
                       bool aIsVisitedLink,
                       nsIAtom* aPseudoTag,
                       nsCSSPseudoElements::Type aPseudoType,
                       bool aDoAnimations,
                       Element* aElementForAnimation)
{
  NS_PRECONDITION((!aPseudoTag &&
                   aPseudoType ==
                     nsCSSPseudoElements::ePseudo_NotPseudoElement) ||
                  (aPseudoTag &&
                   nsCSSPseudoElements::GetPseudoType(aPseudoTag) ==
                     aPseudoType),
                  "Pseudo mismatch");

  if (aVisitedRuleNode == aRuleNode) {
    
    aVisitedRuleNode = nsnull;
  }

  
  
  
  nsStyleContext *parentIfVisited =
    aParentContext ? aParentContext->GetStyleIfVisited() : nsnull;
  if (parentIfVisited) {
    if (!aVisitedRuleNode) {
      aVisitedRuleNode = aRuleNode;
    }
  } else {
    if (aVisitedRuleNode) {
      parentIfVisited = aParentContext;
    }
  }

  if (aIsLink) {
    
    
    
    parentIfVisited = aParentContext;
  }

  nsRefPtr<nsStyleContext> result;
  if (aParentContext)
    result = aParentContext->FindChildWithRules(aPseudoTag, aRuleNode,
                                                aVisitedRuleNode,
                                                aIsVisitedLink);

#ifdef NOISY_DEBUG
  if (result)
    fprintf(stdout, "--- SharedSC %d ---\n", ++gSharedCount);
  else
    fprintf(stdout, "+++ NewSC %d +++\n", ++gNewCount);
#endif

  if (!result) {
    result = NS_NewStyleContext(aParentContext, aPseudoTag, aPseudoType,
                                aRuleNode, PresContext());
    if (!result)
      return nsnull;
    if (aVisitedRuleNode) {
      nsRefPtr<nsStyleContext> resultIfVisited =
        NS_NewStyleContext(parentIfVisited, aPseudoTag, aPseudoType,
                           aVisitedRuleNode, PresContext());
      if (!resultIfVisited) {
        return nsnull;
      }
      if (!parentIfVisited) {
        mRoots.AppendElement(resultIfVisited);
      }
      resultIfVisited->SetIsStyleIfVisited();
      result->SetStyleIfVisited(resultIfVisited.forget());

      bool relevantLinkVisited =
        aIsLink ? aIsVisitedLink
                : (aParentContext && aParentContext->RelevantLinkVisited());
      if (relevantLinkVisited) {
        result->AddStyleBit(NS_STYLE_RELEVANT_LINK_VISITED);
      }
    }
    if (!aParentContext)
      mRoots.AppendElement(result);
  }
  else {
    NS_ASSERTION(result->GetPseudoType() == aPseudoType, "Unexpected type");
    NS_ASSERTION(result->GetPseudo() == aPseudoTag, "Unexpected pseudo");
  }

  if (aDoAnimations) {
    
    
    
    
    nsIStyleRule *oldAnimRule = GetAnimationRule(aRuleNode);
    nsIStyleRule *animRule = PresContext()->AnimationManager()->
      CheckAnimationRule(result, aElementForAnimation);
    NS_ABORT_IF_FALSE(result->GetRuleNode() == aRuleNode,
                      "unexpected rule node");
    NS_ABORT_IF_FALSE(!result->GetStyleIfVisited() == !aVisitedRuleNode,
                      "unexpected visited rule node");
    NS_ABORT_IF_FALSE(!aVisitedRuleNode ||
                      result->GetStyleIfVisited()->GetRuleNode() ==
                        aVisitedRuleNode,
                      "unexpected visited rule node");
    if (oldAnimRule != animRule) {
      nsRuleNode *ruleNode =
        ReplaceAnimationRule(aRuleNode, oldAnimRule, animRule);
      nsRuleNode *visitedRuleNode = aVisitedRuleNode
        ? ReplaceAnimationRule(aVisitedRuleNode, oldAnimRule, animRule)
        : nsnull;
      result = GetContext(aParentContext, ruleNode, visitedRuleNode,
                          aIsLink, aIsVisitedLink,
                          aPseudoTag, aPseudoType, false, nsnull);
    }
  }

  if (aElementForAnimation && aElementForAnimation->IsHTML(nsGkAtoms::body) &&
      aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement &&
      PresContext()->CompatibilityMode() == eCompatibility_NavQuirks) {
    nsIDocument* doc = aElementForAnimation->GetCurrentDoc();
    if (doc && doc->GetBodyElement() == aElementForAnimation) {
      
      PresContext()->SetBodyTextColor(result->GetStyleColor()->mColor);
    }
  }

  return result.forget();
}

void
nsStyleSet::AddImportantRules(nsRuleNode* aCurrLevelNode,
                              nsRuleNode* aLastPrevLevelNode,
                              nsRuleWalker* aRuleWalker)
{
  NS_ASSERTION(aCurrLevelNode &&
               aCurrLevelNode != aLastPrevLevelNode, "How did we get here?");

  nsAutoTArray<nsIStyleRule*, 16> importantRules;
  for (nsRuleNode *node = aCurrLevelNode; node != aLastPrevLevelNode;
       node = node->GetParent()) {
    
    
    NS_ASSERTION(nsRefPtr<css::StyleRule>(do_QueryObject(node->GetRule())),
                 "Unexpected non-CSS rule");

    nsIStyleRule* impRule =
      static_cast<css::StyleRule*>(node->GetRule())->GetImportantRule();
    if (impRule)
      importantRules.AppendElement(impRule);
  }

  NS_ASSERTION(importantRules.Length() != 0,
               "Why did we think there were important rules?");

  for (PRUint32 i = importantRules.Length(); i-- != 0; ) {
    aRuleWalker->Forward(importantRules[i]);
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
    nsRefPtr<css::StyleRule> rule(do_QueryObject(node->GetRule()));
    NS_ASSERTION(rule, "Unexpected non-CSS rule");

    NS_ASSERTION(!rule->GetImportantRule(), "Unexpected important rule");
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
    nsRefPtr<css::StyleRule> cssRule(do_QueryObject(rule));
    NS_ASSERTION(!cssRule || !cssRule->Selector(), "Unexpected CSS rule");
  }
}
#endif


void
nsStyleSet::FileRules(nsIStyleRuleProcessor::EnumFunc aCollectorFunc, 
                      void* aData, nsIContent* aContent,
                      nsRuleWalker* aRuleWalker)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  nsRuleNode* lastRestrictionRN = aRuleWalker->CurrentNode();

  aRuleWalker->SetLevel(eAgentSheet, false, true);
  if (mRuleProcessors[eAgentSheet])
    (*aCollectorFunc)(mRuleProcessors[eAgentSheet], aData);
  nsRuleNode* lastAgentRN = aRuleWalker->CurrentNode();
  bool haveImportantUARules = !aRuleWalker->GetCheckForImportantRules();

  aRuleWalker->SetLevel(eUserSheet, false, true);
  bool skipUserStyles =
    aContent && aContent->IsInNativeAnonymousSubtree();
  if (!skipUserStyles && mRuleProcessors[eUserSheet]) 
    (*aCollectorFunc)(mRuleProcessors[eUserSheet], aData);
  nsRuleNode* lastUserRN = aRuleWalker->CurrentNode();
  bool haveImportantUserRules = !aRuleWalker->GetCheckForImportantRules();

  aRuleWalker->SetLevel(ePresHintSheet, false, false);
  if (mRuleProcessors[ePresHintSheet])
    (*aCollectorFunc)(mRuleProcessors[ePresHintSheet], aData);
  nsRuleNode* lastPresHintRN = aRuleWalker->CurrentNode();
  
  aRuleWalker->SetLevel(eDocSheet, false, true);
  bool cutOffInheritance = false;
  if (mBindingManager && aContent) {
    
    mBindingManager->WalkRules(aCollectorFunc,
                               static_cast<RuleProcessorData*>(aData),
                               &cutOffInheritance);
  }
  if (!skipUserStyles && !cutOffInheritance &&
      mRuleProcessors[eDocSheet]) 
    (*aCollectorFunc)(mRuleProcessors[eDocSheet], aData);
  aRuleWalker->SetLevel(eStyleAttrSheet, false,
                        aRuleWalker->GetCheckForImportantRules());
  if (mRuleProcessors[eStyleAttrSheet])
    (*aCollectorFunc)(mRuleProcessors[eStyleAttrSheet], aData);
  nsRuleNode* lastDocRN = aRuleWalker->CurrentNode();
  bool haveImportantDocRules = !aRuleWalker->GetCheckForImportantRules();

  aRuleWalker->SetLevel(eOverrideSheet, false, true);
  if (mRuleProcessors[eOverrideSheet])
    (*aCollectorFunc)(mRuleProcessors[eOverrideSheet], aData);
  nsRuleNode* lastOvrRN = aRuleWalker->CurrentNode();
  bool haveImportantOverrideRules = !aRuleWalker->GetCheckForImportantRules();

  if (haveImportantDocRules) {
    aRuleWalker->SetLevel(eDocSheet, true, false);
    AddImportantRules(lastDocRN, lastPresHintRN, aRuleWalker);  
  }
#ifdef DEBUG
  else {
    AssertNoImportantRules(lastDocRN, lastPresHintRN);
  }
#endif

  if (haveImportantOverrideRules) {
    aRuleWalker->SetLevel(eOverrideSheet, true, false);
    AddImportantRules(lastOvrRN, lastDocRN, aRuleWalker);  
  }
#ifdef DEBUG
  else {
    AssertNoImportantRules(lastOvrRN, lastDocRN);
  }
#endif

  
  aRuleWalker->SetLevel(eAnimationSheet, false, false);
  (*aCollectorFunc)(mRuleProcessors[eAnimationSheet], aData);

#ifdef DEBUG
  AssertNoCSSRules(lastPresHintRN, lastUserRN);
#endif

  if (haveImportantUserRules) {
    aRuleWalker->SetLevel(eUserSheet, true, false);
    AddImportantRules(lastUserRN, lastAgentRN, aRuleWalker); 
  }
#ifdef DEBUG
  else {
    AssertNoImportantRules(lastUserRN, lastAgentRN);
  }
#endif

  if (haveImportantUARules) {
    aRuleWalker->SetLevel(eAgentSheet, true, false);
    AddImportantRules(lastAgentRN, lastRestrictionRN, aRuleWalker); 
  }
#ifdef DEBUG
  else {
    AssertNoImportantRules(lastAgentRN, lastRestrictionRN);
  }
#endif

#ifdef DEBUG
  AssertNoCSSRules(lastRestrictionRN, mRuleTree);
#endif

#ifdef DEBUG
  nsRuleNode *lastImportantRN = aRuleWalker->CurrentNode();
#endif
  aRuleWalker->SetLevel(eTransitionSheet, false, false);
  (*aCollectorFunc)(mRuleProcessors[eTransitionSheet], aData);
#ifdef DEBUG
  AssertNoCSSRules(aRuleWalker->CurrentNode(), lastImportantRN);
#endif

}



void
nsStyleSet::WalkRuleProcessors(nsIStyleRuleProcessor::EnumFunc aFunc,
                               RuleProcessorData* aData,
                               bool aWalkAllXBLStylesheets)
{
  if (mRuleProcessors[eAgentSheet])
    (*aFunc)(mRuleProcessors[eAgentSheet], aData);

  bool skipUserStyles = aData->mElement->IsInNativeAnonymousSubtree();
  if (!skipUserStyles && mRuleProcessors[eUserSheet]) 
    (*aFunc)(mRuleProcessors[eUserSheet], aData);

  if (mRuleProcessors[ePresHintSheet])
    (*aFunc)(mRuleProcessors[ePresHintSheet], aData);
  
  bool cutOffInheritance = false;
  if (mBindingManager) {
    
    if (aWalkAllXBLStylesheets) {
      mBindingManager->WalkAllRules(aFunc, aData);
    } else {
      mBindingManager->WalkRules(aFunc, aData, &cutOffInheritance);
    }
  }
  if (!skipUserStyles && !cutOffInheritance &&
      mRuleProcessors[eDocSheet]) 
    (*aFunc)(mRuleProcessors[eDocSheet], aData);
  if (mRuleProcessors[eStyleAttrSheet])
    (*aFunc)(mRuleProcessors[eStyleAttrSheet], aData);
  if (mRuleProcessors[eOverrideSheet])
    (*aFunc)(mRuleProcessors[eOverrideSheet], aData);
  (*aFunc)(mRuleProcessors[eAnimationSheet], aData);
  (*aFunc)(mRuleProcessors[eTransitionSheet], aData);
}

bool nsStyleSet::BuildDefaultStyleData(nsPresContext* aPresContext)
{
  NS_ASSERTION(!mDefaultStyleData.mResetData &&
               !mDefaultStyleData.mInheritedData,
               "leaking default style data");
  mDefaultStyleData.mResetData = new (aPresContext) nsResetStyleData;
  if (!mDefaultStyleData.mResetData)
    return false;
  mDefaultStyleData.mInheritedData = new (aPresContext) nsInheritedStyleData;
  if (!mDefaultStyleData.mInheritedData)
    return false;

#define SSARG_PRESCONTEXT aPresContext

#define CREATE_DATA(name, type, args) \
  if (!(mDefaultStyleData.m##type##Data->mStyleStructs[eStyleStruct_##name] = \
          new (aPresContext) nsStyle##name args)) \
    return false;

#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args) \
  CREATE_DATA(name, Inherited, ctor_args)
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args) \
  CREATE_DATA(name, Reset, ctor_args)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET
#undef SSARG_PRESCONTEXT

  return true;
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleFor(Element* aElement,
                            nsStyleContext* aParentContext)
{
  TreeMatchContext treeContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                               aElement->OwnerDoc());
  return ResolveStyleFor(aElement, aParentContext, treeContext);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleFor(Element* aElement,
                            nsStyleContext* aParentContext,
                            TreeMatchContext& aTreeMatchContext)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);
  NS_ASSERTION(aElement, "aElement must not be null");

  nsRuleWalker ruleWalker(mRuleTree);
  aTreeMatchContext.ResetForUnvisitedMatching();
  ElementRuleProcessorData data(PresContext(), aElement, &ruleWalker,
                                aTreeMatchContext);
  FileRules(EnumRulesMatching<ElementRuleProcessorData>, &data, aElement,
            &ruleWalker);

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  nsRuleNode *visitedRuleNode = nsnull;

  if (aTreeMatchContext.HaveRelevantLink()) {
    aTreeMatchContext.ResetForVisitedMatching();
    ruleWalker.Reset();
    FileRules(EnumRulesMatching<ElementRuleProcessorData>, &data, aElement,
              &ruleWalker);
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  return GetContext(aParentContext, ruleNode, visitedRuleNode,
                    nsCSSRuleProcessor::IsLink(aElement),
                    nsCSSRuleProcessor::GetContentState(aElement, aTreeMatchContext).
                      HasState(NS_EVENT_STATE_VISITED),
                    nsnull, nsCSSPseudoElements::ePseudo_NotPseudoElement,
                    true, aElement);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleForRules(nsStyleContext* aParentContext,
                                 const nsCOMArray<nsIStyleRule> &aRules)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);

  nsRuleWalker ruleWalker(mRuleTree);
  
  
  ruleWalker.SetLevel(eDocSheet, false, false);
  for (PRInt32 i = 0; i < aRules.Count(); i++) {
    ruleWalker.ForwardOnPossiblyCSSRule(aRules.ObjectAt(i));
  }

  return GetContext(aParentContext, ruleWalker.CurrentNode(), nsnull,
                    false, false,
                    nsnull, nsCSSPseudoElements::ePseudo_NotPseudoElement,
                    false, nsnull);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleByAddingRules(nsStyleContext* aBaseContext,
                                      const nsCOMArray<nsIStyleRule> &aRules)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);

  nsRuleWalker ruleWalker(mRuleTree);
  ruleWalker.SetCurrentNode(aBaseContext->GetRuleNode());
  
  
  ruleWalker.SetLevel(eDocSheet, false, false);
  for (PRInt32 i = 0; i < aRules.Count(); i++) {
    ruleWalker.ForwardOnPossiblyCSSRule(aRules.ObjectAt(i));
  }

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  nsRuleNode *visitedRuleNode = nsnull;

  if (aBaseContext->GetStyleIfVisited()) {
    ruleWalker.SetCurrentNode(aBaseContext->GetStyleIfVisited()->GetRuleNode());
    for (PRInt32 i = 0; i < aRules.Count(); i++) {
      ruleWalker.ForwardOnPossiblyCSSRule(aRules.ObjectAt(i));
    }
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  return GetContext(aBaseContext->GetParent(), ruleNode, visitedRuleNode,
                    aBaseContext->IsLinkContext(),
                    aBaseContext->RelevantLinkVisited(),
                    aBaseContext->GetPseudo(),
                    aBaseContext->GetPseudoType(),
                    false, nsnull);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleForNonElement(nsStyleContext* aParentContext)
{
  return GetContext(aParentContext, mRuleTree, nsnull,
                    false, false,
                    nsCSSAnonBoxes::mozNonElement,
                    nsCSSPseudoElements::ePseudo_AnonBox, false, nsnull);
}

void
nsStyleSet::WalkRestrictionRule(nsCSSPseudoElements::Type aPseudoType,
                                nsRuleWalker* aRuleWalker)
{
  
  aRuleWalker->SetLevel(eAgentSheet, false, false);
  if (aPseudoType == nsCSSPseudoElements::ePseudo_firstLetter)
    aRuleWalker->Forward(mFirstLetterRule);
  else if (aPseudoType == nsCSSPseudoElements::ePseudo_firstLine)
    aRuleWalker->Forward(mFirstLineRule);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolvePseudoElementStyle(Element* aParentElement,
                                      nsCSSPseudoElements::Type aType,
                                      nsStyleContext* aParentContext)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);

  NS_ASSERTION(aType < nsCSSPseudoElements::ePseudo_PseudoElementCount,
               "must have pseudo element type");
  NS_ASSERTION(aParentElement, "Must have parent element");

  nsRuleWalker ruleWalker(mRuleTree);
  TreeMatchContext treeContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                               aParentElement->OwnerDoc());
  PseudoElementRuleProcessorData data(PresContext(), aParentElement,
                                      &ruleWalker, aType, treeContext);
  WalkRestrictionRule(aType, &ruleWalker);
  FileRules(EnumRulesMatching<PseudoElementRuleProcessorData>, &data,
            aParentElement, &ruleWalker);

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  nsRuleNode *visitedRuleNode = nsnull;

  if (treeContext.HaveRelevantLink()) {
    treeContext.ResetForVisitedMatching();
    ruleWalker.Reset();
    WalkRestrictionRule(aType, &ruleWalker);
    FileRules(EnumRulesMatching<PseudoElementRuleProcessorData>, &data,
              aParentElement, &ruleWalker);
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  return GetContext(aParentContext, ruleNode, visitedRuleNode,
                    
                    
                    false, false,
                    nsCSSPseudoElements::GetPseudoAtom(aType), aType,
                    aType == nsCSSPseudoElements::ePseudo_before ||
                    aType == nsCSSPseudoElements::ePseudo_after,
                    aParentElement);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ProbePseudoElementStyle(Element* aParentElement,
                                    nsCSSPseudoElements::Type aType,
                                    nsStyleContext* aParentContext)
{
  TreeMatchContext treeContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                               aParentElement->OwnerDoc());
  return ProbePseudoElementStyle(aParentElement, aType, aParentContext,
                                 treeContext);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ProbePseudoElementStyle(Element* aParentElement,
                                    nsCSSPseudoElements::Type aType,
                                    nsStyleContext* aParentContext,
                                    TreeMatchContext& aTreeMatchContext)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);

  NS_ASSERTION(aType < nsCSSPseudoElements::ePseudo_PseudoElementCount,
               "must have pseudo element type");
  NS_ASSERTION(aParentElement, "aParentElement must not be null");

  nsIAtom* pseudoTag = nsCSSPseudoElements::GetPseudoAtom(aType);
  nsRuleWalker ruleWalker(mRuleTree);
  aTreeMatchContext.ResetForUnvisitedMatching();
  PseudoElementRuleProcessorData data(PresContext(), aParentElement,
                                      &ruleWalker, aType, aTreeMatchContext);
  WalkRestrictionRule(aType, &ruleWalker);
  
  nsRuleNode *adjustedRoot = ruleWalker.CurrentNode();
  FileRules(EnumRulesMatching<PseudoElementRuleProcessorData>, &data,
            aParentElement, &ruleWalker);

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  if (ruleNode == adjustedRoot) {
    return nsnull;
  }

  nsRuleNode *visitedRuleNode = nsnull;

  if (aTreeMatchContext.HaveRelevantLink()) {
    aTreeMatchContext.ResetForVisitedMatching();
    ruleWalker.Reset();
    WalkRestrictionRule(aType, &ruleWalker);
    FileRules(EnumRulesMatching<PseudoElementRuleProcessorData>, &data,
              aParentElement, &ruleWalker);
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  nsRefPtr<nsStyleContext> result =
    GetContext(aParentContext, ruleNode, visitedRuleNode,
               
               
               false, false,
               pseudoTag, aType,
               aType == nsCSSPseudoElements::ePseudo_before ||
               aType == nsCSSPseudoElements::ePseudo_after,
               aParentElement);

  
  
  
  if (result &&
      (pseudoTag == nsCSSPseudoElements::before ||
       pseudoTag == nsCSSPseudoElements::after)) {
    const nsStyleDisplay *display = result->GetStyleDisplay();
    const nsStyleContent *content = result->GetStyleContent();
    
    if (display->mDisplay == NS_STYLE_DISPLAY_NONE ||
        content->ContentCount() == 0) {
      result = nsnull;
    }
  }

  return result.forget();
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveAnonymousBoxStyle(nsIAtom* aPseudoTag,
                                     nsStyleContext* aParentContext)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);

#ifdef DEBUG
    bool isAnonBox = nsCSSAnonBoxes::IsAnonBox(aPseudoTag)
#ifdef MOZ_XUL
                 && !nsCSSAnonBoxes::IsTreePseudoElement(aPseudoTag)
#endif
      ;
    NS_PRECONDITION(isAnonBox, "Unexpected pseudo");
#endif

  nsRuleWalker ruleWalker(mRuleTree);
  AnonBoxRuleProcessorData data(PresContext(), aPseudoTag, &ruleWalker);
  FileRules(EnumRulesMatching<AnonBoxRuleProcessorData>, &data, nsnull,
            &ruleWalker);

  return GetContext(aParentContext, ruleWalker.CurrentNode(), nsnull,
                    false, false,
                    aPseudoTag, nsCSSPseudoElements::ePseudo_AnonBox,
                    false, nsnull);
}

#ifdef MOZ_XUL
already_AddRefed<nsStyleContext>
nsStyleSet::ResolveXULTreePseudoStyle(Element* aParentElement,
                                      nsIAtom* aPseudoTag,
                                      nsStyleContext* aParentContext,
                                      nsICSSPseudoComparator* aComparator)
{
  NS_ENSURE_FALSE(mInShutdown, nsnull);

  NS_ASSERTION(aPseudoTag, "must have pseudo tag");
  NS_ASSERTION(nsCSSAnonBoxes::IsTreePseudoElement(aPseudoTag),
               "Unexpected pseudo");

  nsRuleWalker ruleWalker(mRuleTree);
  TreeMatchContext treeContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                               aParentElement->OwnerDoc());
  XULTreeRuleProcessorData data(PresContext(), aParentElement, &ruleWalker,
                                aPseudoTag, aComparator, treeContext);
  FileRules(EnumRulesMatching<XULTreeRuleProcessorData>, &data, aParentElement,
            &ruleWalker);

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  nsRuleNode *visitedRuleNode = nsnull;

  if (treeContext.HaveRelevantLink()) {
    treeContext.ResetForVisitedMatching();
    ruleWalker.Reset();
    FileRules(EnumRulesMatching<XULTreeRuleProcessorData>, &data,
              aParentElement, &ruleWalker);
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  return GetContext(aParentContext, ruleNode, visitedRuleNode,
                    
                    
                    false, false,
                    aPseudoTag, nsCSSPseudoElements::ePseudo_XULTree,
                    false, nsnull);
}
#endif

bool
nsStyleSet::AppendFontFaceRules(nsPresContext* aPresContext,
                                nsTArray<nsFontFaceRuleContainer>& aArray)
{
  NS_ENSURE_FALSE(mInShutdown, false);

  for (PRUint32 i = 0; i < ArrayLength(gCSSSheetTypes); ++i) {
    nsCSSRuleProcessor *ruleProc = static_cast<nsCSSRuleProcessor*>
                                    (mRuleProcessors[gCSSSheetTypes[i]].get());
    if (ruleProc && !ruleProc->AppendFontFaceRules(aPresContext, aArray))
      return false;
  }
  return true;
}

bool
nsStyleSet::AppendKeyframesRules(nsPresContext* aPresContext,
                                 nsTArray<nsCSSKeyframesRule*>& aArray)
{
  NS_ENSURE_FALSE(mInShutdown, false);

  for (PRUint32 i = 0; i < ArrayLength(gCSSSheetTypes); ++i) {
    nsCSSRuleProcessor *ruleProc = static_cast<nsCSSRuleProcessor*>
                                    (mRuleProcessors[gCSSSheetTypes[i]].get());
    if (ruleProc && !ruleProc->AppendKeyframesRules(aPresContext, aArray))
      return false;
  }
  return true;
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
  mRuleTree->Destroy();
  mRuleTree = nsnull;

  
  
  
  for (PRUint32 i = mOldRuleTrees.Length(); i > 0; ) {
    --i;
    mOldRuleTrees[i]->Destroy();
  }
  mOldRuleTrees.Clear();

  mDefaultStyleData.Destroy(0, aPresContext);
}

static const PRUint32 kGCInterval = 300;

void
nsStyleSet::NotifyStyleContextDestroyed(nsPresContext* aPresContext,
                                        nsStyleContext* aStyleContext)
{
  if (mInShutdown)
    return;

  
  
  if (!aStyleContext->GetParent()) {
    mRoots.RemoveElement(aStyleContext);
  }

  if (mInReconstruct)
    return;

  if (mUnusedRuleNodeCount >= kGCInterval) {
    GCRuleTrees();
  }
}

void
nsStyleSet::GCRuleTrees()
{
  mUnusedRuleNodeCount = 0;

  
  
  
  
  for (PRInt32 i = mRoots.Length() - 1; i >= 0; --i) {
    mRoots[i]->Mark();
  }

  
#ifdef DEBUG
  bool deleted =
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

static inline nsRuleNode*
SkipAnimationRules(nsRuleNode* aRuleNode, Element* aElement, bool isPseudo)
{
  nsRuleNode* ruleNode = aRuleNode;
  while (!ruleNode->IsRoot() &&
         (ruleNode->GetLevel() == nsStyleSet::eTransitionSheet ||
          ruleNode->GetLevel() == nsStyleSet::eAnimationSheet)) {
    ruleNode = ruleNode->GetParent();
  }
  if (ruleNode != aRuleNode) {
    NS_ASSERTION(aElement, "How can we have transition rules but no element?");
    
    
    
    nsRestyleHint hint = isPseudo ? eRestyle_Subtree : eRestyle_Self;
    aRuleNode->GetPresContext()->PresShell()->RestyleForAnimation(aElement,
                                                                  hint);
  }
  return ruleNode;
}

already_AddRefed<nsStyleContext>
nsStyleSet::ReparentStyleContext(nsStyleContext* aStyleContext,
                                 nsStyleContext* aNewParentContext,
                                 Element* aElement)
{
  if (!aStyleContext) {
    NS_NOTREACHED("must have style context");
    return nsnull;
  }

  
  
  if (aStyleContext->GetParent() == aNewParentContext) {
    aStyleContext->AddRef();
    return aStyleContext;
  }

  nsIAtom* pseudoTag = aStyleContext->GetPseudo();
  nsCSSPseudoElements::Type pseudoType = aStyleContext->GetPseudoType();
  nsRuleNode* ruleNode = aStyleContext->GetRuleNode();

  
  
  bool skipAnimationRules = PresContext()->IsProcessingRestyles() &&
    !PresContext()->IsProcessingAnimationStyleChange();
  if (skipAnimationRules) {
    
    
    
    ruleNode =
      SkipAnimationRules(ruleNode, aElement,
                         pseudoType !=
                           nsCSSPseudoElements::ePseudo_NotPseudoElement);
  }

  nsRuleNode* visitedRuleNode = nsnull;
  nsStyleContext* visitedContext = aStyleContext->GetStyleIfVisited();
  
  
  
  
  if (visitedContext) {
     visitedRuleNode = visitedContext->GetRuleNode();
     
     if (skipAnimationRules) {
      
       visitedRuleNode =
         SkipAnimationRules(visitedRuleNode, aElement,
                            pseudoType !=
                              nsCSSPseudoElements::ePseudo_NotPseudoElement);
     }
  }

  
  
  
  bool relevantLinkVisited = aStyleContext->IsLinkContext() ?
    aStyleContext->RelevantLinkVisited() :
    aNewParentContext->RelevantLinkVisited();

  return GetContext(aNewParentContext, ruleNode, visitedRuleNode,
                    aStyleContext->IsLinkContext(),
                    relevantLinkVisited,
                    pseudoTag, pseudoType,
                    pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
                    pseudoType == nsCSSPseudoElements::ePseudo_before ||
                    pseudoType == nsCSSPseudoElements::ePseudo_after,
                    aElement);
}

struct StatefulData : public StateRuleProcessorData {
  StatefulData(nsPresContext* aPresContext, Element* aElement,
               nsEventStates aStateMask, TreeMatchContext& aTreeMatchContext)
    : StateRuleProcessorData(aPresContext, aElement, aStateMask,
                             aTreeMatchContext),
      mHint(nsRestyleHint(0))
  {}
  nsRestyleHint   mHint;
};

static bool SheetHasDocumentStateStyle(nsIStyleRuleProcessor* aProcessor,
                                         void *aData)
{
  StatefulData* data = (StatefulData*)aData;
  if (aProcessor->HasDocumentStateDependentStyle(data)) {
    data->mHint = eRestyle_Self;
    return false; 
  }
  return true; 
}


bool
nsStyleSet::HasDocumentStateDependentStyle(nsPresContext* aPresContext,
                                           nsIContent*    aContent,
                                           nsEventStates  aStateMask)
{
  if (!aContent || !aContent->IsElement())
    return false;

  TreeMatchContext treeContext(false, nsRuleWalker::eLinksVisitedOrUnvisited,
                               aContent->OwnerDoc());
  StatefulData data(aPresContext, aContent->AsElement(), aStateMask,
                    treeContext);
  WalkRuleProcessors(SheetHasDocumentStateStyle, &data, true);
  return data.mHint != 0;
}

static bool SheetHasStatefulStyle(nsIStyleRuleProcessor* aProcessor,
                                    void *aData)
{
  StatefulData* data = (StatefulData*)aData;
  nsRestyleHint hint = aProcessor->HasStateDependentStyle(data);
  data->mHint = nsRestyleHint(data->mHint | hint);
  return true; 
}


nsRestyleHint
nsStyleSet::HasStateDependentStyle(nsPresContext*       aPresContext,
                                   Element*             aElement,
                                   nsEventStates        aStateMask)
{
  TreeMatchContext treeContext(false, nsRuleWalker::eLinksVisitedOrUnvisited,
                               aElement->OwnerDoc());
  StatefulData data(aPresContext, aElement, aStateMask, treeContext);
  WalkRuleProcessors(SheetHasStatefulStyle, &data, false);
  return data.mHint;
}

struct AttributeData : public AttributeRuleProcessorData {
  AttributeData(nsPresContext* aPresContext,
                Element* aElement, nsIAtom* aAttribute, PRInt32 aModType,
                bool aAttrHasChanged, TreeMatchContext& aTreeMatchContext)
    : AttributeRuleProcessorData(aPresContext, aElement, aAttribute, aModType,
                                 aAttrHasChanged, aTreeMatchContext),
      mHint(nsRestyleHint(0))
  {}
  nsRestyleHint   mHint;
}; 

static bool
SheetHasAttributeStyle(nsIStyleRuleProcessor* aProcessor, void *aData)
{
  AttributeData* data = (AttributeData*)aData;
  nsRestyleHint hint = aProcessor->HasAttributeDependentStyle(data);
  data->mHint = nsRestyleHint(data->mHint | hint);
  return true; 
}


nsRestyleHint
nsStyleSet::HasAttributeDependentStyle(nsPresContext* aPresContext,
                                       Element*       aElement,
                                       nsIAtom*       aAttribute,
                                       PRInt32        aModType,
                                       bool           aAttrHasChanged)
{
  TreeMatchContext treeContext(false, nsRuleWalker::eLinksVisitedOrUnvisited,
                               aElement->OwnerDoc());
  AttributeData data(aPresContext, aElement, aAttribute,
                     aModType, aAttrHasChanged, treeContext);
  WalkRuleProcessors(SheetHasAttributeStyle, &data, false);
  return data.mHint;
}

bool
nsStyleSet::MediumFeaturesChanged(nsPresContext* aPresContext)
{
  
  bool stylesChanged = false;
  for (PRUint32 i = 0; i < ArrayLength(mRuleProcessors); ++i) {
    nsIStyleRuleProcessor *processor = mRuleProcessors[i];
    if (!processor) {
      continue;
    }
    bool thisChanged = processor->MediumFeaturesChanged(aPresContext);
    stylesChanged = stylesChanged || thisChanged;
  }

  if (mBindingManager) {
    bool thisChanged = false;
    mBindingManager->MediumFeaturesChanged(aPresContext, &thisChanged);
    stylesChanged = stylesChanged || thisChanged;
  }

  return stylesChanged;
}

nsCSSStyleSheet::EnsureUniqueInnerResult
nsStyleSet::EnsureUniqueInnerOnCSSSheets()
{
  nsAutoTArray<nsCSSStyleSheet*, 32> queue;
  for (PRUint32 i = 0; i < ArrayLength(gCSSSheetTypes); ++i) {
    nsCOMArray<nsIStyleSheet> &sheets = mSheets[gCSSSheetTypes[i]];
    for (PRUint32 j = 0, j_end = sheets.Count(); j < j_end; ++j) {
      nsCSSStyleSheet *sheet = static_cast<nsCSSStyleSheet*>(sheets[j]);
      if (!queue.AppendElement(sheet)) {
        return nsCSSStyleSheet::eUniqueInner_CloneFailed;
      }
    }
  }

  if (mBindingManager) {
    mBindingManager->AppendAllSheets(queue);
  }

  nsCSSStyleSheet::EnsureUniqueInnerResult res =
    nsCSSStyleSheet::eUniqueInner_AlreadyUnique;
  while (!queue.IsEmpty()) {
    PRUint32 idx = queue.Length() - 1;
    nsCSSStyleSheet *sheet = queue[idx];
    queue.RemoveElementAt(idx);

    nsCSSStyleSheet::EnsureUniqueInnerResult sheetRes =
      sheet->EnsureUniqueInner();
    if (sheetRes == nsCSSStyleSheet::eUniqueInner_CloneFailed) {
      return sheetRes;
    }
    if (sheetRes == nsCSSStyleSheet::eUniqueInner_ClonedInner) {
      res = sheetRes;
    }

    
    if (!sheet->AppendAllChildSheets(queue)) {
      return nsCSSStyleSheet::eUniqueInner_CloneFailed;
    }
  }
  return res;
}

nsIStyleRule*
nsStyleSet::InitialStyleRule()
{
  if (!mInitialStyleRule) {
    mInitialStyleRule = new nsInitialStyleRule;
  }
  return mInitialStyleRule;
}
