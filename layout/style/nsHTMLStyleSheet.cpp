


















































#include "nsHTMLStyleSheet.h"
#include "nsINameSpaceManager.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsMappedAttributes.h"
#include "nsILink.h"
#include "nsIFrame.h"
#include "nsStyleContext.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsEventStates.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsStyleConsts.h"
#include "nsCSSAnonBoxes.h"
#include "nsRuleWalker.h"
#include "nsRuleData.h"
#include "nsContentErrors.h"
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
nsHTMLStyleSheet::HTMLColorRule::List(FILE* out, PRInt32 aIndent) const
{
}
#endif

 
NS_IMPL_ISUPPORTS1(nsHTMLStyleSheet::GenericTableRule, nsIStyleRule)

#ifdef DEBUG
 void
nsHTMLStyleSheet::GenericTableRule::List(FILE* out, PRInt32 aIndent) const
{
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

static PRBool
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



nsHTMLStyleSheet::nsHTMLStyleSheet(void)
  : mDocument(nsnull)
{
  mMappedAttrTable.ops = nsnull;
}

nsresult
nsHTMLStyleSheet::Init()
{
  mTableTHRule = new TableTHRule();
  mTableQuirkColorRule = new TableQuirkColorRule();
  return NS_OK;
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

 PRBool
nsHTMLStyleSheet::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  return PR_FALSE;
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

 PRBool
nsHTMLStyleSheet::MediumFeaturesChanged(nsPresContext* aPresContext)
{
  return PR_FALSE;
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

 PRBool
nsHTMLStyleSheet::HasRules() const
{
  return PR_TRUE; 
}

 PRBool
nsHTMLStyleSheet::IsApplicable() const
{
  return PR_TRUE;
}

 void
nsHTMLStyleSheet::SetEnabled(PRBool aEnabled)
{ 
}

 PRBool
nsHTMLStyleSheet::IsComplete() const
{
  return PR_TRUE;
}

 void
nsHTMLStyleSheet::SetComplete()
{
}

 nsIStyleSheet*
nsHTMLStyleSheet::GetParentSheet() const
{
  return nsnull;
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

nsresult
nsHTMLStyleSheet::Init(nsIURI* aURL, nsIDocument* aDocument)
{
  NS_PRECONDITION(aURL && aDocument, "null ptr");
  if (! aURL || ! aDocument)
    return NS_ERROR_NULL_POINTER;

  if (mURL || mDocument)
    return NS_ERROR_ALREADY_INITIALIZED;

  mDocument = aDocument; 
  mURL = aURL;
  return NS_OK;
}

void
nsHTMLStyleSheet::Reset(nsIURI* aURL)
{
  mURL = aURL;

  mLinkRule          = nsnull;
  mVisitedRule       = nsnull;
  mActiveRule        = nsnull;

  if (mMappedAttrTable.ops) {
    PL_DHashTableFinish(&mMappedAttrTable);
    mMappedAttrTable.ops = nsnull;
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
    PRBool res = PL_DHashTableInit(&mMappedAttrTable, &MappedAttrTable_Ops,
                                   nsnull, sizeof(MappedAttrTableEntry), 16);
    if (!res) {
      mMappedAttrTable.ops = nsnull;
      return nsnull;
    }
  }
  MappedAttrTableEntry *entry = static_cast<MappedAttrTableEntry*>
                                           (PL_DHashTableOperate(&mMappedAttrTable, aMapped, PL_DHASH_ADD));
  if (!entry)
    return nsnull;
  if (!entry->mAttributes) {
    
    entry->mAttributes = aMapped;
  }
  NS_ADDREF(entry->mAttributes); 
  return entry->mAttributes;
}

void
nsHTMLStyleSheet::DropMappedAttributes(nsMappedAttributes* aMapped)
{
  NS_ENSURE_TRUE(aMapped, );

  NS_ASSERTION(mMappedAttrTable.ops, "table uninitialized");
#ifdef DEBUG
  PRUint32 entryCount = mMappedAttrTable.entryCount - 1;
#endif

  PL_DHashTableOperate(&mMappedAttrTable, aMapped, PL_DHASH_REMOVE);

  NS_ASSERTION(entryCount == mMappedAttrTable.entryCount, "not removed");
}

#ifdef DEBUG
 void
nsHTMLStyleSheet::List(FILE* out, PRInt32 aIndent) const
{
  
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("HTML Style Sheet: ", out);
  nsCAutoString urlSpec;
  mURL->GetSpec(urlSpec);
  if (!urlSpec.IsEmpty()) {
    fputs(urlSpec.get(), out);
  }
  fputs("\n", out);
}
#endif


nsresult
NS_NewHTMLStyleSheet(nsHTMLStyleSheet** aInstancePtrResult, nsIURI* aURL, 
                     nsIDocument* aDocument)
{
  nsresult rv;
  nsHTMLStyleSheet* sheet;
  if (NS_FAILED(rv = NS_NewHTMLStyleSheet(&sheet)))
    return rv;

  if (NS_FAILED(rv = sheet->Init(aURL, aDocument))) {
    NS_RELEASE(sheet);
    return rv;
  }

  *aInstancePtrResult = sheet;
  return NS_OK;
}


nsresult
NS_NewHTMLStyleSheet(nsHTMLStyleSheet** aInstancePtrResult)
{
  NS_ASSERTION(aInstancePtrResult, "null out param");

  nsHTMLStyleSheet *it = new nsHTMLStyleSheet();
  if (!it) {
    *aInstancePtrResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(it);
  nsresult rv = it->Init();
  if (NS_FAILED(rv))
    NS_RELEASE(it);

  *aInstancePtrResult = it; 
  return rv;
}
