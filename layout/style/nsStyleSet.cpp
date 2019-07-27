










#include "nsStyleSet.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/CSSStyleSheet.h"
#include "mozilla/EventStates.h"
#include "mozilla/MemoryReporting.h"
#include "nsIDocumentInlines.h"
#include "nsRuleWalker.h"
#include "nsStyleContext.h"
#include "mozilla/css/StyleRule.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSRuleProcessor.h"
#include "nsDataHashtable.h"
#include "nsIContent.h"
#include "nsRuleData.h"
#include "nsRuleProcessorData.h"
#include "nsTransitionManager.h"
#include "nsAnimationManager.h"
#include "nsStyleSheetService.h"
#include "mozilla/dom/Element.h"
#include "GeckoProfiler.h"
#include "nsHTMLCSSStyleSheet.h"
#include "nsHTMLStyleSheet.h"
#include "SVGAttrAnimationRuleProcessor.h"
#include "nsCSSRules.h"
#include "nsPrintfCString.h"
#include "nsIFrame.h"
#include "RestyleManager.h"
#include "nsQueryObject.h"

#include <inttypes.h>

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(nsEmptyStyleRule, nsIStyleRule)

 void
nsEmptyStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
}

#ifdef DEBUG
 void
nsEmptyStyleRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString indentStr;
  for (int32_t index = aIndent; --index >= 0; ) {
    indentStr.AppendLiteral("  ");
  }
  fprintf_stderr(out, "%s[empty style rule] {}\n", indentStr.get());
}
#endif

NS_IMPL_ISUPPORTS(nsInitialStyleRule, nsIStyleRule)

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
                          eCSSProperty_script_min_size) ||
              index == nsCSSProps::PropertyIndexInStruct(
                          eCSSProperty_math_variant) ||
              index == nsCSSProps::PropertyIndexInStruct(
                          eCSSProperty_math_display)) {
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
nsInitialStyleRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString indentStr;
  for (int32_t index = aIndent; --index >= 0; ) {
    indentStr.AppendLiteral("  ");
  }
  fprintf_stderr(out, "%s[initial style rule] {}\n", indentStr.get());
}
#endif

NS_IMPL_ISUPPORTS(nsDisableTextZoomStyleRule, nsIStyleRule)

 void
nsDisableTextZoomStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (!(aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Font)))
    return;

  nsCSSValue* value = aRuleData->ValueForTextZoom();
  if (value->GetUnit() == eCSSUnit_Null)
    value->SetNoneValue();
}

#ifdef DEBUG
 void
nsDisableTextZoomStyleRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString indentStr;
  for (int32_t index = aIndent; --index >= 0; ) {
    indentStr.AppendLiteral("  ");
  }
  fprintf_stderr(out, "%s[disable text zoom style rule] {}\n", indentStr.get());
}
#endif

static const nsStyleSet::sheetType gCSSSheetTypes[] = {
  
  nsStyleSet::eAgentSheet,
  nsStyleSet::eUserSheet,
  nsStyleSet::eDocSheet,
  nsStyleSet::eScopedDocSheet,
  nsStyleSet::eOverrideSheet
};

static bool IsCSSSheetType(nsStyleSet::sheetType aSheetType)
{
  for (nsStyleSet::sheetType type : gCSSSheetTypes) {
    if (type == aSheetType) {
      return true;
    }
  }
  return false;
}

nsStyleSet::nsStyleSet()
  : mRuleTree(nullptr),
    mBatching(0),
    mInShutdown(false),
    mAuthorStyleDisabled(false),
    mInReconstruct(false),
    mInitFontFeatureValuesLookup(true),
    mNeedsRestyleAfterEnsureUniqueInner(false),
    mDirty(0),
    mUnusedRuleNodeCount(0)
{
}

nsStyleSet::~nsStyleSet()
{
  for (sheetType type : gCSSSheetTypes) {
    for (uint32_t i = 0, n = mSheets[type].Length(); i < n; i++) {
      static_cast<CSSStyleSheet*>(mSheets[type][i])->DropStyleSet(this);
    }
  }
}

size_t
nsStyleSet::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);

  for (int i = 0; i < eSheetTypeCount; i++) {
    if (mRuleProcessors[i]) {
      n += mRuleProcessors[i]->SizeOfIncludingThis(aMallocSizeOf);
    }
    
    
    
    
    
    n += mSheets[i].SizeOfExcludingThis(nullptr, aMallocSizeOf);
  }

  for (uint32_t i = 0; i < mScopedDocSheetRuleProcessors.Length(); i++) {
    n += mScopedDocSheetRuleProcessors[i]->SizeOfIncludingThis(aMallocSizeOf);
  }
  n += mScopedDocSheetRuleProcessors.SizeOfExcludingThis(aMallocSizeOf);

  n += mRoots.SizeOfExcludingThis(aMallocSizeOf);
  n += mOldRuleTrees.SizeOfExcludingThis(aMallocSizeOf);

  return n;
}

void
nsStyleSet::Init(nsPresContext *aPresContext)
{
  mFirstLineRule = new nsEmptyStyleRule;
  mFirstLetterRule = new nsEmptyStyleRule;
  mPlaceholderRule = new nsEmptyStyleRule;
  mDisableTextZoomStyleRule = new nsDisableTextZoomStyleRule;

  mRuleTree = nsRuleNode::CreateRootNode(aPresContext);

  
  
  
  
  
  GatherRuleProcessors(eAnimationSheet);
  GatherRuleProcessors(eTransitionSheet);
  GatherRuleProcessors(eSVGAttrAnimationSheet);
}

nsresult
nsStyleSet::BeginReconstruct()
{
  NS_ASSERTION(!mInReconstruct, "Unmatched begin/end?");
  NS_ASSERTION(mRuleTree, "Reconstructing before first construction?");

  
  nsRuleNode* newTree = nsRuleNode::CreateRootNode(mRuleTree->PresContext());

  
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
  for (int32_t i = mRoots.Length() - 1; i >= 0; --i) {
    
    
    
    
    
    

    NS_ASSERTION(mRoots[i]->RuleNode()->RuleTree() == mRuleTree,
                 "style context has old rule node");
  }
#endif
  
  
  
  GCRuleTrees();
}

typedef nsDataHashtable<nsPtrHashKey<nsINode>, uint32_t> ScopeDepthCache;




static uint32_t
GetScopeDepth(nsINode* aScopeElement, ScopeDepthCache& aCache)
{
  nsINode* parent = aScopeElement->GetParent();
  if (!parent || !parent->IsElementInStyleScope()) {
    return 1;
  }

  uint32_t depth = aCache.Get(aScopeElement);
  if (!depth) {
    for (nsINode* n = parent; n; n = n->GetParent()) {
      if (n->IsScopedStyleRoot()) {
        depth = GetScopeDepth(n, aCache) + 1;
        aCache.Put(aScopeElement, depth);
        break;
      }
    }
  }
  return depth;
}

struct ScopedSheetOrder
{
  CSSStyleSheet* mSheet;
  uint32_t mDepth;
  uint32_t mOrder;

  bool operator==(const ScopedSheetOrder& aRHS) const
  {
    return mDepth == aRHS.mDepth &&
           mOrder == aRHS.mOrder;
  }

  bool operator<(const ScopedSheetOrder& aRHS) const
  {
    if (mDepth != aRHS.mDepth) {
      return mDepth < aRHS.mDepth;
    }
    return mOrder < aRHS.mOrder;
  }
};




static void
SortStyleSheetsByScope(nsTArray<CSSStyleSheet*>& aSheets)
{
  uint32_t n = aSheets.Length();
  if (n == 1) {
    return;
  }

  ScopeDepthCache cache;

  nsTArray<ScopedSheetOrder> sheets;
  sheets.SetLength(n);

  
  
  for (uint32_t i = 0; i < n; i++) {
    sheets[i].mSheet = aSheets[i];
    sheets[i].mDepth = GetScopeDepth(aSheets[i]->GetScopeElement(), cache);
    sheets[i].mOrder = i;
  }

  
  sheets.Sort();

  for (uint32_t i = 0; i < n; i++) {
    aSheets[i] = sheets[i].mSheet;
  }
}

nsresult
nsStyleSet::GatherRuleProcessors(sheetType aType)
{
  nsCOMPtr<nsIStyleRuleProcessor> oldRuleProcessor(mRuleProcessors[aType]);
  nsTArray<nsCOMPtr<nsIStyleRuleProcessor>> oldScopedDocRuleProcessors;

  mRuleProcessors[aType] = nullptr;
  if (aType == eScopedDocSheet) {
    for (uint32_t i = 0; i < mScopedDocSheetRuleProcessors.Length(); i++) {
      nsIStyleRuleProcessor* processor = mScopedDocSheetRuleProcessors[i].get();
      Element* scope =
        static_cast<nsCSSRuleProcessor*>(processor)->GetScopeElement();
      scope->ClearIsScopedStyleRoot();
    }

    
    oldScopedDocRuleProcessors.SwapElements(mScopedDocSheetRuleProcessors);
  }
  if (mAuthorStyleDisabled && (aType == eDocSheet || 
                               aType == eScopedDocSheet ||
                               aType == eStyleAttrSheet)) {
    
    
    
    return NS_OK;
  }
  switch (aType) {
    
    
    case eAnimationSheet:
      MOZ_ASSERT(mSheets[aType].IsEmpty());
      mRuleProcessors[aType] = PresContext()->AnimationManager();
      return NS_OK;
    case eTransitionSheet:
      MOZ_ASSERT(mSheets[aType].IsEmpty());
      mRuleProcessors[aType] = PresContext()->TransitionManager();
      return NS_OK;
    case eStyleAttrSheet:
      MOZ_ASSERT(mSheets[aType].IsEmpty());
      mRuleProcessors[aType] = PresContext()->Document()->GetInlineStyleSheet();
      return NS_OK;
    case ePresHintSheet:
      MOZ_ASSERT(mSheets[aType].IsEmpty());
      mRuleProcessors[aType] =
        PresContext()->Document()->GetAttributeStyleSheet();
      return NS_OK;
    case eSVGAttrAnimationSheet:
      MOZ_ASSERT(mSheets[aType].IsEmpty());
      mRuleProcessors[aType] =
        PresContext()->Document()->GetSVGAttrAnimationRuleProcessor();
      return NS_OK;
    default:
      
      break;
  }
  if (aType == eScopedDocSheet) {
    
    uint32_t count = mSheets[eScopedDocSheet].Count();
    if (count) {
      
      
      
      nsTArray<CSSStyleSheet*> sheets(count);
      for (uint32_t i = 0; i < count; i++) {
        nsRefPtr<CSSStyleSheet> sheet =
          do_QueryObject(mSheets[eScopedDocSheet].ObjectAt(i));
        sheets.AppendElement(sheet);

        Element* scope = sheet->GetScopeElement();
        scope->SetIsScopedStyleRoot();
      }

      
      
      SortStyleSheetsByScope(sheets);

      
      
      
      
      nsDataHashtable<nsPtrHashKey<Element>,
                      nsCSSRuleProcessor*> oldScopedRuleProcessorHash;
      for (size_t i = oldScopedDocRuleProcessors.Length(); i-- != 0; ) {
        nsCSSRuleProcessor* oldRP =
          static_cast<nsCSSRuleProcessor*>(oldScopedDocRuleProcessors[i].get());
        Element* scope = oldRP->GetScopeElement();
        MOZ_ASSERT(!oldScopedRuleProcessorHash.Get(scope),
                   "duplicate rule processors for same scope element?");
        oldScopedRuleProcessorHash.Put(scope, oldRP);
      }

      uint32_t start = 0, end;
      do {
        
        Element* scope = sheets[start]->GetScopeElement();
        end = start + 1;
        while (end < count && sheets[end]->GetScopeElement() == scope) {
          end++;
        }

        scope->SetIsScopedStyleRoot();

        
        nsTArray<nsRefPtr<CSSStyleSheet>> sheetsForScope;
        sheetsForScope.AppendElements(sheets.Elements() + start, end - start);
        nsCSSRuleProcessor* oldRP = oldScopedRuleProcessorHash.Get(scope);
        mScopedDocSheetRuleProcessors.AppendElement
          (new nsCSSRuleProcessor(sheetsForScope, uint8_t(aType), scope,
                                  oldRP));

        start = end;
      } while (start < count);
    }
    return NS_OK;
  }
  if (mSheets[aType].Count()) {
    switch (aType) {
      case eAgentSheet:
      case eUserSheet:
      case eDocSheet:
      case eOverrideSheet: {
        
        nsCOMArray<nsIStyleSheet>& sheets = mSheets[aType];
        nsTArray<nsRefPtr<CSSStyleSheet>> cssSheets(sheets.Count());
        for (int32_t i = 0, i_end = sheets.Count(); i < i_end; ++i) {
          nsRefPtr<CSSStyleSheet> cssSheet = do_QueryObject(sheets[i]);
          NS_ASSERTION(cssSheet, "not a CSS sheet");
          cssSheets.AppendElement(cssSheet);
        }
        mRuleProcessors[aType] =
          new nsCSSRuleProcessor(cssSheets, uint8_t(aType), nullptr,
                                 static_cast<nsCSSRuleProcessor*>(
                                   oldRuleProcessor.get()));
      } break;

      default:
        
        NS_ASSERTION(mSheets[aType].Count() == 1, "only one sheet per level");
        mRuleProcessors[aType] = do_QueryInterface(mSheets[aType][0]);
        break;
    }
  }

  return NS_OK;
}

static bool
IsScopedStyleSheet(nsIStyleSheet* aSheet)
{
  nsRefPtr<CSSStyleSheet> cssSheet = do_QueryObject(aSheet);
  NS_ASSERTION(cssSheet, "expected aSheet to be a CSSStyleSheet");

  return cssSheet->GetScopeElement();
}

nsresult
nsStyleSet::AppendStyleSheet(sheetType aType, nsIStyleSheet *aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  NS_ASSERTION(aSheet->IsApplicable(),
               "Inapplicable sheet being placed in style set");
  bool present = mSheets[aType].RemoveObject(aSheet);
  if (!mSheets[aType].AppendObject(aSheet))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!present && IsCSSSheetType(aType)) {
    static_cast<CSSStyleSheet*>(aSheet)->AddStyleSet(this);
  }

  return DirtyRuleProcessors(aType);
}

nsresult
nsStyleSet::PrependStyleSheet(sheetType aType, nsIStyleSheet *aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  NS_ASSERTION(aSheet->IsApplicable(),
               "Inapplicable sheet being placed in style set");
  bool present = mSheets[aType].RemoveObject(aSheet);
  if (!mSheets[aType].InsertObjectAt(aSheet, 0))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!present && IsCSSSheetType(aType)) {
    static_cast<CSSStyleSheet*>(aSheet)->AddStyleSet(this);
  }

  return DirtyRuleProcessors(aType);
}

nsresult
nsStyleSet::RemoveStyleSheet(sheetType aType, nsIStyleSheet *aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  NS_ASSERTION(aSheet->IsComplete(),
               "Incomplete sheet being removed from style set");
  if (mSheets[aType].RemoveObject(aSheet)) {
    if (IsCSSSheetType(aType)) {
      static_cast<CSSStyleSheet*>(aSheet)->DropStyleSet(this);
    }
  }

  return DirtyRuleProcessors(aType);
}

nsresult
nsStyleSet::ReplaceSheets(sheetType aType,
                          const nsCOMArray<nsIStyleSheet> &aNewSheets)
{
  bool cssSheetType = IsCSSSheetType(aType);
  if (cssSheetType) {
    for (uint32_t i = 0, n = mSheets[aType].Length(); i < n; i++) {
      static_cast<CSSStyleSheet*>(mSheets[aType][i])->DropStyleSet(this);
    }
  }

  mSheets[aType].Clear();
  if (!mSheets[aType].AppendObjects(aNewSheets))
    return NS_ERROR_OUT_OF_MEMORY;

  if (cssSheetType) {
    for (uint32_t i = 0, n = mSheets[aType].Length(); i < n; i++) {
      static_cast<CSSStyleSheet*>(mSheets[aType][i])->AddStyleSet(this);
    }
  }

  return DirtyRuleProcessors(aType);
}

nsresult
nsStyleSet::InsertStyleSheetBefore(sheetType aType, nsIStyleSheet *aNewSheet,
                                   nsIStyleSheet *aReferenceSheet)
{
  NS_PRECONDITION(aNewSheet && aReferenceSheet, "null arg");
  NS_ASSERTION(aNewSheet->IsApplicable(),
               "Inapplicable sheet being placed in style set");

  bool present = mSheets[aType].RemoveObject(aNewSheet);
  int32_t idx = mSheets[aType].IndexOf(aReferenceSheet);
  if (idx < 0)
    return NS_ERROR_INVALID_ARG;

  if (!mSheets[aType].InsertObjectAt(aNewSheet, idx))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!present && IsCSSSheetType(aType)) {
    static_cast<CSSStyleSheet*>(aNewSheet)->AddStyleSet(this);
  }

  return DirtyRuleProcessors(aType);
}

nsresult
nsStyleSet::DirtyRuleProcessors(sheetType aType)
{
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
              1 << eScopedDocSheet |
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

  sheetType type = IsScopedStyleSheet(aSheet) ?
                     eScopedDocSheet :
                     eDocSheet;
  nsCOMArray<nsIStyleSheet>& sheets = mSheets[type];

  bool present = sheets.RemoveObject(aSheet);
  nsStyleSheetService *sheetService = nsStyleSheetService::GetInstance();

  
  int32_t newDocIndex = aDocument->GetIndexOfStyleSheet(aSheet);

  int32_t count = sheets.Count();
  int32_t index;
  for (index = 0; index < count; index++) {
    nsIStyleSheet* sheet = sheets.ObjectAt(index);
    int32_t sheetDocIndex = aDocument->GetIndexOfStyleSheet(sheet);
    if (sheetDocIndex > newDocIndex)
      break;

    
    
    
    
    if (sheetDocIndex < 0 &&
        ((sheetService &&
        sheetService->AuthorStyleSheets()->IndexOf(sheet) >= 0) ||
        sheet == aDocument->FirstAdditionalAuthorSheet()))
        break;
  }
  if (!sheets.InsertObjectAt(aSheet, index))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!present) {
    static_cast<CSSStyleSheet*>(aSheet)->AddStyleSet(this);
  }

  return DirtyRuleProcessors(type);
}

nsresult
nsStyleSet::RemoveDocStyleSheet(nsIStyleSheet *aSheet)
{
  nsRefPtr<CSSStyleSheet> cssSheet = do_QueryObject(aSheet);
  bool isScoped = cssSheet && cssSheet->GetScopeElement();
  return RemoveStyleSheet(isScoped ? eScopedDocSheet : eDocSheet, aSheet);
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
          aRuleNode->IsImportantRule());
}

static nsIStyleRule*
GetAnimationRule(nsRuleNode *aRuleNode)
{
  nsRuleNode *n = aRuleNode;
  while (IsMoreSpecificThanAnimation(n)) {
    n = n->GetParent();
  }

  if (n->IsRoot() || n->GetLevel() != nsStyleSet::eAnimationSheet) {
    return nullptr;
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
    MOZ_ASSERT(n->GetRule() == aOldAnimRule, "wrong rule");
    MOZ_ASSERT(n->GetLevel() == nsStyleSet::eAnimationSheet,
               "wrong level");
    n = n->GetParent();
  }

  MOZ_ASSERT(!IsMoreSpecificThanAnimation(n) &&
             (n->IsRoot() || n->GetLevel() != nsStyleSet::eAnimationSheet),
             "wrong level");

  if (aNewAnimRule) {
    n = n->Transition(aNewAnimRule, nsStyleSet::eAnimationSheet, false);
  }

  for (uint32_t i = moreSpecificNodes.Length(); i-- != 0; ) {
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
                       nsIAtom* aPseudoTag,
                       nsCSSPseudoElements::Type aPseudoType,
                       Element* aElementForAnimation,
                       uint32_t aFlags)
{
  NS_PRECONDITION((!aPseudoTag &&
                   aPseudoType ==
                     nsCSSPseudoElements::ePseudo_NotPseudoElement) ||
                  (aPseudoTag &&
                   nsCSSPseudoElements::GetPseudoType(aPseudoTag) ==
                     aPseudoType),
                  "Pseudo mismatch");

  if (aVisitedRuleNode == aRuleNode) {
    
    aVisitedRuleNode = nullptr;
  }

  
  
  
  nsStyleContext *parentIfVisited =
    aParentContext ? aParentContext->GetStyleIfVisited() : nullptr;
  if (parentIfVisited) {
    if (!aVisitedRuleNode) {
      aVisitedRuleNode = aRuleNode;
    }
  } else {
    if (aVisitedRuleNode) {
      parentIfVisited = aParentContext;
    }
  }

  if (aFlags & eIsLink) {
    
    
    
    parentIfVisited = aParentContext;
  }

  bool relevantLinkVisited = (aFlags & eIsLink) ?
    (aFlags & eIsVisitedLink) :
    (aParentContext && aParentContext->RelevantLinkVisited());

  nsRefPtr<nsStyleContext> result;
  if (aParentContext)
    result = aParentContext->FindChildWithRules(aPseudoTag, aRuleNode,
                                                aVisitedRuleNode,
                                                relevantLinkVisited);

#ifdef NOISY_DEBUG
  if (result)
    fprintf(stdout, "--- SharedSC %d ---\n", ++gSharedCount);
  else
    fprintf(stdout, "+++ NewSC %d +++\n", ++gNewCount);
#endif

  if (!result) {
    result = NS_NewStyleContext(aParentContext, aPseudoTag, aPseudoType,
                                aRuleNode,
                                aFlags & eSkipParentDisplayBasedStyleFixup);
    if (aVisitedRuleNode) {
      nsRefPtr<nsStyleContext> resultIfVisited =
        NS_NewStyleContext(parentIfVisited, aPseudoTag, aPseudoType,
                           aVisitedRuleNode,
                           aFlags & eSkipParentDisplayBasedStyleFixup);
      if (!parentIfVisited) {
        mRoots.AppendElement(resultIfVisited);
      }
      resultIfVisited->SetIsStyleIfVisited();
      result->SetStyleIfVisited(resultIfVisited.forget());

      if (relevantLinkVisited) {
        result->AddStyleBit(NS_STYLE_RELEVANT_LINK_VISITED);
      }
    }
    if (!aParentContext) {
      mRoots.AppendElement(result);
    }
  }
  else {
    NS_ASSERTION(result->GetPseudoType() == aPseudoType, "Unexpected type");
    NS_ASSERTION(result->GetPseudo() == aPseudoTag, "Unexpected pseudo");
  }

  if (aFlags & eDoAnimation) {
    
    
    
    
    nsIStyleRule *oldAnimRule = GetAnimationRule(aRuleNode);
    nsIStyleRule *animRule = PresContext()->AnimationManager()->
      CheckAnimationRule(result, aElementForAnimation);
    MOZ_ASSERT(result->RuleNode() == aRuleNode,
               "unexpected rule node");
    MOZ_ASSERT(!result->GetStyleIfVisited() == !aVisitedRuleNode,
               "unexpected visited rule node");
    MOZ_ASSERT(!aVisitedRuleNode ||
               result->GetStyleIfVisited()->RuleNode() == aVisitedRuleNode,
               "unexpected visited rule node");
    MOZ_ASSERT(!aVisitedRuleNode ||
               oldAnimRule == GetAnimationRule(aVisitedRuleNode),
               "animation rule mismatch between rule nodes");
    if (oldAnimRule != animRule) {
      nsRuleNode *ruleNode =
        ReplaceAnimationRule(aRuleNode, oldAnimRule, animRule);
      nsRuleNode *visitedRuleNode = aVisitedRuleNode
        ? ReplaceAnimationRule(aVisitedRuleNode, oldAnimRule, animRule)
        : nullptr;
      MOZ_ASSERT(!visitedRuleNode ||
                 GetAnimationRule(ruleNode) ==
                   GetAnimationRule(visitedRuleNode),
                 "animation rule mismatch between rule nodes");
      result = GetContext(aParentContext, ruleNode, visitedRuleNode,
                          aPseudoTag, aPseudoType, nullptr,
                          aFlags & ~eDoAnimation);
    }
  }

  if (aElementForAnimation &&
      aElementForAnimation->IsHTMLElement(nsGkAtoms::body) &&
      aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement &&
      PresContext()->CompatibilityMode() == eCompatibility_NavQuirks) {
    nsIDocument* doc = aElementForAnimation->GetCurrentDoc();
    if (doc && doc->GetBodyElement() == aElementForAnimation) {
      
      PresContext()->SetBodyTextColor(result->StyleColor()->mColor);
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

  for (uint32_t i = importantRules.Length(); i-- != 0; ) {
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
                      RuleProcessorData* aData, Element* aElement,
                      nsRuleWalker* aRuleWalker)
{
  PROFILER_LABEL("nsStyleSet", "FileRules",
    js::ProfileEntry::Category::CSS);

  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  nsRuleNode* lastRestrictionRN = aRuleWalker->CurrentNode();

  aRuleWalker->SetLevel(eAgentSheet, false, true);
  if (mRuleProcessors[eAgentSheet])
    (*aCollectorFunc)(mRuleProcessors[eAgentSheet], aData);
  nsRuleNode* lastAgentRN = aRuleWalker->CurrentNode();
  bool haveImportantUARules = !aRuleWalker->GetCheckForImportantRules();

  aRuleWalker->SetLevel(eUserSheet, false, true);
  bool skipUserStyles =
    aElement && aElement->IsInNativeAnonymousSubtree();
  if (!skipUserStyles && mRuleProcessors[eUserSheet]) 
    (*aCollectorFunc)(mRuleProcessors[eUserSheet], aData);
  nsRuleNode* lastUserRN = aRuleWalker->CurrentNode();
  bool haveImportantUserRules = !aRuleWalker->GetCheckForImportantRules();

  aRuleWalker->SetLevel(ePresHintSheet, false, false);
  if (mRuleProcessors[ePresHintSheet])
    (*aCollectorFunc)(mRuleProcessors[ePresHintSheet], aData);

  aRuleWalker->SetLevel(eSVGAttrAnimationSheet, false, false);
  if (mRuleProcessors[eSVGAttrAnimationSheet])
    (*aCollectorFunc)(mRuleProcessors[eSVGAttrAnimationSheet], aData);
  nsRuleNode* lastSVGAttrAnimationRN = aRuleWalker->CurrentNode();

  aRuleWalker->SetLevel(eDocSheet, false, true);
  bool cutOffInheritance = false;
  if (mBindingManager && aElement) {
    
    mBindingManager->WalkRules(aCollectorFunc,
                               static_cast<ElementDependentRuleProcessorData*>(aData),
                               &cutOffInheritance);
  }
  if (!skipUserStyles && !cutOffInheritance && 
      mRuleProcessors[eDocSheet])
    (*aCollectorFunc)(mRuleProcessors[eDocSheet], aData);
  nsRuleNode* lastDocRN = aRuleWalker->CurrentNode();
  bool haveImportantDocRules = !aRuleWalker->GetCheckForImportantRules();
  nsTArray<nsRuleNode*> lastScopedRNs;
  nsTArray<bool> haveImportantScopedRules;
  bool haveAnyImportantScopedRules = false;
  if (!skipUserStyles && !cutOffInheritance &&
      aElement && aElement->IsElementInStyleScope()) {
    lastScopedRNs.SetLength(mScopedDocSheetRuleProcessors.Length());
    haveImportantScopedRules.SetLength(mScopedDocSheetRuleProcessors.Length());
    for (uint32_t i = 0; i < mScopedDocSheetRuleProcessors.Length(); i++) {
      aRuleWalker->SetLevel(eScopedDocSheet, false, true);
      nsCSSRuleProcessor* processor =
        static_cast<nsCSSRuleProcessor*>(mScopedDocSheetRuleProcessors[i].get());
      aData->mScope = processor->GetScopeElement();
      (*aCollectorFunc)(mScopedDocSheetRuleProcessors[i], aData);
      lastScopedRNs[i] = aRuleWalker->CurrentNode();
      haveImportantScopedRules[i] = !aRuleWalker->GetCheckForImportantRules();
      haveAnyImportantScopedRules = haveAnyImportantScopedRules || haveImportantScopedRules[i];
    }
    aData->mScope = nullptr;
  }
  nsRuleNode* lastScopedRN = aRuleWalker->CurrentNode();
  aRuleWalker->SetLevel(eStyleAttrSheet, false, true);
  if (mRuleProcessors[eStyleAttrSheet])
    (*aCollectorFunc)(mRuleProcessors[eStyleAttrSheet], aData);
  nsRuleNode* lastStyleAttrRN = aRuleWalker->CurrentNode();
  bool haveImportantStyleAttrRules = !aRuleWalker->GetCheckForImportantRules();

  aRuleWalker->SetLevel(eOverrideSheet, false, true);
  if (mRuleProcessors[eOverrideSheet])
    (*aCollectorFunc)(mRuleProcessors[eOverrideSheet], aData);
  nsRuleNode* lastOvrRN = aRuleWalker->CurrentNode();
  bool haveImportantOverrideRules = !aRuleWalker->GetCheckForImportantRules();

  
  aRuleWalker->SetLevel(eAnimationSheet, false, false);
  (*aCollectorFunc)(mRuleProcessors[eAnimationSheet], aData);

  if (haveAnyImportantScopedRules) {
    for (uint32_t i = lastScopedRNs.Length(); i-- != 0; ) {
      aRuleWalker->SetLevel(eScopedDocSheet, true, false);
      nsRuleNode* startRN = lastScopedRNs[i];
      nsRuleNode* endRN = i == 0 ? lastDocRN : lastScopedRNs[i - 1];
      if (haveImportantScopedRules[i]) {
        AddImportantRules(startRN, endRN, aRuleWalker);  
      }
#ifdef DEBUG
      else {
        AssertNoImportantRules(startRN, endRN);
      }
#endif
    }
  }
#ifdef DEBUG
  else {
    AssertNoImportantRules(lastScopedRN, lastDocRN);
  }
#endif

  if (haveImportantDocRules) {
    aRuleWalker->SetLevel(eDocSheet, true, false);
    AddImportantRules(lastDocRN, lastSVGAttrAnimationRN, aRuleWalker);  
  }
#ifdef DEBUG
  else {
    AssertNoImportantRules(lastDocRN, lastSVGAttrAnimationRN);
  }
#endif

  if (haveImportantStyleAttrRules) {
    aRuleWalker->SetLevel(eStyleAttrSheet, true, false);
    AddImportantRules(lastStyleAttrRN, lastScopedRN, aRuleWalker);  
  }
#ifdef DEBUG
  else {
    AssertNoImportantRules(lastStyleAttrRN, lastScopedRN);
  }
#endif

  if (haveImportantOverrideRules) {
    aRuleWalker->SetLevel(eOverrideSheet, true, false);
    AddImportantRules(lastOvrRN, lastStyleAttrRN, aRuleWalker);  
  }
#ifdef DEBUG
  else {
    AssertNoImportantRules(lastOvrRN, lastStyleAttrRN);
  }
#endif

#ifdef DEBUG
  AssertNoCSSRules(lastSVGAttrAnimationRN, lastUserRN);
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
                               ElementDependentRuleProcessorData* aData,
                               bool aWalkAllXBLStylesheets)
{
  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  if (mRuleProcessors[eAgentSheet])
    (*aFunc)(mRuleProcessors[eAgentSheet], aData);

  bool skipUserStyles = aData->mElement->IsInNativeAnonymousSubtree();
  if (!skipUserStyles && mRuleProcessors[eUserSheet]) 
    (*aFunc)(mRuleProcessors[eUserSheet], aData);

  if (mRuleProcessors[ePresHintSheet])
    (*aFunc)(mRuleProcessors[ePresHintSheet], aData);

  if (mRuleProcessors[eSVGAttrAnimationSheet])
    (*aFunc)(mRuleProcessors[eSVGAttrAnimationSheet], aData);

  bool cutOffInheritance = false;
  if (mBindingManager) {
    
    if (aWalkAllXBLStylesheets) {
      mBindingManager->WalkAllRules(aFunc, aData);
    } else {
      mBindingManager->WalkRules(aFunc, aData, &cutOffInheritance);
    }
  }
  if (!skipUserStyles && !cutOffInheritance) {
    if (mRuleProcessors[eDocSheet]) 
      (*aFunc)(mRuleProcessors[eDocSheet], aData);
    if (aData->mElement->IsElementInStyleScope()) {
      for (uint32_t i = 0; i < mScopedDocSheetRuleProcessors.Length(); i++)
        (*aFunc)(mScopedDocSheetRuleProcessors[i], aData);
    }
  }
  if (mRuleProcessors[eStyleAttrSheet])
    (*aFunc)(mRuleProcessors[eStyleAttrSheet], aData);
  if (mRuleProcessors[eOverrideSheet])
    (*aFunc)(mRuleProcessors[eOverrideSheet], aData);
  (*aFunc)(mRuleProcessors[eAnimationSheet], aData);
  (*aFunc)(mRuleProcessors[eTransitionSheet], aData);
}

static void
InitStyleScopes(TreeMatchContext& aTreeContext, Element* aElement)
{
  if (aElement->IsElementInStyleScope()) {
    aTreeContext.InitStyleScopes(aElement->GetParentElementCrossingShadowRoot());
  }
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleFor(Element* aElement,
                            nsStyleContext* aParentContext)
{
  TreeMatchContext treeContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                               aElement->OwnerDoc());
  InitStyleScopes(treeContext, aElement);
  return ResolveStyleFor(aElement, aParentContext, treeContext);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleFor(Element* aElement,
                            nsStyleContext* aParentContext,
                            TreeMatchContext& aTreeMatchContext)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);
  NS_ASSERTION(aElement, "aElement must not be null");

  nsRuleWalker ruleWalker(mRuleTree, mAuthorStyleDisabled);
  aTreeMatchContext.ResetForUnvisitedMatching();
  ElementRuleProcessorData data(PresContext(), aElement, &ruleWalker,
                                aTreeMatchContext);
  WalkDisableTextZoomRule(aElement, &ruleWalker);
  FileRules(EnumRulesMatching<ElementRuleProcessorData>, &data, aElement,
            &ruleWalker);

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  nsRuleNode *visitedRuleNode = nullptr;

  if (aTreeMatchContext.HaveRelevantLink()) {
    aTreeMatchContext.ResetForVisitedMatching();
    ruleWalker.Reset();
    FileRules(EnumRulesMatching<ElementRuleProcessorData>, &data, aElement,
              &ruleWalker);
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  uint32_t flags = eDoAnimation;
  if (nsCSSRuleProcessor::IsLink(aElement)) {
    flags |= eIsLink;
  }
  if (nsCSSRuleProcessor::GetContentState(aElement, aTreeMatchContext).
                            HasState(NS_EVENT_STATE_VISITED)) {
    flags |= eIsVisitedLink;
  }
  if (aTreeMatchContext.mSkippingParentDisplayBasedStyleFixup) {
    flags |= eSkipParentDisplayBasedStyleFixup;
  }

  return GetContext(aParentContext, ruleNode, visitedRuleNode,
                    nullptr, nsCSSPseudoElements::ePseudo_NotPseudoElement,
                    aElement, flags);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleForRules(nsStyleContext* aParentContext,
                                 const nsTArray< nsCOMPtr<nsIStyleRule> > &aRules)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);

  nsRuleWalker ruleWalker(mRuleTree, mAuthorStyleDisabled);
  
  
  ruleWalker.SetLevel(eDocSheet, false, false);
  for (uint32_t i = 0; i < aRules.Length(); i++) {
    ruleWalker.ForwardOnPossiblyCSSRule(aRules.ElementAt(i));
  }

  return GetContext(aParentContext, ruleWalker.CurrentNode(), nullptr,
                    nullptr, nsCSSPseudoElements::ePseudo_NotPseudoElement,
                    nullptr, eNoFlags);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleByAddingRules(nsStyleContext* aBaseContext,
                                      const nsCOMArray<nsIStyleRule> &aRules)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);

  nsRuleWalker ruleWalker(mRuleTree, mAuthorStyleDisabled);
  ruleWalker.SetCurrentNode(aBaseContext->RuleNode());
  
  
  
  
  
  
  ruleWalker.SetLevel(eTransitionSheet, false, false);
  for (int32_t i = 0; i < aRules.Count(); i++) {
    ruleWalker.ForwardOnPossiblyCSSRule(aRules.ObjectAt(i));
  }

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  nsRuleNode *visitedRuleNode = nullptr;

  if (aBaseContext->GetStyleIfVisited()) {
    ruleWalker.SetCurrentNode(aBaseContext->GetStyleIfVisited()->RuleNode());
    for (int32_t i = 0; i < aRules.Count(); i++) {
      ruleWalker.ForwardOnPossiblyCSSRule(aRules.ObjectAt(i));
    }
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  uint32_t flags = eNoFlags;
  if (aBaseContext->IsLinkContext()) {
    flags |= eIsLink;

    
    
    
    if (aBaseContext->RelevantLinkVisited()) {
      flags |= eIsVisitedLink;
    }
  }
  return GetContext(aBaseContext->GetParent(), ruleNode, visitedRuleNode,
                    aBaseContext->GetPseudo(),
                    aBaseContext->GetPseudoType(),
                    nullptr, flags);
}

struct RuleNodeInfo {
  nsIStyleRule* mRule;
  uint8_t mLevel;
  bool mIsImportant;
};

struct CascadeLevel {
  uint8_t mLevel;
  bool mIsImportant;
  bool mCheckForImportantRules;
  nsRestyleHint mLevelReplacementHint;
};

static const CascadeLevel gCascadeLevels[] = {
  { nsStyleSet::eAgentSheet,            false, false, nsRestyleHint(0) },
  { nsStyleSet::eUserSheet,             false, false, nsRestyleHint(0) },
  { nsStyleSet::ePresHintSheet,         false, false, nsRestyleHint(0) },
  { nsStyleSet::eSVGAttrAnimationSheet, false, false, eRestyle_SVGAttrAnimations },
  { nsStyleSet::eDocSheet,              false, false, nsRestyleHint(0) },
  { nsStyleSet::eScopedDocSheet,        false, false, nsRestyleHint(0) },
  { nsStyleSet::eStyleAttrSheet,        false, true,  eRestyle_StyleAttribute |
                                                      eRestyle_StyleAttribute_Animations },
  { nsStyleSet::eOverrideSheet,         false, false, nsRestyleHint(0) },
  { nsStyleSet::eAnimationSheet,        false, false, eRestyle_CSSAnimations },
  { nsStyleSet::eScopedDocSheet,        true,  false, nsRestyleHint(0) },
  { nsStyleSet::eDocSheet,              true,  false, nsRestyleHint(0) },
  { nsStyleSet::eStyleAttrSheet,        true,  false, eRestyle_StyleAttribute |
                                                      eRestyle_StyleAttribute_Animations },
  { nsStyleSet::eOverrideSheet,         true,  false, nsRestyleHint(0) },
  { nsStyleSet::eUserSheet,             true,  false, nsRestyleHint(0) },
  { nsStyleSet::eAgentSheet,            true,  false, nsRestyleHint(0) },
  { nsStyleSet::eTransitionSheet,       false, false, eRestyle_CSSTransitions },
};

nsRuleNode*
nsStyleSet::RuleNodeWithReplacement(Element* aElement,
                                    Element* aPseudoElement,
                                    nsRuleNode* aOldRuleNode,
                                    nsCSSPseudoElements::Type aPseudoType,
                                    nsRestyleHint aReplacements)
{
  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  MOZ_ASSERT(!aPseudoElement ==
             (aPseudoType >= nsCSSPseudoElements::ePseudo_PseudoElementCount ||
              !(nsCSSPseudoElements::PseudoElementSupportsStyleAttribute(aPseudoType) ||
                nsCSSPseudoElements::PseudoElementSupportsUserActionState(aPseudoType))),
             "should have aPseudoElement only for certain pseudo elements");

  MOZ_ASSERT(!(aReplacements & ~(eRestyle_CSSTransitions |
                                 eRestyle_CSSAnimations |
                                 eRestyle_SVGAttrAnimations |
                                 eRestyle_StyleAttribute |
                                 eRestyle_StyleAttribute_Animations |
                                 eRestyle_Force |
                                 eRestyle_ForceDescendants)),
             "unexpected replacement bits");

  
  
  
  
  
  
  

  nsTArray<RuleNodeInfo> rules;
  for (nsRuleNode* ruleNode = aOldRuleNode; !ruleNode->IsRoot();
       ruleNode = ruleNode->GetParent()) {
    RuleNodeInfo* curRule = rules.AppendElement();
    curRule->mRule = ruleNode->GetRule();
    curRule->mLevel = ruleNode->GetLevel();
    curRule->mIsImportant = ruleNode->IsImportantRule();
  }

  nsRuleWalker ruleWalker(mRuleTree, mAuthorStyleDisabled);
  auto rulesIndex = rules.Length();

  
  
  nsRuleNode* lastScopedRN = nullptr;
  nsRuleNode* lastStyleAttrRN = nullptr;
  bool haveImportantStyleAttrRules = false;

  for (const CascadeLevel *level = gCascadeLevels,
                       *levelEnd = ArrayEnd(gCascadeLevels);
       level != levelEnd; ++level) {

    bool doReplace = level->mLevelReplacementHint & aReplacements;

    ruleWalker.SetLevel(level->mLevel, level->mIsImportant,
                        level->mCheckForImportantRules && doReplace);

    if (doReplace) {
      switch (level->mLevel) {
        case nsStyleSet::eAnimationSheet: {
          if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
              aPseudoType == nsCSSPseudoElements::ePseudo_before ||
              aPseudoType == nsCSSPseudoElements::ePseudo_after) {
            nsIStyleRule* rule = PresContext()->AnimationManager()->
              GetAnimationRule(aElement, aPseudoType);
            if (rule) {
              ruleWalker.ForwardOnPossiblyCSSRule(rule);
            }
          }
          break;
        }
        case nsStyleSet::eTransitionSheet: {
          if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
              aPseudoType == nsCSSPseudoElements::ePseudo_before ||
              aPseudoType == nsCSSPseudoElements::ePseudo_after) {
            nsIStyleRule* rule = PresContext()->TransitionManager()->
              GetAnimationRule(aElement, aPseudoType);
            if (rule) {
              ruleWalker.ForwardOnPossiblyCSSRule(rule);
            }
          }
          break;
        }
        case nsStyleSet::eSVGAttrAnimationSheet: {
          SVGAttrAnimationRuleProcessor* ruleProcessor =
            static_cast<SVGAttrAnimationRuleProcessor*>(
              mRuleProcessors[eSVGAttrAnimationSheet].get());
          if (ruleProcessor &&
              aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
            ruleProcessor->ElementRulesMatching(aElement, &ruleWalker);
          }
          break;
        }
        case nsStyleSet::eStyleAttrSheet: {
          if (!level->mIsImportant) {
            
            nsHTMLCSSStyleSheet* ruleProcessor =
              static_cast<nsHTMLCSSStyleSheet*>(
                mRuleProcessors[eStyleAttrSheet].get());
            if (ruleProcessor) {
              lastScopedRN = ruleWalker.CurrentNode();
              if (aPseudoType ==
                    nsCSSPseudoElements::ePseudo_NotPseudoElement) {
                ruleProcessor->ElementRulesMatching(PresContext(),
                                                    aElement,
                                                    &ruleWalker);
              } else if (aPseudoType <
                           nsCSSPseudoElements::ePseudo_PseudoElementCount &&
                         nsCSSPseudoElements::
                           PseudoElementSupportsStyleAttribute(aPseudoType)) {
                ruleProcessor->PseudoElementRulesMatching(aPseudoElement,
                                                          aPseudoType,
                                                          &ruleWalker);
              }
              lastStyleAttrRN = ruleWalker.CurrentNode();
              haveImportantStyleAttrRules =
                !ruleWalker.GetCheckForImportantRules();
            }
          } else {
            
            if (haveImportantStyleAttrRules) {
              AddImportantRules(lastStyleAttrRN, lastScopedRN, &ruleWalker);
            }
          }
          break;
        }
        default:
          MOZ_ASSERT(false, "unexpected result from gCascadeLevels lookup");
          break;
      }
    }

    while (rulesIndex != 0) {
      --rulesIndex;
      const RuleNodeInfo& ruleInfo = rules[rulesIndex];

      if (ruleInfo.mLevel != level->mLevel ||
          ruleInfo.mIsImportant != level->mIsImportant) {
        ++rulesIndex;
        break;
      }

      if (!doReplace) {
        ruleWalker.ForwardOnPossiblyCSSRule(ruleInfo.mRule);
      }
    }
  }

  NS_ASSERTION(rulesIndex == 0,
               "rules are in incorrect cascading order, "
               "which means we replaced them incorrectly");

  return ruleWalker.CurrentNode();
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleWithReplacement(Element* aElement,
                                        Element* aPseudoElement,
                                        nsStyleContext* aNewParentContext,
                                        nsStyleContext* aOldStyleContext,
                                        nsRestyleHint aReplacements,
                                        uint32_t aFlags)
{
  nsRuleNode* ruleNode =
    RuleNodeWithReplacement(aElement, aPseudoElement,
                            aOldStyleContext->RuleNode(),
                            aOldStyleContext->GetPseudoType(), aReplacements);

  nsRuleNode* visitedRuleNode = nullptr;
  nsStyleContext* oldStyleIfVisited = aOldStyleContext->GetStyleIfVisited();
  if (oldStyleIfVisited) {
    if (oldStyleIfVisited->RuleNode() == aOldStyleContext->RuleNode()) {
      visitedRuleNode = ruleNode;
    } else {
      visitedRuleNode =
        RuleNodeWithReplacement(aElement, aPseudoElement,
                                oldStyleIfVisited->RuleNode(),
                                oldStyleIfVisited->GetPseudoType(),
                                aReplacements);
    }
  }

  uint32_t flags = eNoFlags;
  if (aOldStyleContext->IsLinkContext()) {
    flags |= eIsLink;

    
    
    
    if (aOldStyleContext->RelevantLinkVisited()) {
      flags |= eIsVisitedLink;
    }
  }

  nsCSSPseudoElements::Type pseudoType = aOldStyleContext->GetPseudoType();
  Element* elementForAnimation = nullptr;
  if (!(aFlags & eSkipStartingAnimations) &&
      (pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
       pseudoType == nsCSSPseudoElements::ePseudo_before ||
       pseudoType == nsCSSPseudoElements::ePseudo_after)) {
    
    
    
    
    
    
    
    
    
    if (aReplacements & ~(eRestyle_CSSTransitions | eRestyle_CSSAnimations)) {
      flags |= eDoAnimation;
    }
    elementForAnimation = aElement;
    NS_ASSERTION(pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
                 !elementForAnimation->GetPrimaryFrame() ||
                 elementForAnimation->GetPrimaryFrame()->StyleContext()->
                     GetPseudoType() ==
                   nsCSSPseudoElements::ePseudo_NotPseudoElement,
                 "aElement should be the element and not the pseudo-element");
  }

  if (aElement && aElement->IsRootOfAnonymousSubtree()) {
    
    
    
    
    flags |= eSkipParentDisplayBasedStyleFixup;
  }

  return GetContext(aNewParentContext, ruleNode, visitedRuleNode,
                    aOldStyleContext->GetPseudo(), pseudoType,
                    elementForAnimation, flags);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleWithoutAnimation(dom::Element* aTarget,
                                         nsStyleContext* aStyleContext,
                                         nsRestyleHint aWhichToRemove)
{
#ifdef DEBUG
  nsCSSPseudoElements::Type pseudoType = aStyleContext->GetPseudoType();
#endif
  MOZ_ASSERT(pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
             pseudoType == nsCSSPseudoElements::ePseudo_before ||
             pseudoType == nsCSSPseudoElements::ePseudo_after,
             "unexpected type for animations");
  RestyleManager* restyleManager = PresContext()->RestyleManager();

  bool oldSkipAnimationRules = restyleManager->SkipAnimationRules();
  restyleManager->SetSkipAnimationRules(true);

  nsRefPtr<nsStyleContext> result =
    ResolveStyleWithReplacement(aTarget, nullptr, aStyleContext->GetParent(),
                                aStyleContext, aWhichToRemove,
                                eSkipStartingAnimations);

  restyleManager->SetSkipAnimationRules(oldSkipAnimationRules);

  return result.forget();
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveStyleForNonElement(nsStyleContext* aParentContext)
{
  return GetContext(aParentContext, mRuleTree, nullptr,
                    nsCSSAnonBoxes::mozNonElement,
                    nsCSSPseudoElements::ePseudo_AnonBox, nullptr,
                    eNoFlags);
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
  else if (aPseudoType == nsCSSPseudoElements::ePseudo_mozPlaceholder)
    aRuleWalker->Forward(mPlaceholderRule);
}

void
nsStyleSet::WalkDisableTextZoomRule(Element* aElement, nsRuleWalker* aRuleWalker)
{
  aRuleWalker->SetLevel(eAgentSheet, false, false);
  if (aElement->IsSVGElement(nsGkAtoms::text))
    aRuleWalker->Forward(mDisableTextZoomStyleRule);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolvePseudoElementStyle(Element* aParentElement,
                                      nsCSSPseudoElements::Type aType,
                                      nsStyleContext* aParentContext,
                                      Element* aPseudoElement)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);

  NS_ASSERTION(aType < nsCSSPseudoElements::ePseudo_PseudoElementCount,
               "must have pseudo element type");
  NS_ASSERTION(aParentElement, "Must have parent element");

  nsRuleWalker ruleWalker(mRuleTree, mAuthorStyleDisabled);
  TreeMatchContext treeContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                               aParentElement->OwnerDoc());
  InitStyleScopes(treeContext, aParentElement);
  PseudoElementRuleProcessorData data(PresContext(), aParentElement,
                                      &ruleWalker, aType, treeContext,
                                      aPseudoElement);
  WalkRestrictionRule(aType, &ruleWalker);
  FileRules(EnumRulesMatching<PseudoElementRuleProcessorData>, &data,
            aParentElement, &ruleWalker);

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  nsRuleNode *visitedRuleNode = nullptr;

  if (treeContext.HaveRelevantLink()) {
    treeContext.ResetForVisitedMatching();
    ruleWalker.Reset();
    WalkRestrictionRule(aType, &ruleWalker);
    FileRules(EnumRulesMatching<PseudoElementRuleProcessorData>, &data,
              aParentElement, &ruleWalker);
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  
  
  uint32_t flags = eNoFlags;
  if (aType == nsCSSPseudoElements::ePseudo_before ||
      aType == nsCSSPseudoElements::ePseudo_after) {
    flags |= eDoAnimation;
  } else {
    
    
    
    
    flags |= eSkipParentDisplayBasedStyleFixup;
  }

  return GetContext(aParentContext, ruleNode, visitedRuleNode,
                    nsCSSPseudoElements::GetPseudoAtom(aType), aType,
                    aParentElement, flags);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ProbePseudoElementStyle(Element* aParentElement,
                                    nsCSSPseudoElements::Type aType,
                                    nsStyleContext* aParentContext)
{
  TreeMatchContext treeContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                               aParentElement->OwnerDoc());
  InitStyleScopes(treeContext, aParentElement);
  return ProbePseudoElementStyle(aParentElement, aType, aParentContext,
                                 treeContext);
}

already_AddRefed<nsStyleContext>
nsStyleSet::ProbePseudoElementStyle(Element* aParentElement,
                                    nsCSSPseudoElements::Type aType,
                                    nsStyleContext* aParentContext,
                                    TreeMatchContext& aTreeMatchContext,
                                    Element* aPseudoElement)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);

  NS_ASSERTION(aType < nsCSSPseudoElements::ePseudo_PseudoElementCount,
               "must have pseudo element type");
  NS_ASSERTION(aParentElement, "aParentElement must not be null");

  nsIAtom* pseudoTag = nsCSSPseudoElements::GetPseudoAtom(aType);
  nsRuleWalker ruleWalker(mRuleTree, mAuthorStyleDisabled);
  aTreeMatchContext.ResetForUnvisitedMatching();
  PseudoElementRuleProcessorData data(PresContext(), aParentElement,
                                      &ruleWalker, aType, aTreeMatchContext,
                                      aPseudoElement);
  WalkRestrictionRule(aType, &ruleWalker);
  
  nsRuleNode *adjustedRoot = ruleWalker.CurrentNode();
  FileRules(EnumRulesMatching<PseudoElementRuleProcessorData>, &data,
            aParentElement, &ruleWalker);

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  if (ruleNode == adjustedRoot) {
    return nullptr;
  }

  nsRuleNode *visitedRuleNode = nullptr;

  if (aTreeMatchContext.HaveRelevantLink()) {
    aTreeMatchContext.ResetForVisitedMatching();
    ruleWalker.Reset();
    WalkRestrictionRule(aType, &ruleWalker);
    FileRules(EnumRulesMatching<PseudoElementRuleProcessorData>, &data,
              aParentElement, &ruleWalker);
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  
  
  uint32_t flags = eNoFlags;
  if (aType == nsCSSPseudoElements::ePseudo_before ||
      aType == nsCSSPseudoElements::ePseudo_after) {
    flags |= eDoAnimation;
  } else {
    
    
    
    
    flags |= eSkipParentDisplayBasedStyleFixup;
  }

  nsRefPtr<nsStyleContext> result =
    GetContext(aParentContext, ruleNode, visitedRuleNode,
               pseudoTag, aType,
               aParentElement, flags);

  
  
  
  if (result &&
      (pseudoTag == nsCSSPseudoElements::before ||
       pseudoTag == nsCSSPseudoElements::after)) {
    const nsStyleDisplay *display = result->StyleDisplay();
    const nsStyleContent *content = result->StyleContent();
    
    if (display->mDisplay == NS_STYLE_DISPLAY_NONE ||
        content->ContentCount() == 0) {
      result = nullptr;
    }
  }

  return result.forget();
}

already_AddRefed<nsStyleContext>
nsStyleSet::ResolveAnonymousBoxStyle(nsIAtom* aPseudoTag,
                                     nsStyleContext* aParentContext,
                                     uint32_t aFlags)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);

#ifdef DEBUG
    bool isAnonBox = nsCSSAnonBoxes::IsAnonBox(aPseudoTag)
#ifdef MOZ_XUL
                 && !nsCSSAnonBoxes::IsTreePseudoElement(aPseudoTag)
#endif
      ;
    NS_PRECONDITION(isAnonBox, "Unexpected pseudo");
#endif

  nsRuleWalker ruleWalker(mRuleTree, mAuthorStyleDisabled);
  AnonBoxRuleProcessorData data(PresContext(), aPseudoTag, &ruleWalker);
  FileRules(EnumRulesMatching<AnonBoxRuleProcessorData>, &data, nullptr,
            &ruleWalker);

  if (aPseudoTag == nsCSSAnonBoxes::pageContent) {
    
    nsTArray<nsCSSPageRule*> rules;
    nsTArray<css::ImportantRule*> importantRules;
    PresContext()->StyleSet()->AppendPageRules(rules);
    for (uint32_t i = 0, i_end = rules.Length(); i != i_end; ++i) {
      ruleWalker.Forward(rules[i]);
      css::ImportantRule* importantRule = rules[i]->GetImportantRule();
      if (importantRule) {
        importantRules.AppendElement(importantRule);
      }
    }
    for (uint32_t i = 0, i_end = importantRules.Length(); i != i_end; ++i) {
      ruleWalker.Forward(importantRules[i]);
    }
  }

  return GetContext(aParentContext, ruleWalker.CurrentNode(), nullptr,
                    aPseudoTag, nsCSSPseudoElements::ePseudo_AnonBox,
                    nullptr, aFlags);
}

#ifdef MOZ_XUL
already_AddRefed<nsStyleContext>
nsStyleSet::ResolveXULTreePseudoStyle(Element* aParentElement,
                                      nsIAtom* aPseudoTag,
                                      nsStyleContext* aParentContext,
                                      nsICSSPseudoComparator* aComparator)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);

  NS_ASSERTION(aPseudoTag, "must have pseudo tag");
  NS_ASSERTION(nsCSSAnonBoxes::IsTreePseudoElement(aPseudoTag),
               "Unexpected pseudo");

  nsRuleWalker ruleWalker(mRuleTree, mAuthorStyleDisabled);
  TreeMatchContext treeContext(true, nsRuleWalker::eRelevantLinkUnvisited,
                               aParentElement->OwnerDoc());
  InitStyleScopes(treeContext, aParentElement);
  XULTreeRuleProcessorData data(PresContext(), aParentElement, &ruleWalker,
                                aPseudoTag, aComparator, treeContext);
  FileRules(EnumRulesMatching<XULTreeRuleProcessorData>, &data, aParentElement,
            &ruleWalker);

  nsRuleNode *ruleNode = ruleWalker.CurrentNode();
  nsRuleNode *visitedRuleNode = nullptr;

  if (treeContext.HaveRelevantLink()) {
    treeContext.ResetForVisitedMatching();
    ruleWalker.Reset();
    FileRules(EnumRulesMatching<XULTreeRuleProcessorData>, &data,
              aParentElement, &ruleWalker);
    visitedRuleNode = ruleWalker.CurrentNode();
  }

  return GetContext(aParentContext, ruleNode, visitedRuleNode,
                    
                    
                    aPseudoTag, nsCSSPseudoElements::ePseudo_XULTree,
                    nullptr, eNoFlags);
}
#endif

bool
nsStyleSet::AppendFontFaceRules(nsTArray<nsFontFaceRuleContainer>& aArray)
{
  NS_ENSURE_FALSE(mInShutdown, false);
  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  nsPresContext* presContext = PresContext();
  for (uint32_t i = 0; i < ArrayLength(gCSSSheetTypes); ++i) {
    if (gCSSSheetTypes[i] == eScopedDocSheet)
      continue;
    nsCSSRuleProcessor *ruleProc = static_cast<nsCSSRuleProcessor*>
                                    (mRuleProcessors[gCSSSheetTypes[i]].get());
    if (ruleProc && !ruleProc->AppendFontFaceRules(presContext, aArray))
      return false;
  }
  return true;
}

nsCSSKeyframesRule*
nsStyleSet::KeyframesRuleForName(const nsString& aName)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);
  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  nsPresContext* presContext = PresContext();
  for (uint32_t i = ArrayLength(gCSSSheetTypes); i-- != 0; ) {
    if (gCSSSheetTypes[i] == eScopedDocSheet)
      continue;
    nsCSSRuleProcessor *ruleProc = static_cast<nsCSSRuleProcessor*>
                                    (mRuleProcessors[gCSSSheetTypes[i]].get());
    if (!ruleProc)
      continue;
    nsCSSKeyframesRule* result =
      ruleProc->KeyframesRuleForName(presContext, aName);
    if (result)
      return result;
  }
  return nullptr;
}

nsCSSCounterStyleRule*
nsStyleSet::CounterStyleRuleForName(const nsAString& aName)
{
  NS_ENSURE_FALSE(mInShutdown, nullptr);
  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  nsPresContext* presContext = PresContext();
  for (uint32_t i = ArrayLength(gCSSSheetTypes); i-- != 0; ) {
    if (gCSSSheetTypes[i] == eScopedDocSheet)
      continue;
    nsCSSRuleProcessor *ruleProc = static_cast<nsCSSRuleProcessor*>
                                    (mRuleProcessors[gCSSSheetTypes[i]].get());
    if (!ruleProc)
      continue;
    nsCSSCounterStyleRule *result =
      ruleProc->CounterStyleRuleForName(presContext, aName);
    if (result)
      return result;
  }
  return nullptr;
}

bool
nsStyleSet::AppendFontFeatureValuesRules(
                                 nsTArray<nsCSSFontFeatureValuesRule*>& aArray)
{
  NS_ENSURE_FALSE(mInShutdown, false);
  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  nsPresContext* presContext = PresContext();
  for (uint32_t i = 0; i < ArrayLength(gCSSSheetTypes); ++i) {
    nsCSSRuleProcessor *ruleProc = static_cast<nsCSSRuleProcessor*>
                                    (mRuleProcessors[gCSSSheetTypes[i]].get());
    if (ruleProc &&
        !ruleProc->AppendFontFeatureValuesRules(presContext, aArray))
    {
      return false;
    }
  }
  return true;
}

already_AddRefed<gfxFontFeatureValueSet>
nsStyleSet::GetFontFeatureValuesLookup()
{
  if (mInitFontFeatureValuesLookup) {
    mInitFontFeatureValuesLookup = false;

    nsTArray<nsCSSFontFeatureValuesRule*> rules;
    AppendFontFeatureValuesRules(rules);

    mFontFeatureValuesLookup = new gfxFontFeatureValueSet();

    uint32_t i, numRules = rules.Length();
    for (i = 0; i < numRules; i++) {
      nsCSSFontFeatureValuesRule *rule = rules[i];

      const nsTArray<FontFamilyName>& familyList = rule->GetFamilyList().GetFontlist();
      const nsTArray<gfxFontFeatureValueSet::FeatureValues>&
        featureValues = rule->GetFeatureValues();

      
      size_t f, numFam;

      numFam = familyList.Length();
      for (f = 0; f < numFam; f++) {
        mFontFeatureValuesLookup->AddFontFeatureValues(familyList[f].mName,
                                                       featureValues);
      }
    }
  }

  nsRefPtr<gfxFontFeatureValueSet> lookup = mFontFeatureValuesLookup;
  return lookup.forget();
}

bool
nsStyleSet::AppendPageRules(nsTArray<nsCSSPageRule*>& aArray)
{
  NS_ENSURE_FALSE(mInShutdown, false);
  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  nsPresContext* presContext = PresContext();
  for (uint32_t i = 0; i < ArrayLength(gCSSSheetTypes); ++i) {
    if (gCSSSheetTypes[i] == eScopedDocSheet)
      continue;
    nsCSSRuleProcessor* ruleProc = static_cast<nsCSSRuleProcessor*>
                                    (mRuleProcessors[gCSSSheetTypes[i]].get());
    if (ruleProc && !ruleProc->AppendPageRules(presContext, aArray))
      return false;
  }
  return true;
}

void
nsStyleSet::BeginShutdown()
{
  mInShutdown = 1;
  mRoots.Clear(); 
}

void
nsStyleSet::Shutdown()
{
  mRuleTree->Destroy();
  mRuleTree = nullptr;

  
  
  
  for (uint32_t i = mOldRuleTrees.Length(); i > 0; ) {
    --i;
    mOldRuleTrees[i]->Destroy();
  }
  mOldRuleTrees.Clear();
}

static const uint32_t kGCInterval = 300;

void
nsStyleSet::NotifyStyleContextDestroyed(nsStyleContext* aStyleContext)
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

  
  
  
  
  for (int32_t i = mRoots.Length() - 1; i >= 0; --i) {
    mRoots[i]->Mark();
  }

  
#ifdef DEBUG
  bool deleted =
#endif
    mRuleTree->Sweep();
  NS_ASSERTION(!deleted, "Root node must not be gc'd");

  
  for (uint32_t i = mOldRuleTrees.Length(); i > 0; ) {
    --i;
    if (mOldRuleTrees[i]->Sweep()) {
      
      mOldRuleTrees.RemoveElementAt(i);
    } else {
      NS_NOTREACHED("old rule tree still referenced");
    }
  }
}

already_AddRefed<nsStyleContext>
nsStyleSet::ReparentStyleContext(nsStyleContext* aStyleContext,
                                 nsStyleContext* aNewParentContext,
                                 Element* aElement)
{
  MOZ_ASSERT(aStyleContext, "aStyleContext must not be null");

  
  
  if (aStyleContext->GetParent() == aNewParentContext) {
    nsRefPtr<nsStyleContext> ret = aStyleContext;
    return ret.forget();
  }

  nsIAtom* pseudoTag = aStyleContext->GetPseudo();
  nsCSSPseudoElements::Type pseudoType = aStyleContext->GetPseudoType();
  nsRuleNode* ruleNode = aStyleContext->RuleNode();

  NS_ASSERTION(!PresContext()->RestyleManager()->SkipAnimationRules(),
               "we no longer handle SkipAnimationRules()");

  nsRuleNode* visitedRuleNode = nullptr;
  nsStyleContext* visitedContext = aStyleContext->GetStyleIfVisited();
  
  
  
  
  if (visitedContext) {
    visitedRuleNode = visitedContext->RuleNode();
  }

  uint32_t flags = eNoFlags;
  if (aStyleContext->IsLinkContext()) {
    flags |= eIsLink;

    
    
    
    if (aStyleContext->RelevantLinkVisited()) {
      flags |= eIsVisitedLink;
    }
  }

  if (pseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
      pseudoType == nsCSSPseudoElements::ePseudo_before ||
      pseudoType == nsCSSPseudoElements::ePseudo_after) {
    flags |= eDoAnimation;
  }

  if (aElement && aElement->IsRootOfAnonymousSubtree()) {
    
    
    
    
    flags |= eSkipParentDisplayBasedStyleFixup;
  }

  return GetContext(aNewParentContext, ruleNode, visitedRuleNode,
                    pseudoTag, pseudoType,
                    aElement, flags);
}

struct MOZ_STACK_CLASS StatefulData : public StateRuleProcessorData {
  StatefulData(nsPresContext* aPresContext, Element* aElement,
               EventStates aStateMask, TreeMatchContext& aTreeMatchContext)
    : StateRuleProcessorData(aPresContext, aElement, aStateMask,
                             aTreeMatchContext),
      mHint(nsRestyleHint(0))
  {}
  nsRestyleHint   mHint;
};

struct MOZ_STACK_CLASS StatefulPseudoElementData : public PseudoElementStateRuleProcessorData {
  StatefulPseudoElementData(nsPresContext* aPresContext, Element* aElement,
               EventStates aStateMask, nsCSSPseudoElements::Type aPseudoType,
               TreeMatchContext& aTreeMatchContext, Element* aPseudoElement)
    : PseudoElementStateRuleProcessorData(aPresContext, aElement, aStateMask,
                                          aPseudoType, aTreeMatchContext,
                                          aPseudoElement),
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
nsStyleSet::HasDocumentStateDependentStyle(nsIContent*    aContent,
                                           EventStates    aStateMask)
{
  if (!aContent || !aContent->IsElement())
    return false;

  TreeMatchContext treeContext(false, nsRuleWalker::eLinksVisitedOrUnvisited,
                               aContent->OwnerDoc());
  InitStyleScopes(treeContext, aContent->AsElement());
  StatefulData data(PresContext(), aContent->AsElement(), aStateMask,
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

static bool SheetHasStatefulPseudoElementStyle(nsIStyleRuleProcessor* aProcessor,
                                               void *aData)
{
  StatefulPseudoElementData* data = (StatefulPseudoElementData*)aData;
  nsRestyleHint hint = aProcessor->HasStateDependentStyle(data);
  data->mHint = nsRestyleHint(data->mHint | hint);
  return true; 
}


nsRestyleHint
nsStyleSet::HasStateDependentStyle(Element*             aElement,
                                   EventStates          aStateMask)
{
  TreeMatchContext treeContext(false, nsRuleWalker::eLinksVisitedOrUnvisited,
                               aElement->OwnerDoc());
  InitStyleScopes(treeContext, aElement);
  StatefulData data(PresContext(), aElement, aStateMask, treeContext);
  WalkRuleProcessors(SheetHasStatefulStyle, &data, false);
  return data.mHint;
}

nsRestyleHint
nsStyleSet::HasStateDependentStyle(Element* aElement,
                                   nsCSSPseudoElements::Type aPseudoType,
                                   Element* aPseudoElement,
                                   EventStates aStateMask)
{
  TreeMatchContext treeContext(false, nsRuleWalker::eLinksVisitedOrUnvisited,
                               aElement->OwnerDoc());
  InitStyleScopes(treeContext, aElement);
  StatefulPseudoElementData data(PresContext(), aElement, aStateMask,
                                 aPseudoType, treeContext, aPseudoElement);
  WalkRuleProcessors(SheetHasStatefulPseudoElementStyle, &data, false);
  return data.mHint;
}

struct MOZ_STACK_CLASS AttributeData : public AttributeRuleProcessorData {
  AttributeData(nsPresContext* aPresContext,
                Element* aElement, nsIAtom* aAttribute, int32_t aModType,
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
nsStyleSet::HasAttributeDependentStyle(Element*       aElement,
                                       nsIAtom*       aAttribute,
                                       int32_t        aModType,
                                       bool           aAttrHasChanged)
{
  TreeMatchContext treeContext(false, nsRuleWalker::eLinksVisitedOrUnvisited,
                               aElement->OwnerDoc());
  InitStyleScopes(treeContext, aElement);
  AttributeData data(PresContext(), aElement, aAttribute,
                     aModType, aAttrHasChanged, treeContext);
  WalkRuleProcessors(SheetHasAttributeStyle, &data, false);
  return data.mHint;
}

bool
nsStyleSet::MediumFeaturesChanged()
{
  NS_ASSERTION(mBatching == 0, "rule processors out of date");

  
  nsPresContext* presContext = PresContext();
  bool stylesChanged = false;
  for (uint32_t i = 0; i < ArrayLength(mRuleProcessors); ++i) {
    nsIStyleRuleProcessor *processor = mRuleProcessors[i];
    if (!processor) {
      continue;
    }
    bool thisChanged = processor->MediumFeaturesChanged(presContext);
    stylesChanged = stylesChanged || thisChanged;
  }
  for (uint32_t i = 0; i < mScopedDocSheetRuleProcessors.Length(); ++i) {
    nsIStyleRuleProcessor *processor = mScopedDocSheetRuleProcessors[i];
    bool thisChanged = processor->MediumFeaturesChanged(presContext);
    stylesChanged = stylesChanged || thisChanged;
  }

  if (mBindingManager) {
    bool thisChanged = false;
    mBindingManager->MediumFeaturesChanged(presContext, &thisChanged);
    stylesChanged = stylesChanged || thisChanged;
  }

  return stylesChanged;
}

bool
nsStyleSet::EnsureUniqueInnerOnCSSSheets()
{
  nsAutoTArray<CSSStyleSheet*, 32> queue;
  for (uint32_t i = 0; i < ArrayLength(gCSSSheetTypes); ++i) {
    nsCOMArray<nsIStyleSheet> &sheets = mSheets[gCSSSheetTypes[i]];
    for (uint32_t j = 0, j_end = sheets.Count(); j < j_end; ++j) {
      CSSStyleSheet* sheet = static_cast<CSSStyleSheet*>(sheets[j]);
      queue.AppendElement(sheet);
    }
  }

  if (mBindingManager) {
    mBindingManager->AppendAllSheets(queue);
  }

  while (!queue.IsEmpty()) {
    uint32_t idx = queue.Length() - 1;
    CSSStyleSheet* sheet = queue[idx];
    queue.RemoveElementAt(idx);

    sheet->EnsureUniqueInner();

    
    sheet->AppendAllChildSheets(queue);
  }

  bool res = mNeedsRestyleAfterEnsureUniqueInner;
  mNeedsRestyleAfterEnsureUniqueInner = false;
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
