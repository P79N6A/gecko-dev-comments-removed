


















#include "nsHTMLStyleSheet.h"
#include "nsMappedAttributes.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsEventStates.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsStyleConsts.h"
#include "nsRuleWalker.h"
#include "nsRuleData.h"
#include "nsError.h"
#include "nsRuleProcessorData.h"
#include "nsCSSRuleProcessor.h"
#include "mozilla/dom/Element.h"
#include "nsCSSFrameConstructor.h"

using namespace mozilla::dom;

NS_IMPL_ISUPPORTS1(nsHTMLStyleSheet::HTMLColorRule, nsIStyleRule)

 void
nsHTMLStyleSheet::HTMLColorRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Color)) {
    nsCSSValue* color = aRuleData->ValueForColor();
    if (color->GetUnit() == eCSSUnit_Null &&
        aRuleData->mPresContext->UseDocumentColors())
      color->SetColorValue(mColor);
  }
}

#ifdef DEBUG
 void
nsHTMLStyleSheet::HTMLColorRule::List(FILE* out, int32_t aIndent) const
{
  for (int32_t index = aIndent; --index >= 0; ) fputs("  ", out);
  fputs("[html color rule] {}\n", out);
}
#endif

 
NS_IMPL_ISUPPORTS1(nsHTMLStyleSheet::GenericTableRule, nsIStyleRule)

#ifdef DEBUG
 void
nsHTMLStyleSheet::GenericTableRule::List(FILE* out, int32_t aIndent) const
{
  for (int32_t index = aIndent; --index >= 0; ) fputs("  ", out);
  fputs("[generic table rule] {}\n", out);
}
#endif

 void
nsHTMLStyleSheet::TableTHRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Text)) {
    nsCSSValue* textAlign = aRuleData->ValueForTextAlign();
    if (textAlign->GetUnit() == eCSSUnit_Null) {
      textAlign->SetIntValue(NS_STYLE_TEXT_ALIGN_MOZ_CENTER_OR_INHERIT,
                             eCSSUnit_Enumerated);
    }
  }
}

 void
nsHTMLStyleSheet::TableQuirkColorRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Color)) {
    nsCSSValue* color = aRuleData->ValueForColor();
    
    
    if (color->GetUnit() == eCSSUnit_Null)
      color->SetIntValue(NS_STYLE_COLOR_INHERIT_FROM_BODY,
                         eCSSUnit_Enumerated);
  }
}



struct MappedAttrTableEntry : public PLDHashEntryHdr {
  nsMappedAttributes *mAttributes;
};

static PLDHashNumber
MappedAttrTable_HashKey(PLDHashTable *table, const void *key)
{
  nsMappedAttributes *attributes =
    static_cast<nsMappedAttributes*>(const_cast<void*>(key));

  return attributes->HashValue();
}

static void
MappedAttrTable_ClearEntry(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  MappedAttrTableEntry *entry = static_cast<MappedAttrTableEntry*>(hdr);

  entry->mAttributes->DropStyleSheetReference();
  memset(entry, 0, sizeof(MappedAttrTableEntry));
}

static bool
MappedAttrTable_MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                           const void *key)
{
  nsMappedAttributes *attributes =
    static_cast<nsMappedAttributes*>(const_cast<void*>(key));
  const MappedAttrTableEntry *entry =
    static_cast<const MappedAttrTableEntry*>(hdr);

  return attributes->Equals(entry->mAttributes);
}

static PLDHashTableOps MappedAttrTable_Ops = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  MappedAttrTable_HashKey,
  MappedAttrTable_MatchEntry,
  PL_DHashMoveEntryStub,
  MappedAttrTable_ClearEntry,
  PL_DHashFinalizeStub,
  NULL
};



nsHTMLStyleSheet::nsHTMLStyleSheet(nsIURI* aURL, nsIDocument* aDocument)
  : mURL(aURL)
  , mDocument(aDocument)
  , mTableQuirkColorRule(new TableQuirkColorRule())
  , mTableTHRule(new TableTHRule())
{
  MOZ_ASSERT(aURL);
  MOZ_ASSERT(aDocument);
  mMappedAttrTable.ops = nullptr;
}

nsHTMLStyleSheet::~nsHTMLStyleSheet()
{
  if (mMappedAttrTable.ops)
    PL_DHashTableFinish(&mMappedAttrTable);
}

NS_IMPL_ISUPPORTS2(nsHTMLStyleSheet, nsIStyleSheet, nsIStyleRuleProcessor)

 void
nsHTMLStyleSheet::RulesMatching(ElementRuleProcessorData* aData)
{
  nsRuleWalker *ruleWalker = aData->mRuleWalker;
  if (aData->mElement->IsHTML()) {
    nsIAtom* tag = aData->mElement->Tag();

    
    if (tag == nsGkAtoms::a) {
      if (mLinkRule || mVisitedRule || mActiveRule) {
        nsEventStates state = nsCSSRuleProcessor::GetContentStateForVisitedHandling(
                                  aData->mElement,
                                  aData->mTreeMatchContext,
                                  aData->mTreeMatchContext.VisitedHandling(),
                                  
                                  
                                  nsCSSRuleProcessor::IsLink(aData->mElement));
        if (mLinkRule && state.HasState(NS_EVENT_STATE_UNVISITED)) {
          ruleWalker->Forward(mLinkRule);
          aData->mTreeMatchContext.SetHaveRelevantLink();
        }
        else if (mVisitedRule && state.HasState(NS_EVENT_STATE_VISITED)) {
          ruleWalker->Forward(mVisitedRule);
          aData->mTreeMatchContext.SetHaveRelevantLink();
        }

        
        if (mActiveRule && nsCSSRuleProcessor::IsLink(aData->mElement) &&
            state.HasState(NS_EVENT_STATE_ACTIVE)) {
          ruleWalker->Forward(mActiveRule);
        }
      } 
    } 
    
    else if (tag == nsGkAtoms::th) {
      ruleWalker->Forward(mTableTHRule);
    }
    else if (tag == nsGkAtoms::table) {
      if (aData->mTreeMatchContext.mCompatMode == eCompatibility_NavQuirks) {
        ruleWalker->Forward(mTableQuirkColorRule);
      }
    }
  } 

    
  aData->mElement->WalkContentStyleRules(ruleWalker);
}


 nsRestyleHint
nsHTMLStyleSheet::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  if (aData->mElement->IsHTML(nsGkAtoms::a) &&
      nsCSSRuleProcessor::IsLink(aData->mElement) &&
      ((mActiveRule && aData->mStateMask.HasState(NS_EVENT_STATE_ACTIVE)) ||
       (mLinkRule && aData->mStateMask.HasState(NS_EVENT_STATE_VISITED)) ||
       (mVisitedRule && aData->mStateMask.HasState(NS_EVENT_STATE_VISITED)))) {
    return eRestyle_Self;
  }
  
  return nsRestyleHint(0);
}

 bool
nsHTMLStyleSheet::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  return false;
}

 nsRestyleHint
nsHTMLStyleSheet::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  
  if (!aData->mAttrHasChanged) {
    return nsRestyleHint(0);
  }

  
  
  

  
  Element *element = aData->mElement;
  if (aData->mAttribute == nsGkAtoms::href &&
      (mLinkRule || mVisitedRule || mActiveRule) &&
      element->IsHTML(nsGkAtoms::a)) {
    return eRestyle_Self;
  }

  
  

  
  if (element->IsAttributeMapped(aData->mAttribute)) {
    
    
    if (aData->mAttribute == nsGkAtoms::cellpadding &&
        element->IsHTML(nsGkAtoms::table)) {
      return eRestyle_Subtree;
    }
    return eRestyle_Self;
  }

  return nsRestyleHint(0);
}

 bool
nsHTMLStyleSheet::MediumFeaturesChanged(nsPresContext* aPresContext)
{
  return false;
}

 size_t
nsHTMLStyleSheet::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return 0; 
}

 size_t
nsHTMLStyleSheet::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return 0; 
}

 void
nsHTMLStyleSheet::RulesMatching(PseudoElementRuleProcessorData* aData)
{
}

 void
nsHTMLStyleSheet::RulesMatching(AnonBoxRuleProcessorData* aData)
{
}

#ifdef MOZ_XUL
 void
nsHTMLStyleSheet::RulesMatching(XULTreeRuleProcessorData* aData)
{
}
#endif

  
 nsIURI*
nsHTMLStyleSheet::GetSheetURI() const
{
  return mURL;
}

 nsIURI*
nsHTMLStyleSheet::GetBaseURI() const
{
  return mURL;
}

 void
nsHTMLStyleSheet::GetTitle(nsString& aTitle) const
{
  aTitle.Truncate();
}

 void
nsHTMLStyleSheet::GetType(nsString& aType) const
{
  aType.AssignLiteral("text/html");
}

 bool
nsHTMLStyleSheet::HasRules() const
{
  return true; 
}

 bool
nsHTMLStyleSheet::IsApplicable() const
{
  return true;
}

 void
nsHTMLStyleSheet::SetEnabled(bool aEnabled)
{ 
}

 bool
nsHTMLStyleSheet::IsComplete() const
{
  return true;
}

 void
nsHTMLStyleSheet::SetComplete()
{
}

 nsIStyleSheet*
nsHTMLStyleSheet::GetParentSheet() const
{
  return nullptr;
}

 nsIDocument*
nsHTMLStyleSheet::GetOwningDocument() const
{
  return mDocument;
}

 void
nsHTMLStyleSheet::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument; 
}

void
nsHTMLStyleSheet::Reset(nsIURI* aURL)
{
  mURL = aURL;

  mLinkRule          = nullptr;
  mVisitedRule       = nullptr;
  mActiveRule        = nullptr;

  if (mMappedAttrTable.ops) {
    PL_DHashTableFinish(&mMappedAttrTable);
    mMappedAttrTable.ops = nullptr;
  }
}

nsresult
nsHTMLStyleSheet::ImplLinkColorSetter(nsRefPtr<HTMLColorRule>& aRule, nscolor aColor)
{
  if (aRule && aRule->mColor == aColor) {
    return NS_OK;
  }

  aRule = new HTMLColorRule();
  if (!aRule)
    return NS_ERROR_OUT_OF_MEMORY;

  aRule->mColor = aColor;
  
  
  if (mDocument && mDocument->GetShell()) {
    Element* root = mDocument->GetRootElement();
    if (root) {
      mDocument->GetShell()->FrameConstructor()->
        PostRestyleEvent(root, eRestyle_Subtree, NS_STYLE_HINT_NONE);
    }
  }
  return NS_OK;
}

nsresult
nsHTMLStyleSheet::SetLinkColor(nscolor aColor)
{
  return ImplLinkColorSetter(mLinkRule, aColor);
}


nsresult
nsHTMLStyleSheet::SetActiveLinkColor(nscolor aColor)
{
  return ImplLinkColorSetter(mActiveRule, aColor);
}

nsresult
nsHTMLStyleSheet::SetVisitedLinkColor(nscolor aColor)
{
  return ImplLinkColorSetter(mVisitedRule, aColor);
}

already_AddRefed<nsMappedAttributes>
nsHTMLStyleSheet::UniqueMappedAttributes(nsMappedAttributes* aMapped)
{
  if (!mMappedAttrTable.ops) {
    bool res = PL_DHashTableInit(&mMappedAttrTable, &MappedAttrTable_Ops,
                                   nullptr, sizeof(MappedAttrTableEntry), 16);
    if (!res) {
      mMappedAttrTable.ops = nullptr;
      return nullptr;
    }
  }
  MappedAttrTableEntry *entry = static_cast<MappedAttrTableEntry*>
                                           (PL_DHashTableOperate(&mMappedAttrTable, aMapped, PL_DHASH_ADD));
  if (!entry)
    return nullptr;
  if (!entry->mAttributes) {
    
    entry->mAttributes = aMapped;
  }
  NS_ADDREF(entry->mAttributes); 
  return entry->mAttributes;
}

void
nsHTMLStyleSheet::DropMappedAttributes(nsMappedAttributes* aMapped)
{
  NS_ENSURE_TRUE_VOID(aMapped);

  NS_ASSERTION(mMappedAttrTable.ops, "table uninitialized");
#ifdef DEBUG
  uint32_t entryCount = mMappedAttrTable.entryCount - 1;
#endif

  PL_DHashTableOperate(&mMappedAttrTable, aMapped, PL_DHASH_REMOVE);

  NS_ASSERTION(entryCount == mMappedAttrTable.entryCount, "not removed");
}

#ifdef DEBUG
 void
nsHTMLStyleSheet::List(FILE* out, int32_t aIndent) const
{
  
  for (int32_t index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("HTML Style Sheet: ", out);
  nsAutoCString urlSpec;
  mURL->GetSpec(urlSpec);
  if (!urlSpec.IsEmpty()) {
    fputs(urlSpec.get(), out);
  }
  fputs("\n", out);
}
#endif

static size_t
SizeOfAttributesEntryExcludingThis(PLDHashEntryHdr* aEntry,
                                   nsMallocSizeOfFun aMallocSizeOf,
                                   void* aArg)
{
  NS_PRECONDITION(aEntry, "The entry should not be null!");

  MappedAttrTableEntry* entry = static_cast<MappedAttrTableEntry*>(aEntry);
  NS_ASSERTION(entry->mAttributes, "entry->mAttributes should not be null!");
  return entry->mAttributes->SizeOfIncludingThis(aMallocSizeOf);
}

size_t
nsHTMLStyleSheet::DOMSizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);

  if (mMappedAttrTable.ops) {
    n += PL_DHashTableSizeOfExcludingThis(&mMappedAttrTable,
                                          SizeOfAttributesEntryExcludingThis,
                                          aMallocSizeOf);
  }

  
  
  
  
  
  
  
  
  
  
  

  return n;
}
