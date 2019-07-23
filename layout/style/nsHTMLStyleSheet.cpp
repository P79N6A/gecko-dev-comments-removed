


















































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
#include "nsIEventStateManager.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsStyleConsts.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsCSSAnonBoxes.h"
#include "nsRuleWalker.h"
#include "nsRuleData.h"
#include "nsContentErrors.h"
#include "nsRuleProcessorData.h"

NS_IMPL_ISUPPORTS1(nsHTMLStyleSheet::HTMLColorRule, nsIStyleRule)

NS_IMETHODIMP
nsHTMLStyleSheet::HTMLColorRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Color)) {
    if (aRuleData->mColorData->mColor.GetUnit() == eCSSUnit_Null &&
        aRuleData->mPresContext->UseDocumentColors())
      aRuleData->mColorData->mColor.SetColorValue(mColor);
  }
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsHTMLStyleSheet::HTMLColorRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}
#endif

 
NS_IMPL_ISUPPORTS1(nsHTMLStyleSheet::GenericTableRule, nsIStyleRule)

NS_IMETHODIMP
nsHTMLStyleSheet::GenericTableRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsHTMLStyleSheet::GenericTableRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}
#endif

NS_IMETHODIMP
nsHTMLStyleSheet::TableTHRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Text)) {
    if (aRuleData->mTextData->mTextAlign.GetUnit() == eCSSUnit_Null) {
      aRuleData->mTextData->mTextAlign.
        SetIntValue(NS_STYLE_TEXT_ALIGN_MOZ_CENTER_OR_INHERIT,
                    eCSSUnit_Enumerated);
    }
  }
  return NS_OK;
}

static void 
ProcessTableRulesAttribute(void*       aStyleStruct, 
                           nsRuleData* aRuleData,
                           PRUint8     aSide,
                           PRBool      aGroup,
                           PRUint8     aRulesArg1,
                           PRUint8     aRulesArg2,
                           PRUint8     aRulesArg3)
{
  if (!aStyleStruct || !aRuleData || !aRuleData->mPresContext) return;

  nsStyleContext* tableContext = aRuleData->mStyleContext->GetParent();
  if (!tableContext)
    return;
  if (!aGroup) {
    tableContext = tableContext->GetParent();
    if (!tableContext)
      return;
  } 
  
  const nsStyleTable* tableData = tableContext->GetStyleTable();
  if (aRulesArg1 == tableData->mRules ||
      aRulesArg2 == tableData->mRules ||
      aRulesArg3 == tableData->mRules) {
    const nsStyleBorder* tableBorderData = tableContext->GetStyleBorder();
    PRUint8 tableBorderStyle = tableBorderData->GetBorderStyle(aSide);

    nsStyleBorder* borderData = (nsStyleBorder*)aStyleStruct;
    if (!borderData)
      return;
    PRUint8 borderStyle = borderData->GetBorderStyle(aSide);
    
    
    
    
    
    
    
    
    if (NS_STYLE_BORDER_STYLE_NONE == borderStyle) {
      
      PRUint8 bStyle = ((NS_STYLE_BORDER_STYLE_NONE != tableBorderStyle) &&
                        (NS_STYLE_BORDER_STYLE_HIDDEN != tableBorderStyle)) 
                        ? tableBorderStyle : NS_STYLE_BORDER_STYLE_SOLID;
      if ((NS_STYLE_BORDER_STYLE_DASHED != bStyle) && 
          (NS_STYLE_BORDER_STYLE_DOTTED != bStyle) && 
          (NS_STYLE_BORDER_STYLE_SOLID  != bStyle)) {
        bStyle = NS_STYLE_BORDER_STYLE_SOLID;
      }
      bStyle |= NS_STYLE_BORDER_STYLE_RULES_MARKER;
      borderData->SetBorderStyle(aSide, bStyle);

      nscolor borderColor;
      PRBool foreground;
      borderData->GetBorderColor(aSide, borderColor, foreground);
      if (foreground || NS_GET_A(borderColor) == 0) {
        
        nscolor tableBorderColor;
        tableBorderData->GetBorderColor(aSide, tableBorderColor, foreground);
        borderColor = (foreground || NS_GET_A(tableBorderColor) == 0)
                        ? NS_RGB(0,0,0) : tableBorderColor;
        borderData->SetBorderColor(aSide, borderColor);
      }
      
      borderData->SetBorderWidth(aSide, nsPresContext::CSSPixelsToAppUnits(1));
    }
  }
}

static void TbodyPostResolveCallback(void* aStyleStruct, nsRuleData* aRuleData, nsIStyleRule* aStyleRule)
{
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_TOP, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_GROUPS, NS_STYLE_TABLE_RULES_ROWS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_BOTTOM, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_GROUPS, NS_STYLE_TABLE_RULES_ROWS);
}

NS_IMETHODIMP
nsHTMLStyleSheet::TableTbodyRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    nsPostResolveCallback prc = { &TbodyPostResolveCallback, this };
    aRuleData->mPostResolveCallbacks.AppendElement(prc);
  }
  return NS_OK;
}


static void RowPostResolveCallback(void* aStyleStruct, nsRuleData* aRuleData, nsIStyleRule* aStyleRule)
{
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_TOP, PR_FALSE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_ROWS, NS_STYLE_TABLE_RULES_ROWS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_BOTTOM, PR_FALSE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_ROWS, NS_STYLE_TABLE_RULES_ROWS);
}

NS_IMETHODIMP
nsHTMLStyleSheet::TableRowRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    nsPostResolveCallback prc = { &RowPostResolveCallback, this };
    aRuleData->mPostResolveCallbacks.AppendElement(prc);
  }
  return NS_OK;
}

static void ColgroupPostResolveCallback(void* aStyleStruct, nsRuleData* aRuleData, nsIStyleRule* aStyleRule)
{
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_LEFT, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_GROUPS, NS_STYLE_TABLE_RULES_COLS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_RIGHT, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_GROUPS, NS_STYLE_TABLE_RULES_COLS);
}

NS_IMETHODIMP
nsHTMLStyleSheet::TableColgroupRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    nsPostResolveCallback prc = { &ColgroupPostResolveCallback, this };
    aRuleData->mPostResolveCallbacks.AppendElement(prc);
  }
  return NS_OK;
}

static void ColPostResolveCallback(void* aStyleStruct, nsRuleData* aRuleData, nsIStyleRule* aStyleRule)
{
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_LEFT, PR_FALSE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_COLS, NS_STYLE_TABLE_RULES_COLS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_RIGHT, PR_FALSE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_COLS, NS_STYLE_TABLE_RULES_COLS);
}

static void UngroupedColPostResolveCallback(void* aStyleStruct,
                                            nsRuleData* aRuleData, nsIStyleRule* aStyleRule)
{
  
  
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_LEFT, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_COLS, NS_STYLE_TABLE_RULES_COLS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_RIGHT, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_COLS, NS_STYLE_TABLE_RULES_COLS);
}

NS_IMETHODIMP
nsHTMLStyleSheet::TableColRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    nsPostResolveCallback prc = { &ColPostResolveCallback, this };
    aRuleData->mPostResolveCallbacks.AppendElement(prc);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::TableUngroupedColRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    nsPostResolveCallback prc = { &UngroupedColPostResolveCallback, this };
    aRuleData->mPostResolveCallbacks.AppendElement(prc);
  }
  return NS_OK;
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
  : mURL(nsnull),
    mDocument(nsnull),
    mLinkRule(nsnull),
    mVisitedRule(nsnull),
    mActiveRule(nsnull),
    mDocumentColorRule(nsnull)
{
  mMappedAttrTable.ops = nsnull;
}

nsresult
nsHTMLStyleSheet::Init()
{
  mTableTbodyRule = new TableTbodyRule();
  if (!mTableTbodyRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableTbodyRule);

  mTableRowRule = new TableRowRule();
  if (!mTableRowRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableRowRule);

  mTableColgroupRule = new TableColgroupRule();
  if (!mTableColgroupRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableColgroupRule);

  mTableColRule = new TableColRule();
  if (!mTableColRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableColRule);

  mTableUngroupedColRule = new TableUngroupedColRule();
  if (!mTableUngroupedColRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableUngroupedColRule);

  mTableTHRule = new TableTHRule();
  if (!mTableTHRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableTHRule);
  return NS_OK;
}

nsHTMLStyleSheet::~nsHTMLStyleSheet()
{
  NS_IF_RELEASE(mURL);

  NS_IF_RELEASE(mLinkRule);
  NS_IF_RELEASE(mVisitedRule);
  NS_IF_RELEASE(mActiveRule);
  NS_IF_RELEASE(mDocumentColorRule);
  NS_IF_RELEASE(mTableTbodyRule);
  NS_IF_RELEASE(mTableRowRule);
  NS_IF_RELEASE(mTableColgroupRule);
  NS_IF_RELEASE(mTableColRule);
  NS_IF_RELEASE(mTableUngroupedColRule);
  NS_IF_RELEASE(mTableTHRule);

  if (mMappedAttrTable.ops)
    PL_DHashTableFinish(&mMappedAttrTable);
}

NS_IMPL_ISUPPORTS2(nsHTMLStyleSheet, nsIStyleSheet, nsIStyleRuleProcessor)

static nsresult GetBodyColor(nsPresContext* aPresContext, nscolor* aColor)
{
  nsIPresShell *shell = aPresContext->PresShell();
  nsCOMPtr<nsIDOMHTMLDocument> domdoc = do_QueryInterface(shell->GetDocument());
  if (!domdoc)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMHTMLElement> body;
  domdoc->GetBody(getter_AddRefs(body));
  nsCOMPtr<nsIContent> bodyContent = do_QueryInterface(body);
  nsIFrame *bodyFrame = shell->GetPrimaryFrameFor(bodyContent);
  if (!bodyFrame)
    return NS_ERROR_FAILURE;
  *aColor = bodyFrame->GetStyleColor()->mColor;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::RulesMatching(ElementRuleProcessorData* aData)
{
  nsIContent *content = aData->mContent;

  if (content) {
    nsRuleWalker *ruleWalker = aData->mRuleWalker;
    if (aData->mIsHTMLContent) {
      nsIAtom* tag = aData->mContentTag;

      
      if (tag == nsGkAtoms::a) {
        if (mLinkRule || mVisitedRule || mActiveRule) {
          if (aData->mIsLink) {
            switch (aData->mLinkState) {
              case eLinkState_Unvisited:
                if (mLinkRule)
                  ruleWalker->Forward(mLinkRule);
                break;
              case eLinkState_Visited:
                if (mVisitedRule)
                  ruleWalker->Forward(mVisitedRule);
                break;
              default:
                break;
            }

            
            if (mActiveRule && (aData->mEventState & NS_EVENT_STATE_ACTIVE))
              ruleWalker->Forward(mActiveRule);
          }
        } 
      } 
      
      else if (tag == nsGkAtoms::th) {
        ruleWalker->Forward(mTableTHRule);
      }
      else if (tag == nsGkAtoms::tr) {
        ruleWalker->Forward(mTableRowRule);
      }
      else if ((tag == nsGkAtoms::thead) || (tag == nsGkAtoms::tbody) || (tag == nsGkAtoms::tfoot)) {
        ruleWalker->Forward(mTableTbodyRule);
      }
      else if (tag == nsGkAtoms::col) {
        nsIContent* parent = aData->mParentContent;
        if (parent && parent->IsNodeOfType(nsIContent::eHTML) &&
            parent->Tag() == nsGkAtoms::colgroup) {
          ruleWalker->Forward(mTableColRule);
        } else {
          ruleWalker->Forward(mTableUngroupedColRule);
        }
      }
      else if (tag == nsGkAtoms::colgroup) {
        ruleWalker->Forward(mTableColgroupRule);
      }
      else if (tag == nsGkAtoms::table) {
        if (aData->mCompatMode == eCompatibility_NavQuirks) {
          nscolor bodyColor;
          nsresult rv =
            GetBodyColor(ruleWalker->GetCurrentNode()->GetPresContext(),
                         &bodyColor);
          if (NS_SUCCEEDED(rv) &&
              (!mDocumentColorRule || bodyColor != mDocumentColorRule->mColor)) {
            NS_IF_RELEASE(mDocumentColorRule);
            mDocumentColorRule = new HTMLColorRule();
            if (mDocumentColorRule) {
              NS_ADDREF(mDocumentColorRule);
              mDocumentColorRule->mColor = bodyColor;
            }
          }
          if (mDocumentColorRule)
            ruleWalker->Forward(mDocumentColorRule);
        }
      }
    } 

    
    content->WalkContentStyleRules(ruleWalker);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLStyleSheet::HasStateDependentStyle(StateRuleProcessorData* aData,
                                         nsReStyleHint* aResult)
{
  if (aData->mContent &&
      aData->mIsHTMLContent &&
      aData->mIsLink &&
      aData->mContentTag == nsGkAtoms::a &&
      ((mActiveRule && (aData->mStateMask & NS_EVENT_STATE_ACTIVE)) ||
       (mLinkRule && (aData->mStateMask & NS_EVENT_STATE_VISITED)) ||
       (mVisitedRule && (aData->mStateMask & NS_EVENT_STATE_VISITED)))) {
    *aResult = eReStyle_Self;
  }
  else
    *aResult = nsReStyleHint(0);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::HasAttributeDependentStyle(AttributeRuleProcessorData* aData,
                                             nsReStyleHint* aResult)
{
  
  
  

  
  nsIContent *content = aData->mContent;
  if (aData->mAttribute == nsGkAtoms::href &&
      (mLinkRule || mVisitedRule || mActiveRule) &&
      content &&
      content->IsNodeOfType(nsINode::eHTML) &&
      aData->mContentTag == nsGkAtoms::a) {
    *aResult = eReStyle_Self;
    return NS_OK;
  }

  
  

  
  if (content && content->IsAttributeMapped(aData->mAttribute)) {
    *aResult = eReStyle_Self;
    return NS_OK;
  }

  *aResult = nsReStyleHint(0);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::MediumFeaturesChanged(nsPresContext* aPresContext,
                                        PRBool* aRulesChanged)
{
  *aRulesChanged = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLStyleSheet::RulesMatching(PseudoRuleProcessorData* aData)
{
  nsIAtom* pseudoTag = aData->mPseudoTag;
  if (pseudoTag == nsCSSAnonBoxes::tableCol) {
    nsRuleWalker *ruleWalker = aData->mRuleWalker;
    if (ruleWalker) {
      ruleWalker->Forward(mTableColRule);
    }
  }
  return NS_OK;
}


  
NS_IMETHODIMP
nsHTMLStyleSheet::GetSheetURI(nsIURI** aSheetURI) const
{
  *aSheetURI = mURL;
  NS_IF_ADDREF(*aSheetURI);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::GetBaseURI(nsIURI** aBaseURI) const
{
  *aBaseURI = mURL;
  NS_IF_ADDREF(*aBaseURI);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::GetTitle(nsString& aTitle) const
{
  aTitle.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::GetType(nsString& aType) const
{
  aType.AssignLiteral("text/html");
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsHTMLStyleSheet::HasRules() const
{
  return PR_TRUE; 
}

NS_IMETHODIMP
nsHTMLStyleSheet::GetApplicable(PRBool& aApplicable) const
{
  aApplicable = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::SetEnabled(PRBool aEnabled)
{ 
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::GetComplete(PRBool& aComplete) const
{
  aComplete = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::SetComplete()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::GetParentSheet(nsIStyleSheet*& aParent) const
{
  aParent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::GetOwningDocument(nsIDocument*& aDocument) const
{
  aDocument = mDocument;
  NS_IF_ADDREF(aDocument);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLStyleSheet::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument; 
  return NS_OK;
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
  NS_ADDREF(mURL);
  return NS_OK;
}

nsresult
nsHTMLStyleSheet::Reset(nsIURI* aURL)
{
  NS_IF_RELEASE(mURL);
  mURL = aURL;
  NS_ADDREF(mURL);

  NS_IF_RELEASE(mLinkRule);
  NS_IF_RELEASE(mVisitedRule);
  NS_IF_RELEASE(mActiveRule);
  NS_IF_RELEASE(mDocumentColorRule);

  if (mMappedAttrTable.ops) {
    PL_DHashTableFinish(&mMappedAttrTable);
    mMappedAttrTable.ops = nsnull;
  }

  return NS_OK;
}

nsresult
nsHTMLStyleSheet::GetLinkColor(nscolor& aColor)
{
  if (!mLinkRule) {
    return NS_HTML_STYLE_PROPERTY_NOT_THERE;
  }
  else {
    aColor = mLinkRule->mColor;
    return NS_OK;
  }
}

nsresult
nsHTMLStyleSheet::GetActiveLinkColor(nscolor& aColor)
{
  if (!mActiveRule) {
    return NS_HTML_STYLE_PROPERTY_NOT_THERE;
  }
  else {
    aColor = mActiveRule->mColor;
    return NS_OK;
  }
}

nsresult
nsHTMLStyleSheet::GetVisitedLinkColor(nscolor& aColor)
{
  if (!mVisitedRule) {
    return NS_HTML_STYLE_PROPERTY_NOT_THERE;
  }
  else {
    aColor = mVisitedRule->mColor;
    return NS_OK;
  }
}

nsresult
nsHTMLStyleSheet::SetLinkColor(nscolor aColor)
{
  if (mLinkRule) {
    if (mLinkRule->mColor == aColor)
      return NS_OK;
    NS_RELEASE(mLinkRule);
  }

  mLinkRule = new HTMLColorRule();
  if (!mLinkRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mLinkRule);

  mLinkRule->mColor = aColor;
  return NS_OK;
}


nsresult
nsHTMLStyleSheet::SetActiveLinkColor(nscolor aColor)
{
  if (mActiveRule) {
    if (mActiveRule->mColor == aColor)
      return NS_OK;
    NS_RELEASE(mActiveRule);
  }

  mActiveRule = new HTMLColorRule();
  if (!mActiveRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mActiveRule);

  mActiveRule->mColor = aColor;
  return NS_OK;
}

nsresult
nsHTMLStyleSheet::SetVisitedLinkColor(nscolor aColor)
{
  if (mVisitedRule) {
    if (mVisitedRule->mColor == aColor)
      return NS_OK;
    NS_RELEASE(mVisitedRule);
  }

  mVisitedRule = new HTMLColorRule();
  if (!mVisitedRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mVisitedRule);

  mVisitedRule->mColor = aColor;
  return NS_OK;
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
void nsHTMLStyleSheet::List(FILE* out, PRInt32 aIndent) const
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
