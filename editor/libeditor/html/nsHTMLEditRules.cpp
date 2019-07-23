








































#if defined(XP_MAC) && defined(MOZ_MAC_LOWMEM)
#pragma optimization_level 1
#endif

#include "nsHTMLEditRules.h"

#include "nsEditor.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsHTMLCSSUtils.h"
#include "nsHTMLEditor.h"

#include "nsIServiceManager.h"
#include "nsCRT.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMNode.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionController.h"
#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsIRangeUtils.h"
#include "nsIDOMCharacterData.h"
#include "nsIEnumerator.h"
#include "nsIPresShell.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIRange.h"

#include "nsEditorUtils.h"
#include "nsWSRunObject.h"

#include "InsertTextTxn.h"
#include "DeleteTextTxn.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"

#include "nsFrameSelection.h"




enum
{
  kLonely = 0,
  kPrevSib = 1,
  kNextSib = 2,
  kBothSibs = 3
};





static PRBool IsBlockNode(nsIDOMNode* node)
{
  PRBool isBlock (PR_FALSE);
  nsHTMLEditor::NodeIsBlockStatic(node, &isBlock);
  return isBlock;
}

static PRBool IsInlineNode(nsIDOMNode* node)
{
  return !IsBlockNode(node);
}
 
class nsTableCellAndListItemFunctor : public nsBoolDomIterFunctor
{
  public:
    virtual PRBool operator()(nsIDOMNode* aNode)  
    {
      if (nsHTMLEditUtils::IsTableCell(aNode)) return PR_TRUE;
      if (nsHTMLEditUtils::IsListItem(aNode)) return PR_TRUE;
      return PR_FALSE;
    }
};

class nsBRNodeFunctor : public nsBoolDomIterFunctor
{
  public:
    virtual PRBool operator()(nsIDOMNode* aNode)  
    {
      if (nsTextEditUtils::IsBreak(aNode)) return PR_TRUE;
      return PR_FALSE;
    }
};

class nsEmptyFunctor : public nsBoolDomIterFunctor
{
  public:
    nsEmptyFunctor(nsHTMLEditor* editor) : mHTMLEditor(editor) {}
    virtual PRBool operator()(nsIDOMNode* aNode)  
    {
      if (nsHTMLEditUtils::IsListItem(aNode) || nsHTMLEditUtils::IsTableCellOrCaption(aNode))
      {
        PRBool bIsEmptyNode;
        nsresult res = mHTMLEditor->IsEmptyNode(aNode, &bIsEmptyNode, PR_FALSE, PR_FALSE);
        if (NS_FAILED(res)) return PR_FALSE;
        if (bIsEmptyNode) 
          return PR_TRUE;
      }
      return PR_FALSE;
    }
  protected:
    nsHTMLEditor* mHTMLEditor;
};

class nsEditableTextFunctor : public nsBoolDomIterFunctor
{
  public:
    nsEditableTextFunctor(nsHTMLEditor* editor) : mHTMLEditor(editor) {}
    virtual PRBool operator()(nsIDOMNode* aNode)  
    {
      if (nsEditor::IsTextNode(aNode) && mHTMLEditor->IsEditable(aNode)) 
      {
        return PR_TRUE;
      }
      return PR_FALSE;
    }
  protected:
    nsHTMLEditor* mHTMLEditor;
};






nsresult
NS_NewHTMLEditRules(nsIEditRules** aInstancePtrResult)
{
  nsHTMLEditRules * rules = new nsHTMLEditRules();
  if (rules)
    return rules->QueryInterface(NS_GET_IID(nsIEditRules), (void**) aInstancePtrResult);
  return NS_ERROR_OUT_OF_MEMORY;
}





nsHTMLEditRules::nsHTMLEditRules() : 
mDocChangeRange(nsnull)
,mListenerEnabled(PR_TRUE)
,mReturnInEmptyLIKillsList(PR_TRUE)
,mDidDeleteSelection(PR_FALSE)
,mDidRangedDelete(PR_FALSE)
,mUtilRange(nsnull)
,mJoinOffset(0)
{
  nsString emptyString;
  
  mCachedStyles[0] = StyleCache(nsEditProperty::b, emptyString, emptyString);
  mCachedStyles[1] = StyleCache(nsEditProperty::i, emptyString, emptyString);
  mCachedStyles[2] = StyleCache(nsEditProperty::u, emptyString, emptyString);
  mCachedStyles[3] = StyleCache(nsEditProperty::font, NS_LITERAL_STRING("face"), emptyString);
  mCachedStyles[4] = StyleCache(nsEditProperty::font, NS_LITERAL_STRING("size"), emptyString);
  mCachedStyles[5] = StyleCache(nsEditProperty::font, NS_LITERAL_STRING("color"), emptyString);
  mCachedStyles[6] = StyleCache(nsEditProperty::tt, emptyString, emptyString);
  mCachedStyles[7] = StyleCache(nsEditProperty::em, emptyString, emptyString);
  mCachedStyles[8] = StyleCache(nsEditProperty::strong, emptyString, emptyString);
  mCachedStyles[9] = StyleCache(nsEditProperty::dfn, emptyString, emptyString);
  mCachedStyles[10] = StyleCache(nsEditProperty::code, emptyString, emptyString);
  mCachedStyles[11] = StyleCache(nsEditProperty::samp, emptyString, emptyString);
  mCachedStyles[12] = StyleCache(nsEditProperty::var, emptyString, emptyString);
  mCachedStyles[13] = StyleCache(nsEditProperty::cite, emptyString, emptyString);
  mCachedStyles[14] = StyleCache(nsEditProperty::abbr, emptyString, emptyString);
  mCachedStyles[15] = StyleCache(nsEditProperty::acronym, emptyString, emptyString);
  mCachedStyles[16] = StyleCache(nsEditProperty::cssBackgroundColor, emptyString, emptyString);
  mCachedStyles[17] = StyleCache(nsEditProperty::sub, emptyString, emptyString);
  mCachedStyles[18] = StyleCache(nsEditProperty::sup, emptyString, emptyString);
}

nsHTMLEditRules::~nsHTMLEditRules()
{
  
  
  
  
  
  mHTMLEditor->RemoveEditActionListener(this);
}





NS_IMPL_ADDREF_INHERITED(nsHTMLEditRules, nsTextEditRules)
NS_IMPL_RELEASE_INHERITED(nsHTMLEditRules, nsTextEditRules)
NS_IMPL_QUERY_INTERFACE3(nsHTMLEditRules, nsIHTMLEditRules, nsIEditRules, nsIEditActionListener)






NS_IMETHODIMP
nsHTMLEditRules::Init(nsPlaintextEditor *aEditor, PRUint32 aFlags)
{
  mHTMLEditor = NS_STATIC_CAST(nsHTMLEditor*, aEditor);
  nsresult res;
  
  
  res = nsTextEditRules::Init(aEditor, aFlags);
  if (NS_FAILED(res)) return res;

  
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &res);
  if (NS_FAILED(res)) return res;

  char *returnInEmptyLIKillsList = 0;
  res = prefBranch->GetCharPref("editor.html.typing.returnInEmptyListItemClosesList",
                                &returnInEmptyLIKillsList);

  if (NS_SUCCEEDED(res) && returnInEmptyLIKillsList)
  {
    if (!strncmp(returnInEmptyLIKillsList, "false", 5))
      mReturnInEmptyLIKillsList = PR_FALSE; 
    else
      mReturnInEmptyLIKillsList = PR_TRUE; 
  }
  else
  {
    mReturnInEmptyLIKillsList = PR_TRUE; 
  }
  
  
  mUtilRange = do_CreateInstance("@mozilla.org/content/range;1");
  if (!mUtilRange) return NS_ERROR_NULL_POINTER;
   
  
  nsIDOMElement *rootElem = mHTMLEditor->GetRoot();
  if (rootElem)
  {
    
    nsAutoLockRulesSniffing lockIt((nsTextEditRules*)this);
    if (!mDocChangeRange)
    {
      mDocChangeRange = do_CreateInstance("@mozilla.org/content/range;1");
      if (!mDocChangeRange) return NS_ERROR_NULL_POINTER;
    }
    mDocChangeRange->SelectNode(rootElem);
    res = AdjustSpecialBreaks();
    if (NS_FAILED(res)) return res;
  }

  
  res = mHTMLEditor->AddEditActionListener(this);

  return res;
}


NS_IMETHODIMP
nsHTMLEditRules::BeforeEdit(PRInt32 action, nsIEditor::EDirection aDirection)
{
  if (mLockRulesSniffing) return NS_OK;
  
  nsAutoLockRulesSniffing lockIt((nsTextEditRules*)this);
  mDidExplicitlySetInterline = PR_FALSE;

  if (!mActionNesting)
  {
    
    mDidRangedDelete = PR_FALSE;
    
    
    
    
    nsCOMPtr<nsISelection>selection;
    nsresult res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(res)) return res;
  
    
    nsCOMPtr<nsIDOMNode> selStartNode, selEndNode;
    PRInt32 selOffset;
    res = mHTMLEditor->GetStartNodeAndOffset(selection, address_of(selStartNode), &selOffset);
    if (NS_FAILED(res)) return res;
    mRangeItem.startNode = selStartNode;
    mRangeItem.startOffset = selOffset;

    
    res = mHTMLEditor->GetEndNodeAndOffset(selection, address_of(selEndNode), &selOffset);
    if (NS_FAILED(res)) return res;
    mRangeItem.endNode = selEndNode;
    mRangeItem.endOffset = selOffset;

    
    (mHTMLEditor->mRangeUpdater).RegisterRangeItem(&mRangeItem);

    
    mDidDeleteSelection = PR_FALSE;
    
    
    if(mDocChangeRange)
    {
      
      nsCOMPtr<nsIRange> range = do_QueryInterface(mDocChangeRange);
      range->Reset(); 
    }
    if(mUtilRange)
    {
      
      nsCOMPtr<nsIRange> range = do_QueryInterface(mUtilRange);
      range->Reset(); 
    }

    
    if ((action == nsEditor::kOpInsertText)      || 
        (action == nsEditor::kOpInsertIMEText)   ||
        (action == nsEditor::kOpDeleteSelection) ||
        (action == nsEditor::kOpInsertBreak))
    {
      nsCOMPtr<nsIDOMNode> selNode = selStartNode;
      if (aDirection == nsIEditor::eNext)
        selNode = selEndNode;
      res = CacheInlineStyles(selNode);
      if (NS_FAILED(res)) return res;
    }
    
    
    ConfirmSelectionInBody();
    
    mTheAction = action;
  }
  mActionNesting++;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditRules::AfterEdit(PRInt32 action, nsIEditor::EDirection aDirection)
{
  if (mLockRulesSniffing) return NS_OK;

  nsAutoLockRulesSniffing lockIt(this);

  NS_PRECONDITION(mActionNesting>0, "bad action nesting!");
  nsresult res = NS_OK;
  if (!--mActionNesting)
  {
    
    res = AfterEditInner(action, aDirection);

    
    (mHTMLEditor->mRangeUpdater).DropRangeItem(&mRangeItem);

    





    if (action == nsEditor::kOpInsertText
        || action == nsEditor::kOpInsertIMEText) {

      nsCOMPtr<nsISelection> selection;
      nsresult res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
      if (NS_FAILED(res)) return res;
      nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(selection));
      nsCOMPtr<nsFrameSelection> frameSelection;
      privateSelection->GetFrameSelection(getter_AddRefs(frameSelection));
      if (frameSelection) {
        frameSelection->UndefineCaretBidiLevel();
      }
    }
  }

  return res;
}


nsresult
nsHTMLEditRules::AfterEditInner(PRInt32 action, nsIEditor::EDirection aDirection)
{
  ConfirmSelectionInBody();
  if (action == nsEditor::kOpIgnore) return NS_OK;
  
  nsCOMPtr<nsISelection>selection;
  nsresult res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  
  nsCOMPtr<nsIDOMNode> rangeStartParent, rangeEndParent;
  PRInt32 rangeStartOffset = 0, rangeEndOffset = 0;
  
  PRBool bDamagedRange = PR_FALSE;  
  if (mDocChangeRange)
  {  
    mDocChangeRange->GetStartContainer(getter_AddRefs(rangeStartParent));
    mDocChangeRange->GetEndContainer(getter_AddRefs(rangeEndParent));
    mDocChangeRange->GetStartOffset(&rangeStartOffset);
    mDocChangeRange->GetEndOffset(&rangeEndOffset);
    if (rangeStartParent && rangeEndParent) 
      bDamagedRange = PR_TRUE; 
  }
  
  if (bDamagedRange && !((action == nsEditor::kOpUndo) || (action == nsEditor::kOpRedo)))
  {
    
    
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
   
    
    res = PromoteRange(mDocChangeRange, action);
    if (NS_FAILED(res)) return res;

    
    
    
    
    if ((action == nsEditor::kOpDeleteSelection) && mDidRangedDelete)
    {
      res = InsertBRIfNeeded(selection);
      if (NS_FAILED(res)) return res;
    }  
    
    
    res = AdjustSpecialBreaks();
    if (NS_FAILED(res)) return res;
    
    
    if ( (action != nsEditor::kOpInsertText &&
         action != nsEditor::kOpInsertIMEText) )
    {
      res = mHTMLEditor->CollapseAdjacentTextNodes(mDocChangeRange);
      if (NS_FAILED(res)) return res;
    }
    
    
    
    
    if (
        
        (action == nsHTMLEditor::kOpInsertElement) ||
        (action == nsHTMLEditor::kOpInsertQuotation) ||
        (action == nsEditor::kOpInsertNode) ||
        (action == nsHTMLEditor::kOpHTMLPaste ||
        (action == nsHTMLEditor::kOpLoadHTML)))
    {
      res = ReplaceNewlines(mDocChangeRange);
      if (NS_FAILED(res)) return res;
    }
    
    
    res = RemoveEmptyNodes();
    if (NS_FAILED(res)) return res;

    
    if ((action == nsEditor::kOpInsertText) || 
        (action == nsEditor::kOpInsertIMEText) ||
        (action == nsEditor::kOpDeleteSelection) ||
        (action == nsEditor::kOpInsertBreak) || 
        (action == nsHTMLEditor::kOpHTMLPaste ||
        (action == nsHTMLEditor::kOpLoadHTML)))
    {
      res = AdjustWhitespace(selection);
      if (NS_FAILED(res)) return res;
      
      
      nsWSRunObject(mHTMLEditor, mRangeItem.startNode, mRangeItem.startOffset).AdjustWhitespace();
      
      if ((mRangeItem.startNode != mRangeItem.endNode) || (mRangeItem.startOffset != mRangeItem.endOffset))
      {
        nsWSRunObject(mHTMLEditor, mRangeItem.endNode, mRangeItem.endOffset).AdjustWhitespace();
      }
    }
    
    
    if (mNewBlock)
    {
      res = PinSelectionToNewBlock(selection);
      mNewBlock = 0;
    }

    
    if ((action == nsEditor::kOpInsertText) || 
        (action == nsEditor::kOpInsertIMEText) ||
        (action == nsEditor::kOpDeleteSelection) ||
        (action == nsEditor::kOpInsertBreak) || 
        (action == nsHTMLEditor::kOpHTMLPaste ||
        (action == nsHTMLEditor::kOpLoadHTML)))
    {
      res = AdjustSelection(selection, aDirection);
      if (NS_FAILED(res)) return res;
    }

    
    if ((action == nsEditor::kOpInsertText)      || 
        (action == nsEditor::kOpInsertIMEText)   ||
        (action == nsEditor::kOpDeleteSelection) ||
        (action == nsEditor::kOpInsertBreak))
    {
      mHTMLEditor->mTypeInState->UpdateSelState(selection);
      res = ReapplyCachedStyles();
      if (NS_FAILED(res)) return res;
      res = ClearCachedStyles();
      if (NS_FAILED(res)) return res;
    }    
  }

  res = mHTMLEditor->HandleInlineSpellCheck(action, selection, 
                                            mRangeItem.startNode, mRangeItem.startOffset,
                                            rangeStartParent, rangeStartOffset,
                                            rangeEndParent, rangeEndOffset);
  if (NS_FAILED(res)) 
    return res;

  
  res = CreateBogusNodeIfNeeded(selection);
  
  
  if (NS_FAILED(res)) 
    return res;
  
  if (!mDidExplicitlySetInterline)
  {
    res = CheckInterlinePosition(selection);
  }
  
  return res;
}


NS_IMETHODIMP 
nsHTMLEditRules::WillDoAction(nsISelection *aSelection, 
                              nsRulesInfo *aInfo, 
                              PRBool *aCancel, 
                              PRBool *aHandled)
{
  if (!aInfo || !aCancel || !aHandled) 
    return NS_ERROR_NULL_POINTER;
#if defined(DEBUG_ftang)
  printf("nsHTMLEditRules::WillDoAction action = %d\n", aInfo->action);
#endif

  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;
    
  
  nsTextRulesInfo *info = NS_STATIC_CAST(nsTextRulesInfo*, aInfo);
    
  switch (info->action)
  {
    case kInsertText:
    case kInsertTextIME:
      return WillInsertText(info->action,
                            aSelection, 
                            aCancel, 
                            aHandled,
                            info->inString,
                            info->outString,
                            info->maxLength);
    case kLoadHTML:
      return WillLoadHTML(aSelection, aCancel);
    case kInsertBreak:
      return WillInsertBreak(aSelection, aCancel, aHandled);
    case kDeleteSelection:
      return WillDeleteSelection(aSelection, info->collapsedAction, aCancel, aHandled);
    case kMakeList:
      return WillMakeList(aSelection, info->blockType, info->entireList, info->bulletType, aCancel, aHandled);
    case kIndent:
      return WillIndent(aSelection, aCancel, aHandled);
    case kOutdent:
      return WillOutdent(aSelection, aCancel, aHandled);
    case kSetAbsolutePosition:
      return WillAbsolutePosition(aSelection, aCancel, aHandled);
    case kRemoveAbsolutePosition:
      return WillRemoveAbsolutePosition(aSelection, aCancel, aHandled);
    case kAlign:
      return WillAlign(aSelection, info->alignType, aCancel, aHandled);
    case kMakeBasicBlock:
      return WillMakeBasicBlock(aSelection, info->blockType, aCancel, aHandled);
    case kRemoveList:
      return WillRemoveList(aSelection, info->bOrdered, aCancel, aHandled);
    case kMakeDefListItem:
      return WillMakeDefListItem(aSelection, info->blockType, info->entireList, aCancel, aHandled);
    case kInsertElement:
      return WillInsert(aSelection, aCancel);
    case kDecreaseZIndex:
      return WillRelativeChangeZIndex(aSelection, -1, aCancel, aHandled);
    case kIncreaseZIndex:
      return WillRelativeChangeZIndex(aSelection, 1, aCancel, aHandled);
  }
  return nsTextEditRules::WillDoAction(aSelection, aInfo, aCancel, aHandled);
}
  
  
NS_IMETHODIMP 
nsHTMLEditRules::DidDoAction(nsISelection *aSelection,
                             nsRulesInfo *aInfo, nsresult aResult)
{
  nsTextRulesInfo *info = NS_STATIC_CAST(nsTextRulesInfo*, aInfo);
  switch (info->action)
  {
    case kInsertBreak:
      return DidInsertBreak(aSelection, aResult);
    case kDeleteSelection:
      return DidDeleteSelection(aSelection, info->collapsedAction, aResult);
    case kMakeBasicBlock:
    case kIndent:
    case kOutdent:
    case kAlign:
      return DidMakeBasicBlock(aSelection, aInfo, aResult);
    case kSetAbsolutePosition: {
      nsresult rv = DidMakeBasicBlock(aSelection, aInfo, aResult);
      if (NS_FAILED(rv)) return rv;
      return DidAbsolutePosition();
      }
  }
  
  
  return nsTextEditRules::DidDoAction(aSelection, aInfo, aResult);
}
  




NS_IMETHODIMP 
nsHTMLEditRules::GetListState(PRBool *aMixed, PRBool *aOL, PRBool *aUL, PRBool *aDL)
{
  if (!aMixed || !aOL || !aUL || !aDL)
    return NS_ERROR_NULL_POINTER;
  *aMixed = PR_FALSE;
  *aOL = PR_FALSE;
  *aUL = PR_FALSE;
  *aDL = PR_FALSE;
  PRBool bNonList = PR_FALSE;
  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  nsresult res = GetListActionNodes(arrayOfNodes, PR_FALSE, PR_TRUE);
  if (NS_FAILED(res)) return res;

  
  PRInt32 listCount = arrayOfNodes.Count();
  PRInt32 i;
  for (i=listCount-1; i>=0; i--)
  {
    nsIDOMNode* curNode = arrayOfNodes[i];
    
    if (nsHTMLEditUtils::IsUnorderedList(curNode))
      *aUL = PR_TRUE;
    else if (nsHTMLEditUtils::IsOrderedList(curNode))
      *aOL = PR_TRUE;
    else if (nsEditor::NodeIsType(curNode, nsEditProperty::li))
    {
      nsCOMPtr<nsIDOMNode> parent;
      PRInt32 offset;
      res = nsEditor::GetNodeLocation(curNode, address_of(parent), &offset);
      if (NS_FAILED(res)) return res;
      if (nsHTMLEditUtils::IsUnorderedList(parent))
        *aUL = PR_TRUE;
      else if (nsHTMLEditUtils::IsOrderedList(parent))
        *aOL = PR_TRUE;
    }
    else if (nsEditor::NodeIsType(curNode, nsEditProperty::dl) ||
             nsEditor::NodeIsType(curNode, nsEditProperty::dt) ||
             nsEditor::NodeIsType(curNode, nsEditProperty::dd) )
    {
      *aDL = PR_TRUE;
    }
    else bNonList = PR_TRUE;
  }  
  
  
  if ( (*aUL + *aOL + *aDL + bNonList) > 1) *aMixed = PR_TRUE;
  
  return res;
}

NS_IMETHODIMP 
nsHTMLEditRules::GetListItemState(PRBool *aMixed, PRBool *aLI, PRBool *aDT, PRBool *aDD)
{
  if (!aMixed || !aLI || !aDT || !aDD)
    return NS_ERROR_NULL_POINTER;
  *aMixed = PR_FALSE;
  *aLI = PR_FALSE;
  *aDT = PR_FALSE;
  *aDD = PR_FALSE;
  PRBool bNonList = PR_FALSE;
  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  nsresult res = GetListActionNodes(arrayOfNodes, PR_FALSE, PR_TRUE);
  if (NS_FAILED(res)) return res;

  
  PRInt32 listCount = arrayOfNodes.Count();
  PRInt32 i;
  for (i = listCount-1; i>=0; i--)
  {
    nsIDOMNode* curNode = arrayOfNodes[i];
    
    if (nsHTMLEditUtils::IsUnorderedList(curNode) ||
        nsHTMLEditUtils::IsOrderedList(curNode) ||
        nsEditor::NodeIsType(curNode, nsEditProperty::li) )
    {
      *aLI = PR_TRUE;
    }
    else if (nsEditor::NodeIsType(curNode, nsEditProperty::dt))
    {
      *aDT = PR_TRUE;
    }
    else if (nsEditor::NodeIsType(curNode, nsEditProperty::dd))
    {
      *aDD = PR_TRUE;
    }
    else if (nsEditor::NodeIsType(curNode, nsEditProperty::dl))
    {
      
      PRBool bDT, bDD;
      res = GetDefinitionListItemTypes(curNode, bDT, bDD);
      if (NS_FAILED(res)) return res;
      *aDT |= bDT;
      *aDD |= bDD;
    }
    else bNonList = PR_TRUE;
  }  
  
  
  if ( (*aDT + *aDD + bNonList) > 1) *aMixed = PR_TRUE;
  
  return res;
}

NS_IMETHODIMP 
nsHTMLEditRules::GetAlignment(PRBool *aMixed, nsIHTMLEditor::EAlignment *aAlign)
{
  
  
  
  
  

  

  
  if (!aMixed || !aAlign)
    return NS_ERROR_NULL_POINTER;
  *aMixed = PR_FALSE;
  *aAlign = nsIHTMLEditor::eLeft;

  
  nsCOMPtr<nsISelection>selection;
  nsresult res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;

  
  nsCOMPtr<nsIDOMNode> parent;
  nsIDOMElement *rootElem = mHTMLEditor->GetRoot();
  if (!rootElem)
    return NS_ERROR_FAILURE;

  PRInt32 offset, rootOffset;
  res = nsEditor::GetNodeLocation(rootElem, address_of(parent), &rootOffset);
  if (NS_FAILED(res)) return res;
  res = mHTMLEditor->GetStartNodeAndOffset(selection, address_of(parent), &offset);
  if (NS_FAILED(res)) return res;

  
  PRBool bCollapsed;
  res = selection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  nsCOMPtr<nsIDOMNode> nodeToExamine;
  nsCOMPtr<nsISupports> isupports;
  if (bCollapsed)
  {
    
    
    nodeToExamine = parent;
  }
  else if (mHTMLEditor->IsTextNode(parent)) 
  {
    
    nodeToExamine = parent;
  }
  else if (nsEditor::NodeIsType(parent, nsEditProperty::html) &&
           offset == rootOffset)
  {
    
    mHTMLEditor->GetNextNode(parent, offset, PR_TRUE, address_of(nodeToExamine));
  }
  else
  {
    nsCOMArray<nsIDOMRange> arrayOfRanges;
    res = GetPromotedRanges(selection, arrayOfRanges, kAlign);
    if (NS_FAILED(res)) return res;

    
    nsCOMArray<nsIDOMNode> arrayOfNodes;
    res = GetNodesForOperation(arrayOfRanges, arrayOfNodes, kAlign, PR_TRUE);
    if (NS_FAILED(res)) return res;                                 
    nodeToExamine = arrayOfNodes.SafeObjectAt(0);
  }

  if (!nodeToExamine) return NS_ERROR_NULL_POINTER;

  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);
  NS_NAMED_LITERAL_STRING(typeAttrName, "align");
  nsIAtom  *dummyProperty = nsnull;
  nsCOMPtr<nsIDOMNode> blockParent;
  if (mHTMLEditor->IsBlockNode(nodeToExamine))
    blockParent = nodeToExamine;
  else
    blockParent = mHTMLEditor->GetBlockNodeParent(nodeToExamine);

  if (!blockParent) return NS_ERROR_FAILURE;

  if (useCSS)
  {
    nsCOMPtr<nsIContent> blockParentContent = do_QueryInterface(blockParent);
    if (blockParentContent && 
        mHTMLEditor->mHTMLCSSUtils->IsCSSEditableProperty(blockParent, dummyProperty, &typeAttrName))
    {
      
      nsAutoString value;
      
      mHTMLEditor->mHTMLCSSUtils->GetCSSEquivalentToHTMLInlineStyleSet(blockParent,
                                                     dummyProperty,
                                                     &typeAttrName,
                                                     value,
                                                     COMPUTED_STYLE_TYPE);
      if (value.EqualsLiteral("center") ||
          value.EqualsLiteral("-moz-center") ||
          value.EqualsLiteral("auto auto"))
      {
        *aAlign = nsIHTMLEditor::eCenter;
        return NS_OK;
      }
      if (value.EqualsLiteral("right") ||
          value.EqualsLiteral("-moz-right") ||
          value.EqualsLiteral("auto 0px"))
      {
        *aAlign = nsIHTMLEditor::eRight;
        return NS_OK;
      }
      if (value.EqualsLiteral("justify"))
      {
        *aAlign = nsIHTMLEditor::eJustify;
        return NS_OK;
      }
      *aAlign = nsIHTMLEditor::eLeft;
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIDOMNode> temp = nodeToExamine;
  PRBool isFirstNodeToExamine = PR_TRUE;
  while (nodeToExamine)
  {
    if (!isFirstNodeToExamine && nsHTMLEditUtils::IsTable(nodeToExamine))
    {
      
      
      
      return NS_OK;
    }
    if (nsHTMLEditUtils::SupportsAlignAttr(nodeToExamine))
    {
      
      nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(nodeToExamine);
      if (elem)
      {
        nsAutoString typeAttrVal;
        res = elem->GetAttribute(NS_LITERAL_STRING("align"), typeAttrVal);
        ToLowerCase(typeAttrVal);
        if (NS_SUCCEEDED(res) && typeAttrVal.Length())
        {
          if (typeAttrVal.EqualsLiteral("center"))
            *aAlign = nsIHTMLEditor::eCenter;
          else if (typeAttrVal.EqualsLiteral("right"))
            *aAlign = nsIHTMLEditor::eRight;
          else if (typeAttrVal.EqualsLiteral("justify"))
            *aAlign = nsIHTMLEditor::eJustify;
          else
            *aAlign = nsIHTMLEditor::eLeft;
          return res;
        }
      }
    }
    isFirstNodeToExamine = PR_FALSE;
    res = nodeToExamine->GetParentNode(getter_AddRefs(temp));
    if (NS_FAILED(res)) temp = nsnull;
    nodeToExamine = temp; 
  }
  return NS_OK;
}

nsIAtom* MarginPropertyAtomForIndent(nsHTMLCSSUtils* aHTMLCSSUtils, nsIDOMNode* aNode) {
  nsAutoString direction;
  aHTMLCSSUtils->GetComputedProperty(aNode, nsEditProperty::cssDirection, direction);
  return direction.EqualsLiteral("rtl") ?
    nsEditProperty::cssMarginRight : nsEditProperty::cssMarginLeft;
}

NS_IMETHODIMP 
nsHTMLEditRules::GetIndentState(PRBool *aCanIndent, PRBool *aCanOutdent)
{
  if (!aCanIndent || !aCanOutdent)
    return NS_ERROR_FAILURE;
  *aCanIndent = PR_TRUE;    
  *aCanOutdent = PR_FALSE;

  
  nsCOMPtr<nsISelection>selection;
  nsresult res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
  if (!selPriv)
    return NS_ERROR_FAILURE;

  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  res = GetNodesFromSelection(selection, kIndent, arrayOfNodes, PR_TRUE);
  if (NS_FAILED(res)) return res;

  
  
  
  PRInt32 listCount = arrayOfNodes.Count();
  PRInt32 i;
  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);
  for (i=listCount-1; i>=0; i--)
  {
    nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];
    
    if (nsHTMLEditUtils::IsNodeThatCanOutdent(curNode))
    {
      *aCanOutdent = PR_TRUE;
      break;
    }
    else if (useCSS) {
      
      nsIAtom* marginProperty = MarginPropertyAtomForIndent(mHTMLEditor->mHTMLCSSUtils, curNode);
      nsAutoString value;
      
      mHTMLEditor->mHTMLCSSUtils->GetSpecifiedProperty(curNode, marginProperty, value);
      float f;
      nsCOMPtr<nsIAtom> unit;
      
      mHTMLEditor->mHTMLCSSUtils->ParseLength(value, &f, getter_AddRefs(unit));
      
      if (0 < f) {
        *aCanOutdent = PR_TRUE;
        break;
      }
    }
  }  
  
  if (!*aCanOutdent)
  {
    
    
    
    
    
    nsCOMPtr<nsIDOMNode> parent, tmp, root;
    nsIDOMElement *rootElem = mHTMLEditor->GetRoot();
    if (!rootElem) return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsISelection> selection;
    PRInt32 selOffset;
    root = do_QueryInterface(rootElem);
    if (!root) return NS_ERROR_NO_INTERFACE;
    res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(res)) return res;
    if (!selection) return NS_ERROR_NULL_POINTER;
    
    
    res = mHTMLEditor->GetStartNodeAndOffset(selection, address_of(parent), &selOffset);
    if (NS_FAILED(res)) return res;
    while (parent && (parent!=root))
    {
      if (nsHTMLEditUtils::IsNodeThatCanOutdent(parent))
      {
        *aCanOutdent = PR_TRUE;
        break;
      }
      tmp=parent;
      tmp->GetParentNode(getter_AddRefs(parent));
    }

    
    res = mHTMLEditor->GetEndNodeAndOffset(selection, address_of(parent), &selOffset);
    if (NS_FAILED(res)) return res;
    while (parent && (parent!=root))
    {
      if (nsHTMLEditUtils::IsNodeThatCanOutdent(parent))
      {
        *aCanOutdent = PR_TRUE;
        break;
      }
      tmp=parent;
      tmp->GetParentNode(getter_AddRefs(parent));
    }
  }
  return res;
}


NS_IMETHODIMP 
nsHTMLEditRules::GetParagraphState(PRBool *aMixed, nsAString &outFormat)
{
  
  
  if (!aMixed)
    return NS_ERROR_NULL_POINTER;
  *aMixed = PR_TRUE;
  outFormat.Truncate(0);
  
  PRBool bMixed = PR_FALSE;
  
  nsAutoString formatStr(NS_LITERAL_STRING("x")); 
  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  nsresult res = GetParagraphFormatNodes(arrayOfNodes, PR_TRUE);
  if (NS_FAILED(res)) return res;

  
  
  
  PRInt32 listCount = arrayOfNodes.Count();
  PRInt32 i;
  for (i=listCount-1; i>=0; i--)
  {
    nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];
    nsAutoString format;
    
    if (IsBlockNode(curNode) && !nsHTMLEditUtils::IsFormatNode(curNode))
    {
      
      res = AppendInnerFormatNodes(arrayOfNodes, curNode);
      if (NS_FAILED(res)) return res;
    }
  }
  
  
  
  listCount = arrayOfNodes.Count();
  if (!listCount)
  {
    nsCOMPtr<nsIDOMNode> selNode;
    PRInt32 selOffset;
    nsCOMPtr<nsISelection>selection;
    res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->GetStartNodeAndOffset(selection, address_of(selNode), &selOffset);
    if (NS_FAILED(res)) return res;
    if (!selNode) return NS_ERROR_NULL_POINTER;
    arrayOfNodes.AppendObject(selNode);
    listCount = 1;
  }

  
  nsIDOMElement *rootElem = mHTMLEditor->GetRoot();
  if (!rootElem) return NS_ERROR_NULL_POINTER;

  
  for (i=listCount-1; i>=0; i--)
  {
    nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];
    nsAutoString format;
    
    if (nsHTMLEditUtils::IsFormatNode(curNode))
      GetFormatString(curNode, format);
    else if (IsBlockNode(curNode))
    {
      
      
      
      
      continue;
    }
    else
    {
      nsCOMPtr<nsIDOMNode> node, tmp = curNode;
      tmp->GetParentNode(getter_AddRefs(node));
      while (node)
      {
        if (node == rootElem)
        {
          format.Truncate(0);
          break;
        }
        else if (nsHTMLEditUtils::IsFormatNode(node))
        {
          GetFormatString(node, format);
          break;
        }
        
        tmp = node;
        tmp->GetParentNode(getter_AddRefs(node));
      }
    }
    
    
    if (formatStr.EqualsLiteral("x"))
      formatStr = format;
    
    else if (format != formatStr) 
    {
      bMixed = PR_TRUE;
      break; 
    }
  }  
  
  *aMixed = bMixed;
  outFormat = formatStr;
  return res;
}

nsresult 
nsHTMLEditRules::AppendInnerFormatNodes(nsCOMArray<nsIDOMNode>& aArray,
                                        nsIDOMNode *aNode)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNodeList> childList;
  nsCOMPtr<nsIDOMNode> child;

  aNode->GetChildNodes(getter_AddRefs(childList));
  if (!childList)  return NS_OK;
  PRUint32 len, j=0;
  childList->GetLength(&len);

  
  
  
  
  PRBool foundInline = PR_FALSE;
  while (j < len)
  {
    childList->Item(j, getter_AddRefs(child));
    PRBool isBlock = IsBlockNode(child);
    PRBool isFormat = nsHTMLEditUtils::IsFormatNode(child);
    if (isBlock && !isFormat)  
      AppendInnerFormatNodes(aArray, child);
    else if (isFormat)
    {
      aArray.AppendObject(child);
    }
    else if (!foundInline)  
    {
      foundInline = PR_TRUE;      
      aArray.AppendObject(child);
    }
    j++;
  }
  return NS_OK;
}

nsresult 
nsHTMLEditRules::GetFormatString(nsIDOMNode *aNode, nsAString &outFormat)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;

  if (nsHTMLEditUtils::IsFormatNode(aNode))
  {
    nsCOMPtr<nsIAtom> atom = nsEditor::GetTag(aNode);
    atom->ToString(outFormat);
  }
  else
    outFormat.Truncate();

  return NS_OK;
}    





nsresult
nsHTMLEditRules::WillInsert(nsISelection *aSelection, PRBool *aCancel)
{
  nsresult res = nsTextEditRules::WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res; 
  
  
  
  
  
  PRBool bCollapsed;
  res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (!bCollapsed) return NS_OK;

  
  
  nsCOMPtr<nsIDOMNode> selNode, priorNode;
  PRInt32 selOffset;
  
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(selNode),
                                           &selOffset);
  if (NS_FAILED(res)) return res;
  
  res = mHTMLEditor->GetPriorHTMLNode(selNode, selOffset,
                                      address_of(priorNode));
  if (NS_SUCCEEDED(res) && priorNode && nsTextEditUtils::IsMozBR(priorNode))
  {
    nsCOMPtr<nsIDOMNode> block1, block2;
    if (IsBlockNode(selNode)) block1 = selNode;
    else block1 = mHTMLEditor->GetBlockNodeParent(selNode);
    block2 = mHTMLEditor->GetBlockNodeParent(priorNode);
  
    if (block1 == block2)
    {
      
      
      
      res = nsEditor::GetNodeLocation(priorNode, address_of(selNode), &selOffset);
      if (NS_FAILED(res)) return res;
      res = aSelection->Collapse(selNode,selOffset);
      if (NS_FAILED(res)) return res;
    }
  }

  
  nsCOMPtr<nsIDOMDocument>doc;
  res = mHTMLEditor->GetDocument(getter_AddRefs(doc));
  if (NS_FAILED(res)) return res;
  if (!doc) return NS_ERROR_NULL_POINTER;
    
  
  return CreateStyleForInsertText(aSelection, doc);
}    

#ifdef XXX_DEAD_CODE
nsresult
nsHTMLEditRules::DidInsert(nsISelection *aSelection, nsresult aResult)
{
  return nsTextEditRules::DidInsert(aSelection, aResult);
}
#endif

nsresult
nsHTMLEditRules::WillInsertText(PRInt32          aAction,
                                nsISelection *aSelection, 
                                PRBool          *aCancel,
                                PRBool          *aHandled,
                                const nsAString *inString,
                                nsAString       *outString,
                                PRInt32          aMaxLength)
{  
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }



  if (inString->IsEmpty() && (aAction != kInsertTextIME))
  {
    
    
    
    
    
    *aCancel = PR_TRUE;
    *aHandled = PR_FALSE;
    return NS_OK;
  }
  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;
  nsresult res;
  nsCOMPtr<nsIDOMNode> selNode;
  PRInt32 selOffset;

  PRBool bPlaintext = mFlags & nsIPlaintextEditor::eEditorPlaintextMask;

  
  PRBool bCollapsed;
  res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (!bCollapsed)
  {
    res = mHTMLEditor->DeleteSelection(nsIEditor::eNone);
    if (NS_FAILED(res)) return res;
  }

  res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;
  
  
  *aCancel = PR_FALSE;
  
  
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;

  
  if (!mHTMLEditor->IsTextNode(selNode) &&
      !mHTMLEditor->CanContainTag(selNode, NS_LITERAL_STRING("#text")))
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMDocument>doc;
  res = mHTMLEditor->GetDocument(getter_AddRefs(doc));
  if (NS_FAILED(res)) return res;
  if (!doc) return NS_ERROR_NULL_POINTER;
    
  if (aAction == kInsertTextIME) 
  { 
    
    
    if (inString->IsEmpty())
    {
      res = mHTMLEditor->InsertTextImpl(*inString, address_of(selNode), &selOffset, doc);
    }
    else
    {
      nsWSRunObject wsObj(mHTMLEditor, selNode, selOffset);
      res = wsObj.InsertText(*inString, address_of(selNode), &selOffset, doc);
    }
    if (NS_FAILED(res)) return res;
  }
  else 
  {
    
    nsCOMPtr<nsIDOMNode> curNode = selNode;
    PRInt32 curOffset = selOffset;
    
    
    
    PRBool isPRE;
    res = mHTMLEditor->IsPreformatted(selNode, &isPRE);
    if (NS_FAILED(res)) return res;    
    
    
    
    
    
    nsAutoLockListener lockit(&mListenerEnabled); 
    
    
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    nsAutoString tString(*inString);
    const PRUnichar *unicodeBuf = tString.get();
    nsCOMPtr<nsIDOMNode> unused;
    PRInt32 pos = 0;
    NS_NAMED_LITERAL_STRING(newlineStr, LFSTR);
        
    
    
    
    if (isPRE || bPlaintext)
    {
      while (unicodeBuf && (pos != -1) && (pos < (PRInt32)(*inString).Length()))
      {
        PRInt32 oldPos = pos;
        PRInt32 subStrLen;
        pos = tString.FindChar(nsCRT::LF, oldPos);

        if (pos != -1) 
        {
          subStrLen = pos - oldPos;
          
          if (subStrLen == 0)
            subStrLen = 1;
        }
        else
        {
          subStrLen = tString.Length() - oldPos;
          pos = tString.Length();
        }

        nsDependentSubstring subStr(tString, oldPos, subStrLen);
        
        
        if (subStr.Equals(newlineStr))
        {
          res = mHTMLEditor->CreateBRImpl(address_of(curNode), &curOffset, address_of(unused), nsIEditor::eNone);
          pos++;
        }
        else
        {
          res = mHTMLEditor->InsertTextImpl(subStr, address_of(curNode), &curOffset, doc);
        }
        if (NS_FAILED(res)) return res;
      }
    }
    else
    {
      NS_NAMED_LITERAL_STRING(tabStr, "\t");
      NS_NAMED_LITERAL_STRING(spacesStr, "    ");
      char specialChars[] = {TAB, nsCRT::LF, 0};
      while (unicodeBuf && (pos != -1) && (pos < (PRInt32)inString->Length()))
      {
        PRInt32 oldPos = pos;
        PRInt32 subStrLen;
        pos = tString.FindCharInSet(specialChars, oldPos);
        
        if (pos != -1) 
        {
          subStrLen = pos - oldPos;
          
          if (subStrLen == 0)
            subStrLen = 1;
        }
        else
        {
          subStrLen = tString.Length() - oldPos;
          pos = tString.Length();
        }

        nsDependentSubstring subStr(tString, oldPos, subStrLen);
        nsWSRunObject wsObj(mHTMLEditor, curNode, curOffset);

        
        if (subStr.Equals(tabStr))
        {
          res = wsObj.InsertText(spacesStr, address_of(curNode), &curOffset, doc);
          if (NS_FAILED(res)) return res;
          pos++;
        }
        
        else if (subStr.Equals(newlineStr))
        {
          res = wsObj.InsertBreak(address_of(curNode), &curOffset, address_of(unused), nsIEditor::eNone);
          if (NS_FAILED(res)) return res;
          pos++;
        }
        else
        {
          res = wsObj.InsertText(subStr, address_of(curNode), &curOffset, doc);
          if (NS_FAILED(res)) return res;
        }
        if (NS_FAILED(res)) return res;
      }
    }
    nsCOMPtr<nsISelection> selection(aSelection);
    nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
    selPriv->SetInterlinePosition(PR_FALSE);
    if (curNode) aSelection->Collapse(curNode, curOffset);
    
    
    if (!mDocChangeRange)
    {
      mDocChangeRange = do_CreateInstance("@mozilla.org/content/range;1");
      if (!mDocChangeRange) return NS_ERROR_NULL_POINTER;
    }
    res = mDocChangeRange->SetStart(selNode, selOffset);
    if (NS_FAILED(res)) return res;
    if (curNode)
      res = mDocChangeRange->SetEnd(curNode, curOffset);
    else
      res = mDocChangeRange->SetEnd(selNode, selOffset);
    if (NS_FAILED(res)) return res;
  }
  return res;
}

nsresult
nsHTMLEditRules::WillLoadHTML(nsISelection *aSelection, PRBool *aCancel)
{
  if (!aSelection || !aCancel) return NS_ERROR_NULL_POINTER;

  *aCancel = PR_FALSE;

  
  

  if (mBogusNode)
  {
    mEditor->DeleteNode(mBogusNode);
    mBogusNode = nsnull;
  }

  return NS_OK;
}

nsresult
nsHTMLEditRules::WillInsertBreak(nsISelection *aSelection, PRBool *aCancel, PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;
  
  PRBool bPlaintext = mFlags & nsIPlaintextEditor::eEditorPlaintextMask;

  
  PRBool bCollapsed;
  nsresult res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (!bCollapsed)
  {
    res = mHTMLEditor->DeleteSelection(nsIEditor::eNone);
    if (NS_FAILED(res)) return res;
  }
  
  res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;
  
  
  
  *aCancel = PR_FALSE;
  
  
  
  if (mFlags & nsIPlaintextEditor::eEditorMailMask)
  {
    res = SplitMailCites(aSelection, bPlaintext, aHandled);
    if (NS_FAILED(res)) return res;
    if (*aHandled) return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 offset;
  
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(node), &offset);
  if (NS_FAILED(res)) return res;
  if (!node) return NS_ERROR_FAILURE;
    
  
  nsCOMPtr<nsIDOMNode> blockParent;
  
  if (IsBlockNode(node)) 
    blockParent = node;
  else 
    blockParent = mHTMLEditor->GetBlockNodeParent(node);
    
  if (!blockParent) return NS_ERROR_FAILURE;
  
  
  
  
  
  
  PRBool isEmpty;
  res = IsEmptyBlock(blockParent, &isEmpty);
  if (isEmpty)
  {
    PRUint32 blockLen;
    res = mHTMLEditor->GetLengthOfDOMNode(blockParent, blockLen);
    if (NS_FAILED(res)) return res;
    nsCOMPtr<nsIDOMNode> brNode;
    res = mHTMLEditor->CreateBR(blockParent, blockLen, address_of(brNode));
    if (NS_FAILED(res)) return res;
  }
  
  nsCOMPtr<nsIDOMNode> listItem = IsInListItem(blockParent);
  if (listItem)
  {
    res = ReturnInListItem(aSelection, listItem, node, offset);
    *aHandled = PR_TRUE;
    return NS_OK;
  }
  
  
  else if (nsHTMLEditUtils::IsHeader(blockParent))
  {
    res = ReturnInHeader(aSelection, blockParent, node, offset);
    *aHandled = PR_TRUE;
    return NS_OK;
  }
  
  
  else if (nsHTMLEditUtils::IsParagraph(blockParent))
  {
    res = ReturnInParagraph(aSelection, blockParent, node, offset, aCancel, aHandled);
    if (NS_FAILED(res)) return res;
    
  }
  
  
  if (!(*aHandled))
  {
    res = StandardBreakImpl(node, offset, aSelection);
    *aHandled = PR_TRUE;
  }
  return res;
}

nsresult
nsHTMLEditRules::StandardBreakImpl(nsIDOMNode *aNode, PRInt32 aOffset, nsISelection *aSelection)
{
  nsCOMPtr<nsIDOMNode> brNode;
  PRBool bAfterBlock = PR_FALSE;
  PRBool bBeforeBlock = PR_FALSE;
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> node(aNode);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(aSelection));
  
  if (mFlags & nsIPlaintextEditor::eEditorPlaintextMask)
  {
    res = mHTMLEditor->CreateBR(node, aOffset, address_of(brNode));
  }
  else
  {
    nsWSRunObject wsObj(mHTMLEditor, node, aOffset);
    nsCOMPtr<nsIDOMNode> visNode, linkNode;
    PRInt32 visOffset=0, newOffset;
    PRInt16 wsType;
    res = wsObj.PriorVisibleNode(node, aOffset, address_of(visNode), &visOffset, &wsType);
    if (NS_FAILED(res)) return res;
    if (wsType & nsWSRunObject::eBlock)
      bAfterBlock = PR_TRUE;
    res = wsObj.NextVisibleNode(node, aOffset, address_of(visNode), &visOffset, &wsType);
    if (NS_FAILED(res)) return res;
    if (wsType & nsWSRunObject::eBlock)
      bBeforeBlock = PR_TRUE;
    if (mHTMLEditor->IsInLink(node, address_of(linkNode)))
    {
      
      nsCOMPtr<nsIDOMNode> linkParent;
      res = linkNode->GetParentNode(getter_AddRefs(linkParent));
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->SplitNodeDeep(linkNode, node, aOffset, &newOffset, PR_TRUE);
      if (NS_FAILED(res)) return res;
      
      node = linkParent;
      aOffset = newOffset;
    }
    res = wsObj.InsertBreak(address_of(node), &aOffset, address_of(brNode), nsIEditor::eNone);
  }
  if (NS_FAILED(res)) return res;
  res = nsEditor::GetNodeLocation(brNode, address_of(node), &aOffset);
  if (NS_FAILED(res)) return res;
  if (bAfterBlock && bBeforeBlock)
  {
    
    
    
    
    selPriv->SetInterlinePosition(PR_TRUE);
    res = aSelection->Collapse(node, aOffset);
  }
  else
  {
     nsWSRunObject wsObj(mHTMLEditor, node, aOffset+1);
     nsCOMPtr<nsIDOMNode> secondBR;
     PRInt32 visOffset=0;
     PRInt16 wsType;
     res = wsObj.NextVisibleNode(node, aOffset+1, address_of(secondBR), &visOffset, &wsType);
     if (NS_FAILED(res)) return res;
     if (wsType==nsWSRunObject::eBreak)
     {
       
       
       
       
       
       nsCOMPtr<nsIDOMNode> brParent;
       PRInt32 brOffset;
       res = nsEditor::GetNodeLocation(secondBR, address_of(brParent), &brOffset);
       if (NS_FAILED(res)) return res;
       if ((brParent != node) || (brOffset != (aOffset+1)))
       {
         res = mHTMLEditor->MoveNode(secondBR, node, aOffset+1);
         if (NS_FAILED(res)) return res;
       }
     }
    
    
    
    
    
    
    
    nsCOMPtr<nsIDOMNode> siblingNode;
    brNode->GetNextSibling(getter_AddRefs(siblingNode));
    if (siblingNode && IsBlockNode(siblingNode))
      selPriv->SetInterlinePosition(PR_FALSE);
    else 
      selPriv->SetInterlinePosition(PR_TRUE);
    res = aSelection->Collapse(node, aOffset+1);
  }
  return res;
}

nsresult
nsHTMLEditRules::DidInsertBreak(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}


nsresult
nsHTMLEditRules::SplitMailCites(nsISelection *aSelection, PRBool aPlaintext, PRBool *aHandled)
{
  if (!aSelection || !aHandled)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(aSelection));
  nsCOMPtr<nsIDOMNode> citeNode, selNode, leftCite, rightCite;
  PRInt32 selOffset, newOffset;
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  res = GetTopEnclosingMailCite(selNode, address_of(citeNode), aPlaintext);
  if (NS_FAILED(res)) return res;
  if (citeNode)
  {
    
    
    
    
    
    
    
    
    nsWSRunObject wsObj(mHTMLEditor, selNode, selOffset);
    nsCOMPtr<nsIDOMNode> visNode;
    PRInt32 visOffset=0;
    PRInt16 wsType;
    res = wsObj.NextVisibleNode(selNode, selOffset, address_of(visNode), &visOffset, &wsType);
    if (NS_FAILED(res)) return res;
    if (wsType==nsWSRunObject::eBreak)
    {
      
      PRInt32 unused;
      if (nsEditorUtils::IsDescendantOf(visNode, citeNode, &unused))
      {
        
        res = mHTMLEditor->GetNodeLocation(visNode, address_of(selNode), &selOffset);
        if (NS_FAILED(res)) return res;
        ++selOffset;
      }
    }
     
    nsCOMPtr<nsIDOMNode> brNode;
    res = mHTMLEditor->SplitNodeDeep(citeNode, selNode, selOffset, &newOffset, 
                       PR_TRUE, address_of(leftCite), address_of(rightCite));
    if (NS_FAILED(res)) return res;
    res = citeNode->GetParentNode(getter_AddRefs(selNode));
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->CreateBR(selNode, newOffset, address_of(brNode));
    if (NS_FAILED(res)) return res;
    
    selPriv->SetInterlinePosition(PR_TRUE);
    res = aSelection->Collapse(selNode, newOffset);
    if (NS_FAILED(res)) return res;
    
    
    
    
    if (IsInlineNode(citeNode))
    {
      nsWSRunObject wsObj(mHTMLEditor, selNode, newOffset);
      nsCOMPtr<nsIDOMNode> visNode;
      PRInt32 visOffset=0;
      PRInt16 wsType;
      res = wsObj.PriorVisibleNode(selNode, newOffset, address_of(visNode), &visOffset, &wsType);
      if (NS_FAILED(res)) return res;
      if ((wsType==nsWSRunObject::eNormalWS) || 
          (wsType==nsWSRunObject::eText)     ||
          (wsType==nsWSRunObject::eSpecial))
      {
        nsWSRunObject wsObjAfterBR(mHTMLEditor, selNode, newOffset+1);
        res = wsObjAfterBR.NextVisibleNode(selNode, newOffset+1, address_of(visNode), &visOffset, &wsType);
        if (NS_FAILED(res)) return res;
        if ((wsType==nsWSRunObject::eNormalWS) || 
            (wsType==nsWSRunObject::eText)     ||
            (wsType==nsWSRunObject::eSpecial))
        {
          res = mHTMLEditor->CreateBR(selNode, newOffset, address_of(brNode));
          if (NS_FAILED(res)) return res;
        }
      }
    }
    
    PRBool bEmptyCite = PR_FALSE;
    if (leftCite)
    {
      res = mHTMLEditor->IsEmptyNode(leftCite, &bEmptyCite, PR_TRUE, PR_FALSE);
      if (NS_SUCCEEDED(res) && bEmptyCite)
        res = mHTMLEditor->DeleteNode(leftCite);
      if (NS_FAILED(res)) return res;
    }
    if (rightCite)
    {
      res = mHTMLEditor->IsEmptyNode(rightCite, &bEmptyCite, PR_TRUE, PR_FALSE);
      if (NS_SUCCEEDED(res) && bEmptyCite)
        res = mHTMLEditor->DeleteNode(rightCite);
      if (NS_FAILED(res)) return res;
    }
    *aHandled = PR_TRUE;
  }
  return NS_OK;
}


nsresult
nsHTMLEditRules::WillDeleteSelection(nsISelection *aSelection, 
                                     nsIEditor::EDirection aAction, 
                                     PRBool *aCancel,
                                     PRBool *aHandled)
{

  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;

  
  mDidDeleteSelection = PR_TRUE;
  
  
  if (mBogusNode) 
  {
    *aCancel = PR_TRUE;
    return NS_OK;
  }

  nsresult res = NS_OK;
  PRBool bPlaintext = mFlags & nsIPlaintextEditor::eEditorPlaintextMask;
  
  PRBool bCollapsed;
  res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  
  nsCOMPtr<nsIDOMNode> startNode, selNode;
  PRInt32 startOffset, selOffset;
  
  
  
  {
    nsCOMPtr<nsIDOMElement> cell;
    res = mHTMLEditor->GetFirstSelectedCell(nsnull, getter_AddRefs(cell));
    if (NS_SUCCEEDED(res) && cell)
    {
      res = mHTMLEditor->DeleteTableCellContents();
      *aHandled = PR_TRUE;
      return res;
    }
  }
  
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(startNode), &startOffset);
  if (NS_FAILED(res)) return res;
  if (!startNode) return NS_ERROR_FAILURE;
    
  
  nsIDOMElement *rootNode = mHTMLEditor->GetRoot();
  if (!rootNode) return NS_ERROR_UNEXPECTED;

  if (bCollapsed)
  {
    
    res = CheckForEmptyBlock(startNode, rootNode, aSelection, aHandled);
    if (NS_FAILED(res)) return res;
    if (*aHandled) return NS_OK;
        
    
    res = CheckBidiLevelForDeletion(aSelection, startNode, startOffset, aAction, aCancel);
    if (NS_FAILED(res)) return res;
    if (*aCancel) return NS_OK;

    
    if (aAction == nsIEditor::eNone)
      return NS_OK;

    
    nsWSRunObject wsObj(mHTMLEditor, startNode, startOffset);
    nsCOMPtr<nsIDOMNode> visNode;
    PRInt32 visOffset;
    PRInt16 wsType;

    
    if (aAction == nsIEditor::eNext)
      res = wsObj.NextVisibleNode(startNode, startOffset, address_of(visNode), &visOffset, &wsType);
    else
      res = wsObj.PriorVisibleNode(startNode, startOffset, address_of(visNode), &visOffset, &wsType);
    if (NS_FAILED(res)) return res;
    
    if (!visNode) 
    {
      *aCancel = PR_TRUE;
      return res;
    }
    
    if (wsType==nsWSRunObject::eNormalWS)
    {
      
      if (aAction == nsIEditor::eNext)
        res = wsObj.DeleteWSForward();
      else
        res = wsObj.DeleteWSBackward();
      *aHandled = PR_TRUE;
      if (NS_FAILED(res)) return res;
      res = InsertBRIfNeeded(aSelection);
      return res;
    } 
    else if (wsType==nsWSRunObject::eText)
    {
      
      PRInt32 so = visOffset;
      PRInt32 eo = visOffset+1;
      if (aAction == nsIEditor::ePrevious) 
      { 
        if (so == 0) return NS_ERROR_UNEXPECTED;
        so--; 
        eo--; 
      }
      res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor, address_of(visNode), &so, address_of(visNode), &eo);
      if (NS_FAILED(res)) return res;
      nsCOMPtr<nsIDOMCharacterData> nodeAsText(do_QueryInterface(visNode));
      res = mHTMLEditor->DeleteText(nodeAsText,so,1);
      *aHandled = PR_TRUE;
      if (NS_FAILED(res)) return res;    
      res = InsertBRIfNeeded(aSelection);
      return res;
    }
    else if ( (wsType==nsWSRunObject::eSpecial)  || 
              (wsType==nsWSRunObject::eBreak)    ||
              nsHTMLEditUtils::IsHR(visNode) ) 
    {
      
      if (nsTextEditUtils::IsBreak(visNode) && !mHTMLEditor->IsVisBreak(visNode))
      {
        res = mHTMLEditor->DeleteNode(visNode);
        if (NS_FAILED(res)) return res;
        return WillDeleteSelection(aSelection, aAction, aCancel, aHandled);
      }
      
      
      if (aAction == nsIEditor::ePrevious && nsHTMLEditUtils::IsHR(visNode))
      {
        




















        PRBool moveOnly = PR_TRUE;

        res = nsEditor::GetNodeLocation(visNode, address_of(selNode), &selOffset);
        if (NS_FAILED(res)) return res;

        PRBool interLineIsRight;
        nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(aSelection));
        res = selPriv->GetInterlinePosition(&interLineIsRight);
        if (NS_FAILED(res)) return res;

        if (startNode == selNode &&
            startOffset -1 == selOffset &&
            !interLineIsRight)
        {
          moveOnly = PR_FALSE;
        }
        
        if (moveOnly)
        {
          
          
          ++selOffset;
          res = aSelection->Collapse(selNode, selOffset);
          selPriv->SetInterlinePosition(PR_FALSE);
          mDidExplicitlySetInterline = PR_TRUE;
          *aHandled = PR_TRUE;

          
          

          PRInt16 otherWSType;
          nsCOMPtr<nsIDOMNode> otherNode;
          PRInt32 otherOffset;

          res = wsObj.NextVisibleNode(startNode, startOffset, address_of(otherNode), &otherOffset, &otherWSType);
          if (NS_FAILED(res)) return res;

          if (otherWSType == nsWSRunObject::eBreak)
          {
            

            res = nsWSRunObject::PrepareToDeleteNode(mHTMLEditor, otherNode);
            if (NS_FAILED(res)) return res;
            res = mHTMLEditor->DeleteNode(otherNode);
            if (NS_FAILED(res)) return res;
          }

          return NS_OK;
        }
        
      }

      
      res = nsWSRunObject::PrepareToDeleteNode(mHTMLEditor, visNode);
      if (NS_FAILED(res)) return res;
      
      nsCOMPtr<nsIDOMNode> sibling, stepbrother;
      mHTMLEditor->GetPriorHTMLSibling(visNode, address_of(sibling));
      
      res = mHTMLEditor->DeleteNode(visNode);
      if (NS_FAILED(res)) return res;
      
      *aHandled = PR_TRUE;
      
      if (sibling)
         mHTMLEditor->GetNextHTMLSibling(sibling, address_of(stepbrother));
      if (startNode == stepbrother) 
      {
        
        if (mHTMLEditor->IsTextNode(startNode) && mHTMLEditor->IsTextNode(sibling))
        {
          
          res = JoinNodesSmart(sibling, startNode, address_of(selNode), &selOffset);
          if (NS_FAILED(res)) return res;
          
          res = aSelection->Collapse(selNode, selOffset);
        }
      }
      if (NS_FAILED(res)) return res;    
      res = InsertBRIfNeeded(aSelection);
      return res;
    }
    else if (wsType==nsWSRunObject::eOtherBlock)
    {
      
      
      if (nsHTMLEditUtils::IsTableElement(visNode))
      {
        *aCancel = PR_TRUE;
        return NS_OK;
      }
      
      
      
      
      PRBool bDeletedBR = PR_FALSE;
      PRInt16 otherWSType;
      nsCOMPtr<nsIDOMNode> otherNode;
      PRInt32 otherOffset;
      
      
      if (aAction == nsIEditor::eNext)
        res = wsObj.PriorVisibleNode(startNode, startOffset, address_of(otherNode), &otherOffset, &otherWSType);
      else
        res = wsObj.NextVisibleNode(startNode, startOffset, address_of(otherNode), &otherOffset, &otherWSType);
      if (NS_FAILED(res)) return res;
      
      
      nsCOMPtr<nsIDOMNode> leafNode, leftNode, rightNode, leftParent, rightParent;
      if (aAction == nsIEditor::ePrevious) 
      {
        res = mHTMLEditor->GetLastEditableLeaf( visNode, address_of(leafNode));
        if (NS_FAILED(res)) return res;
        leftNode = leafNode;
        rightNode = startNode;
      }
      else
      {
        res = mHTMLEditor->GetFirstEditableLeaf( visNode, address_of(leafNode));
        if (NS_FAILED(res)) return res;
        leftNode = startNode;
        rightNode = leafNode;
      }
      
      if (nsTextEditUtils::IsBreak(otherNode))
      {
        res = mHTMLEditor->DeleteNode(otherNode);
        if (NS_FAILED(res)) return res;
        *aHandled = PR_TRUE;
        bDeletedBR = PR_TRUE;
      }
      
      
      if (leftNode && rightNode)
      {
        PRBool bInDifTblElems;
        res = InDifferentTableElements(leftNode, rightNode, &bInDifTblElems);
        if (NS_FAILED(res) || bInDifTblElems) return res;
      }
      
      if (bDeletedBR)
      {
        
        nsCOMPtr<nsIDOMNode> newSelNode;
        PRInt32 newSelOffset;
        res = GetGoodSelPointForNode(leafNode, aAction, address_of(newSelNode), &newSelOffset);
        if (NS_FAILED(res)) return res;
        aSelection->Collapse(newSelNode, newSelOffset);
        return res;
      }
      
      
      
      
      if (IsBlockNode(leftNode))
        leftParent = leftNode;
      else
        leftParent = mHTMLEditor->GetBlockNodeParent(leftNode);
      if (IsBlockNode(rightNode))
        rightParent = rightNode;
      else
        rightParent = mHTMLEditor->GetBlockNodeParent(rightNode);
      
      
      if (!leftParent || !rightParent)
        return NS_ERROR_NULL_POINTER;  
      if (leftParent == rightParent)
        return NS_ERROR_UNEXPECTED;  
      
      
      nsCOMPtr<nsIDOMNode> selPointNode = startNode;
      PRInt32 selPointOffset = startOffset;
      {
        nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, address_of(selPointNode), &selPointOffset);
        res = JoinBlocks(address_of(leftParent), address_of(rightParent), aCancel);
        *aHandled = PR_TRUE;
      }
      aSelection->Collapse(selPointNode, selPointOffset);
      return res;
    }
    else if (wsType==nsWSRunObject::eThisBlock)
    {
      
      
      
      
      if (nsHTMLEditUtils::IsTableElement(visNode))
      {
        *aCancel = PR_TRUE;
        return NS_OK;
      }
      
      
      nsCOMPtr<nsIDOMNode> leftNode, rightNode, leftParent, rightParent;
      if (aAction == nsIEditor::ePrevious) 
      {
        res = mHTMLEditor->GetPriorHTMLNode(visNode, address_of(leftNode));
        if (NS_FAILED(res)) return res;
        rightNode = startNode;
      }
      else
      {
        res = mHTMLEditor->GetNextHTMLNode( visNode, address_of(rightNode));
        if (NS_FAILED(res)) return res;
        leftNode = startNode;
      }

      
      if (!leftNode || !rightNode)
      {
        *aCancel = PR_TRUE;
        return NS_OK;
      }

      
      PRBool bInDifTblElems;
      res = InDifferentTableElements(leftNode, rightNode, &bInDifTblElems);
      if (NS_FAILED(res) || bInDifTblElems) return res;

      
      if (IsBlockNode(leftNode))
        leftParent = leftNode;
      else
        leftParent = mHTMLEditor->GetBlockNodeParent(leftNode);
      if (IsBlockNode(rightNode))
        rightParent = rightNode;
      else
        rightParent = mHTMLEditor->GetBlockNodeParent(rightNode);
      
      
      if (!leftParent || !rightParent)
        return NS_ERROR_NULL_POINTER;  
      if (leftParent == rightParent)
        return NS_ERROR_UNEXPECTED;  
      
      
      nsCOMPtr<nsIDOMNode> selPointNode = startNode;
      PRInt32 selPointOffset = startOffset;
      {
        nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, address_of(selPointNode), &selPointOffset);
        res = JoinBlocks(address_of(leftParent), address_of(rightParent), aCancel);
        *aHandled = PR_TRUE;
      }
      aSelection->Collapse(selPointNode, selPointOffset);
      return res;
    }
  }

  
  
  
  res = ExpandSelectionForDeletion(aSelection);
  if (NS_FAILED(res)) return res;
  
  
  mDidRangedDelete = PR_TRUE;
  
  
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(startNode), &startOffset);
  if (NS_FAILED(res)) return res;
  if (!startNode) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 endOffset;
  res = mHTMLEditor->GetEndNodeAndOffset(aSelection, address_of(endNode), &endOffset);
  if (NS_FAILED(res)) return res; 
  if (!endNode) return NS_ERROR_FAILURE;

  
  
  if (!bPlaintext)
  {
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor,
                                            address_of(startNode), &startOffset, 
                                            address_of(endNode), &endOffset);
    if (NS_FAILED(res)) return res; 
  }
  
  {
    
    nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, address_of(endNode), &endOffset);
    
    *aHandled = PR_TRUE;
    
    if (endNode == startNode)
    {
      res = mHTMLEditor->DeleteSelectionImpl(aAction);
      if (NS_FAILED(res)) return res; 
    }
    else
    {
      
      nsCOMPtr<nsIDOMNode> endCiteNode, startCiteNode;
      res = GetTopEnclosingMailCite(startNode, address_of(startCiteNode), 
                                    mFlags & nsIPlaintextEditor::eEditorPlaintextMask);
      if (NS_FAILED(res)) return res; 
      res = GetTopEnclosingMailCite(endNode, address_of(endCiteNode), 
                                    mFlags & nsIPlaintextEditor::eEditorPlaintextMask);
      if (NS_FAILED(res)) return res; 
      
      
      
      if (startCiteNode && !endCiteNode)
      {
        aAction = nsIEditor::eNext;
      }
      else if (!startCiteNode && endCiteNode)
      {
        aAction = nsIEditor::ePrevious;
      }
      
      
      nsCOMPtr<nsIDOMNode> leftParent;
      nsCOMPtr<nsIDOMNode> rightParent;
      if (IsBlockNode(startNode))
        leftParent = startNode;
      else
        leftParent = mHTMLEditor->GetBlockNodeParent(startNode);
      if (IsBlockNode(endNode))
        rightParent = endNode;
      else
        rightParent = mHTMLEditor->GetBlockNodeParent(endNode);
        
      
      if (leftParent == rightParent) 
      {
        res = mHTMLEditor->DeleteSelectionImpl(aAction);
      }
      else
      {
        
        
        
        
        nsCOMPtr<nsIDOMNode> leftBlockParent;
        nsCOMPtr<nsIDOMNode> rightBlockParent;
        leftParent->GetParentNode(getter_AddRefs(leftBlockParent));
        rightParent->GetParentNode(getter_AddRefs(rightBlockParent));

        
        if (   (leftBlockParent == rightBlockParent)
            && (mHTMLEditor->NodesSameType(leftParent, rightParent))  )
        {
          if (nsHTMLEditUtils::IsParagraph(leftParent))
          {
            
            res = mHTMLEditor->DeleteSelectionImpl(aAction);
            if (NS_FAILED(res)) return res;
            
            res = mHTMLEditor->JoinNodeDeep(leftParent,rightParent,address_of(selNode),&selOffset);
            if (NS_FAILED(res)) return res;
            
            res = aSelection->Collapse(selNode,selOffset);
            return res;
          }
          if (nsHTMLEditUtils::IsListItem(leftParent)
              || nsHTMLEditUtils::IsHeader(leftParent))
          {
            
            res = mHTMLEditor->DeleteSelectionImpl(aAction);
            if (NS_FAILED(res)) return res;
            
            res = mHTMLEditor->JoinNodeDeep(leftParent,rightParent,address_of(selNode),&selOffset);
            if (NS_FAILED(res)) return res;
            
            res = aSelection->Collapse(selNode,selOffset);
            return res;
          }
        }
        
        
        
        nsCOMPtr<nsIEnumerator> enumerator;
        nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(aSelection));
        res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
        if (NS_FAILED(res)) return res;
        if (!enumerator) return NS_ERROR_UNEXPECTED;

        for (enumerator->First(); NS_OK!=enumerator->IsDone(); enumerator->Next())
        {
          nsCOMPtr<nsISupports> currentItem;
          res = enumerator->CurrentItem(getter_AddRefs(currentItem));
          if (NS_FAILED(res)) return res;
          if (!currentItem) return NS_ERROR_UNEXPECTED;

          
          nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
          nsCOMArray<nsIDOMNode> arrayOfNodes;
          nsTrivialFunctor functor;
          nsDOMSubtreeIterator iter;
          res = iter.Init(range);
          if (NS_FAILED(res)) return res;
          res = iter.AppendList(functor, arrayOfNodes);
          if (NS_FAILED(res)) return res;
      
          
          PRInt32 listCount = arrayOfNodes.Count();
          PRInt32 j;

          for (j = 0; j < listCount; j++)
          {
            nsIDOMNode* somenode = arrayOfNodes[0];
            res = DeleteNonTableElements(somenode);
            arrayOfNodes.RemoveObjectAt(0);
          }
        }
        
        
        
        
        
        
        if ( mHTMLEditor->IsTextNode(startNode) )
        {
          
          nsCOMPtr<nsIDOMCharacterData>nodeAsText;
          PRUint32 len;
          nodeAsText = do_QueryInterface(startNode);
          nodeAsText->GetLength(&len);
          if (len > (PRUint32)startOffset)
          {
            res = mHTMLEditor->DeleteText(nodeAsText,startOffset,len-startOffset);
            if (NS_FAILED(res)) return res;
          }
        }
        if ( mHTMLEditor->IsTextNode(endNode) )
        {
          
          nsCOMPtr<nsIDOMCharacterData>nodeAsText;
          nodeAsText = do_QueryInterface(endNode);
          if (endOffset)
          {
            res = mHTMLEditor->DeleteText(nodeAsText,0,endOffset);
            if (NS_FAILED(res)) return res;
          }
        }
      }
    }
  }
  if (aAction == nsIEditor::eNext)
  {
    res = aSelection->Collapse(endNode,endOffset);
  }
  else
  {
    res = aSelection->Collapse(startNode,startOffset);
  }
  return res;
}  







nsresult
nsHTMLEditRules::InsertBRIfNeeded(nsISelection *aSelection)
{
  if (!aSelection)
    return NS_ERROR_NULL_POINTER;
  
  
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 offset;
  nsresult res = mEditor->GetStartNodeAndOffset(aSelection, address_of(node), &offset);
  if (NS_FAILED(res)) return res;
  if (!node) return NS_ERROR_FAILURE;

  
  nsWSRunObject wsObj(mHTMLEditor, node, offset);
  if (((wsObj.mStartReason & nsWSRunObject::eBlock) || (wsObj.mStartReason & nsWSRunObject::eBreak))
      && (wsObj.mEndReason & nsWSRunObject::eBlock))
  {
    
    
    if (mHTMLEditor->CanContainTag(node, NS_LITERAL_STRING("br")))
    {
      nsCOMPtr<nsIDOMNode> brNode;
      res = mHTMLEditor->CreateBR(node, offset, address_of(brNode), nsIEditor::ePrevious);
    }
  }
  return res;
}









nsresult
nsHTMLEditRules::GetGoodSelPointForNode(nsIDOMNode *aNode, nsIEditor::EDirection aAction, 
                                        nsCOMPtr<nsIDOMNode> *outSelNode, PRInt32 *outSelOffset)
{
  if (!aNode || !outSelNode || !outSelOffset)
    return NS_ERROR_NULL_POINTER;
  
  nsresult res = NS_OK;
  
  
  *outSelNode = aNode;
  *outSelOffset = 0;
  
  if (mHTMLEditor->IsTextNode(aNode) || mHTMLEditor->IsContainer(aNode))
  {
    if (aAction == nsIEditor::ePrevious)
    {
      PRUint32 len;
      res = mHTMLEditor->GetLengthOfDOMNode(aNode, len);
      *outSelOffset = PRInt32(len);
      if (NS_FAILED(res)) return res;
    }
  }
  else 
  {
    res = nsEditor::GetNodeLocation(aNode, outSelNode, outSelOffset);
    if (NS_FAILED(res)) return res;
    if (!nsTextEditUtils::IsBreak(aNode) || mHTMLEditor->IsVisBreak(aNode))
    {
      if (aAction == nsIEditor::ePrevious)
        (*outSelOffset)++;
    }
  }
  return res;
}












nsresult
nsHTMLEditRules::JoinBlocks(nsCOMPtr<nsIDOMNode> *aLeftBlock, 
                            nsCOMPtr<nsIDOMNode> *aRightBlock, 
                            PRBool *aCanceled)
{
  if (!aLeftBlock || !aRightBlock || !*aLeftBlock || !*aRightBlock) return NS_ERROR_NULL_POINTER;
  if (nsHTMLEditUtils::IsTableElement(*aLeftBlock) || nsHTMLEditUtils::IsTableElement(*aRightBlock))
  {
    
    *aCanceled = PR_TRUE;
    return NS_OK;
  }

  
  if (nsHTMLEditUtils::IsHR(*aLeftBlock))
  {
    nsCOMPtr<nsIDOMNode> realLeft = mHTMLEditor->GetBlockNodeParent(*aLeftBlock);
    *aLeftBlock = realLeft;
  }
  if (nsHTMLEditUtils::IsHR(*aRightBlock))
  {
    nsCOMPtr<nsIDOMNode> realRight = mHTMLEditor->GetBlockNodeParent(*aRightBlock);
    *aRightBlock = realRight;
  }

  
  if (*aLeftBlock == *aRightBlock)
  {
    *aCanceled = PR_TRUE;
    return NS_OK;
  }
  
  
  
  PRBool bMergeLists = PR_FALSE;
  nsAutoString existingListStr;
  PRInt32 theOffset;
  nsCOMPtr<nsIDOMNode> leftList, rightList;
  if (nsHTMLEditUtils::IsListItem(*aLeftBlock) && nsHTMLEditUtils::IsListItem(*aRightBlock))
  {
    (*aLeftBlock)->GetParentNode(getter_AddRefs(leftList));
    (*aRightBlock)->GetParentNode(getter_AddRefs(rightList));
    if (leftList && rightList && (leftList!=rightList))
    {
      
      
      
      
      if (!nsEditorUtils::IsDescendantOf(leftList, *aRightBlock, &theOffset) &&
          !nsEditorUtils::IsDescendantOf(rightList, *aLeftBlock, &theOffset))
      {
        *aLeftBlock = leftList;
        *aRightBlock = rightList;
        bMergeLists = PR_TRUE;
        mHTMLEditor->GetTagString(leftList, existingListStr);
        ToLowerCase(existingListStr);
      }
    }
  }
  
  nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
  
  nsresult res = NS_OK;
  PRInt32  rightOffset = 0;
  PRInt32  leftOffset  = -1;

  
  
  if (nsEditorUtils::IsDescendantOf(*aLeftBlock, *aRightBlock, &rightOffset))
  {
    
    
    rightOffset++;
    res = nsWSRunObject::ScrubBlockBoundary(mHTMLEditor, aLeftBlock, nsWSRunObject::kBlockEnd);
    if (NS_FAILED(res)) return res;
    res = nsWSRunObject::ScrubBlockBoundary(mHTMLEditor, aRightBlock, nsWSRunObject::kAfterBlock, &rightOffset);
    if (NS_FAILED(res)) return res;
    
    nsCOMPtr<nsIDOMNode> brNode;
    res = CheckForInvisibleBR(*aLeftBlock, kBlockEnd, address_of(brNode));
    if (NS_FAILED(res)) return res;
    if (bMergeLists)
    {
      
      
      nsCOMPtr<nsIDOMNode> childToMove;
      nsCOMPtr<nsIContent> parent(do_QueryInterface(rightList));
      if (!parent)
        return NS_ERROR_NULL_POINTER;

      nsIContent *child = parent->GetChildAt(theOffset);
      while (child)
      {
        childToMove = do_QueryInterface(child);
        res = mHTMLEditor->MoveNode(childToMove, leftList, -1);
        if (NS_FAILED(res))
          return res;

        child = parent->GetChildAt(rightOffset);
      }
    }
    else
    {
      res = MoveBlock(*aLeftBlock, *aRightBlock, leftOffset, rightOffset);
    }
    if (brNode) mHTMLEditor->DeleteNode(brNode);
  }
  
  
  else if (nsEditorUtils::IsDescendantOf(*aRightBlock, *aLeftBlock, &leftOffset))
  {
    
    
    res = nsWSRunObject::ScrubBlockBoundary(mHTMLEditor, aRightBlock, nsWSRunObject::kBlockStart);
    if (NS_FAILED(res)) return res;
    res = nsWSRunObject::ScrubBlockBoundary(mHTMLEditor, aLeftBlock, nsWSRunObject::kBeforeBlock, &leftOffset);
    if (NS_FAILED(res)) return res;
    
    nsCOMPtr<nsIDOMNode> brNode;
    res = CheckForInvisibleBR(*aLeftBlock, kBeforeBlock, address_of(brNode), leftOffset);
    if (NS_FAILED(res)) return res;
    if (bMergeLists)
    {
      res = MoveContents(rightList, leftList, &leftOffset);
    }
    else
    {
      res = MoveBlock(*aLeftBlock, *aRightBlock, leftOffset, rightOffset);
    }
    if (brNode) mHTMLEditor->DeleteNode(brNode);
  }
  else
  {
    
    
    
    
    
    
    res = nsWSRunObject::PrepareToJoinBlocks(mHTMLEditor, *aLeftBlock, *aRightBlock);
    if (NS_FAILED(res)) return res;
    
    nsCOMPtr<nsIDOMNode> brNode;
    res = CheckForInvisibleBR(*aLeftBlock, kBlockEnd, address_of(brNode));
    if (NS_FAILED(res)) return res;
    if (bMergeLists || mHTMLEditor->NodesSameType(*aLeftBlock, *aRightBlock))
    {
      
      nsCOMPtr<nsIDOMNode> parent;
      PRInt32 offset;
      res = JoinNodesSmart(*aLeftBlock, *aRightBlock, address_of(parent), &offset);
      if (NS_SUCCEEDED(res) && bMergeLists)
      {
        nsCOMPtr<nsIDOMNode> newBlock;
        res = ConvertListType(*aRightBlock, address_of(newBlock), existingListStr, NS_LITERAL_STRING("li"));
      }
    }
    else
    {
      
      res = MoveBlock(*aLeftBlock, *aRightBlock, leftOffset, rightOffset);
    }
    if (NS_SUCCEEDED(res) && brNode)
    {
      res = mHTMLEditor->DeleteNode(brNode);
    }
  }
  return res;
}











nsresult
nsHTMLEditRules::MoveBlock(nsIDOMNode *aLeftBlock, nsIDOMNode *aRightBlock, PRInt32 aLeftOffset, PRInt32 aRightOffset)
{
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  nsCOMPtr<nsISupports> isupports;
  
  nsresult res = GetNodesFromPoint(DOMPoint(aRightBlock,aRightOffset), kMakeList, arrayOfNodes, PR_TRUE);
  if (NS_FAILED(res)) return res;
  PRInt32 listCount = arrayOfNodes.Count();
  PRInt32 i;
  for (i=0; i<listCount; i++)
  {
    
    nsIDOMNode* curNode = arrayOfNodes[i];
    if (IsBlockNode(curNode))
    {
      
      res = MoveContents(curNode, aLeftBlock, &aLeftOffset); 
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->DeleteNode(curNode);
    }
    else
    {
      
      res = MoveNodeSmart(curNode, aLeftBlock, &aLeftOffset);
    }
  }
  return res;
}









nsresult
nsHTMLEditRules::MoveNodeSmart(nsIDOMNode *aSource, nsIDOMNode *aDest, PRInt32 *aOffset)
{
  if (!aSource || !aDest || !aOffset) return NS_ERROR_NULL_POINTER;

  nsAutoString tag;
  nsresult res;
  res = mHTMLEditor->GetTagString(aSource, tag);
  if (NS_FAILED(res)) return res;
  ToLowerCase(tag);
  
  if (mHTMLEditor->CanContainTag(aDest, tag))
  {
    
    res = mHTMLEditor->MoveNode(aSource, aDest, *aOffset);
    if (NS_FAILED(res)) return res;
    if (*aOffset != -1) ++(*aOffset);
  }
  else
  {
    
    res = MoveContents(aSource, aDest, aOffset);
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->DeleteNode(aSource);
    if (NS_FAILED(res)) return res;
  }
  return NS_OK;
}









nsresult
nsHTMLEditRules::MoveContents(nsIDOMNode *aSource, nsIDOMNode *aDest, PRInt32 *aOffset)
{
  if (!aSource || !aDest || !aOffset) return NS_ERROR_NULL_POINTER;
  if (aSource == aDest) return NS_ERROR_ILLEGAL_VALUE;
  
  nsCOMPtr<nsIDOMNode> child;
  nsAutoString tag;
  nsresult res;
  aSource->GetFirstChild(getter_AddRefs(child));
  while (child)
  {
    res = MoveNodeSmart(child, aDest, aOffset);
    if (NS_FAILED(res)) return res;
    aSource->GetFirstChild(getter_AddRefs(child));
  }
  return NS_OK;
}


nsresult
nsHTMLEditRules::DeleteNonTableElements(nsIDOMNode *aNode)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;
  nsresult res = NS_OK;
  if (nsHTMLEditUtils::IsTableElementButNotTable(aNode))
  {
    nsCOMPtr<nsIDOMNodeList> children;
    aNode->GetChildNodes(getter_AddRefs(children));
    if (children)
    {
      PRUint32 len;
      children->GetLength(&len);
      if (!len) return NS_OK;
      PRInt32 j;
      for (j=len-1; j>=0; j--)
      {
        nsCOMPtr<nsIDOMNode> node;
        children->Item(j,getter_AddRefs(node));
        res = DeleteNonTableElements(node);
        if (NS_FAILED(res)) return res;

      }
    }
  }
  else
  {
    res = mHTMLEditor->DeleteNode(aNode);
    if (NS_FAILED(res)) return res;
  }
  return res;
}

nsresult
nsHTMLEditRules::DidDeleteSelection(nsISelection *aSelection, 
                                    nsIEditor::EDirection aDir, 
                                    nsresult aResult)
{
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  
  
  nsCOMPtr<nsIDOMNode> startNode;
  PRInt32 startOffset;
  nsresult res = mEditor->GetStartNodeAndOffset(aSelection, address_of(startNode), &startOffset);
  if (NS_FAILED(res)) return res;
  if (!startNode) return NS_ERROR_FAILURE;
  
  
  nsCOMPtr<nsIDOMNode> citeNode;
  res = GetTopEnclosingMailCite(startNode, address_of(citeNode), 
                                mFlags & nsIPlaintextEditor::eEditorPlaintextMask);
  if (NS_FAILED(res)) return res;
  if (citeNode)
  {
    PRBool isEmpty = PR_TRUE, seenBR = PR_FALSE;
    mHTMLEditor->IsEmptyNodeImpl(citeNode, &isEmpty, PR_TRUE, PR_TRUE, PR_FALSE, &seenBR);
    if (isEmpty)
    {
      nsCOMPtr<nsIDOMNode> parent, brNode;
      PRInt32 offset;
      nsEditor::GetNodeLocation(citeNode, address_of(parent), &offset);
      res = mHTMLEditor->DeleteNode(citeNode);
      if (NS_FAILED(res)) return res;
      if (parent && seenBR)
      {
        res = mHTMLEditor->CreateBR(parent, offset, address_of(brNode));
        if (NS_FAILED(res)) return res;
        aSelection->Collapse(parent, offset);
      }
    }
  }
  
  
  return nsTextEditRules::DidDeleteSelection(aSelection, aDir, aResult);
}

nsresult
nsHTMLEditRules::WillMakeList(nsISelection *aSelection, 
                              const nsAString *aListType, 
                              PRBool aEntireList,
                              const nsAString *aBulletType,
                              PRBool *aCancel,
                              PRBool *aHandled,
                              const nsAString *aItemType)
{
  if (!aSelection || !aListType || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }

  nsresult res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;

  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;

  
  nsAutoString itemType;
  if (aItemType) 
    itemType = *aItemType;
  else if (aListType->LowerCaseEqualsLiteral("dl"))
    itemType.AssignLiteral("dd");
  else
    itemType.AssignLiteral("li");
    
  
  
  
  
  
  *aHandled = PR_TRUE;

  res = NormalizeSelection(aSelection);
  if (NS_FAILED(res)) return res;
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  res = GetListActionNodes(arrayOfNodes, aEntireList);
  if (NS_FAILED(res)) return res;
  
  PRInt32 listCount = arrayOfNodes.Count();
  
  
  PRBool bOnlyBreaks = PR_TRUE;
  PRInt32 j;
  for (j=0; j<listCount; j++)
  {
    nsIDOMNode* curNode = arrayOfNodes[j];
    
    if ( (!nsTextEditUtils::IsBreak(curNode)) && (!IsEmptyInline(curNode)) )
    {
      bOnlyBreaks = PR_FALSE;
      break;
    }
  }
  
  
  if (!listCount || bOnlyBreaks) 
  {
    nsCOMPtr<nsIDOMNode> parent, theList, theListItem;
    PRInt32 offset;

    
    if (bOnlyBreaks)
    {
      for (j=0; j<(PRInt32)listCount; j++)
      {
        res = mHTMLEditor->DeleteNode(arrayOfNodes[j]);
        if (NS_FAILED(res)) return res;
      }
    }
    
    
    res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    
    
    res = SplitAsNeeded(aListType, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->CreateNode(*aListType, parent, offset, getter_AddRefs(theList));
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->CreateNode(itemType, theList, 0, getter_AddRefs(theListItem));
    if (NS_FAILED(res)) return res;
    
    mNewBlock = theListItem;
    
    res = aSelection->Collapse(theListItem,0);
    selectionResetter.Abort();  
    *aHandled = PR_TRUE;
    return res;
  }

  
  

  res = LookInsideDivBQandList(arrayOfNodes);
  if (NS_FAILED(res)) return res;                                 

  
  

  listCount = arrayOfNodes.Count();
  nsCOMPtr<nsIDOMNode> curParent;
  nsCOMPtr<nsIDOMNode> curList;
  nsCOMPtr<nsIDOMNode> prevListItem;
  
  PRInt32 i;
  for (i=0; i<listCount; i++)
  {
    
    nsCOMPtr<nsIDOMNode> newBlock;
    nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];
    PRInt32 offset;
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;
  
    
    
    if (curList)
    {
      PRBool bInDifTblElems;
      res = InDifferentTableElements(curList, curNode, &bInDifTblElems);
      if (NS_FAILED(res)) return res;
      if (bInDifTblElems)
        curList = nsnull;
    }
    
    
    if (nsTextEditUtils::IsBreak(curNode)) 
    {
      res = mHTMLEditor->DeleteNode(curNode);
      if (NS_FAILED(res)) return res;
      prevListItem = 0;
      continue;
    }
    
    else if (IsEmptyInline(curNode)) 
    {
      res = mHTMLEditor->DeleteNode(curNode);
      if (NS_FAILED(res)) return res;
      continue;
    }
    
    if (nsHTMLEditUtils::IsList(curNode))
    {
      nsAutoString existingListStr;
      res = mHTMLEditor->GetTagString(curNode, existingListStr);
      ToLowerCase(existingListStr);
      
      if (curList && !nsEditorUtils::IsDescendantOf(curNode, curList))
      {
        
        
        
        
        
        res = mHTMLEditor->MoveNode(curNode, curList, -1);
        if (NS_FAILED(res)) return res;
        res = ConvertListType(curNode, address_of(newBlock), *aListType, itemType);
        if (NS_FAILED(res)) return res;
        res = mHTMLEditor->RemoveBlockContainer(newBlock);
        if (NS_FAILED(res)) return res;
      }
      else
      {
        
        res = ConvertListType(curNode, address_of(newBlock), *aListType, itemType);
        if (NS_FAILED(res)) return res;
        curList = newBlock;
      }
      prevListItem = 0;
      continue;
    }

    if (nsHTMLEditUtils::IsListItem(curNode))
    {
      nsAutoString existingListStr;
      res = mHTMLEditor->GetTagString(curParent, existingListStr);
      ToLowerCase(existingListStr);
      if ( existingListStr != *aListType )
      {
        
        
        
        if (!curList || nsEditorUtils::IsDescendantOf(curNode, curList))
        {
          res = mHTMLEditor->SplitNode(curParent, offset, getter_AddRefs(newBlock));
          if (NS_FAILED(res)) return res;
          nsCOMPtr<nsIDOMNode> p;
          PRInt32 o;
          res = nsEditor::GetNodeLocation(curParent, address_of(p), &o);
          if (NS_FAILED(res)) return res;
          res = mHTMLEditor->CreateNode(*aListType, p, o, getter_AddRefs(curList));
          if (NS_FAILED(res)) return res;
        }
        
        res = mHTMLEditor->MoveNode(curNode, curList, -1);
        if (NS_FAILED(res)) return res;
        
        if (!mHTMLEditor->NodeIsTypeString(curNode,itemType))
        {
          res = mHTMLEditor->ReplaceContainer(curNode, address_of(newBlock), itemType);
          if (NS_FAILED(res)) return res;
        }
      }
      else
      {
        
        
        if (!curList)
          curList = curParent;
        else
        {
          if (curParent != curList)
          {
            
            res = mHTMLEditor->MoveNode(curNode, curList, -1);
            if (NS_FAILED(res)) return res;
          }
        }
        if (!mHTMLEditor->NodeIsTypeString(curNode,itemType))
        {
          res = mHTMLEditor->ReplaceContainer(curNode, address_of(newBlock), itemType);
          if (NS_FAILED(res)) return res;
        }
      }
      nsCOMPtr<nsIDOMElement> curElement = do_QueryInterface(curNode);
      NS_NAMED_LITERAL_STRING(typestr, "type");
      if (aBulletType && !aBulletType->IsEmpty()) {
        res = mHTMLEditor->SetAttribute(curElement, typestr, *aBulletType);
      }
      else {
        res = mHTMLEditor->RemoveAttribute(curElement, typestr);
      }
      if (NS_FAILED(res)) return res;
      continue;
    }
    
    
    
    if (nsHTMLEditUtils::IsDiv(curNode))
    {
      prevListItem = nsnull;
      PRInt32 j=i+1;
      res = GetInnerContent(curNode, arrayOfNodes, &j);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->RemoveContainer(curNode);
      if (NS_FAILED(res)) return res;
      listCount = arrayOfNodes.Count();
      continue;
    }
      
    
    if (!curList)
    {
      res = SplitAsNeeded(aListType, address_of(curParent), &offset);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->CreateNode(*aListType, curParent, offset, getter_AddRefs(curList));
      if (NS_FAILED(res)) return res;
      
      mNewBlock = curList;
      
      prevListItem = 0;
    }
  
    
    nsCOMPtr<nsIDOMNode> listItem;
    if (!nsHTMLEditUtils::IsListItem(curNode))
    {
      if (IsInlineNode(curNode) && prevListItem)
      {
        
        
        res = mHTMLEditor->MoveNode(curNode, prevListItem, -1);
        if (NS_FAILED(res)) return res;
      }
      else
      {
        
        if (nsHTMLEditUtils::IsParagraph(curNode))
        {
          res = mHTMLEditor->ReplaceContainer(curNode, address_of(listItem), itemType);
        }
        else
        {
          res = mHTMLEditor->InsertContainerAbove(curNode, address_of(listItem), itemType);
        }
        if (NS_FAILED(res)) return res;
        if (IsInlineNode(curNode)) 
          prevListItem = listItem;
        else
          prevListItem = nsnull;
      }
    }
    else
    {
      listItem = curNode;
    }
  
    if (listItem)  
    {
      
      res = mHTMLEditor->MoveNode(listItem, curList, -1);
      if (NS_FAILED(res)) return res;
    }
  }

  return res;
}


nsresult
nsHTMLEditRules::WillRemoveList(nsISelection *aSelection, 
                                PRBool aOrdered, 
                                PRBool *aCancel,
                                PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;
  
  nsresult res = NormalizeSelection(aSelection);
  if (NS_FAILED(res)) return res;
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  
  nsCOMArray<nsIDOMRange> arrayOfRanges;
  res = GetPromotedRanges(aSelection, arrayOfRanges, kMakeList);
  if (NS_FAILED(res)) return res;
  
  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  res = GetListActionNodes(arrayOfNodes, PR_FALSE);
  if (NS_FAILED(res)) return res;                                 
                                     
  
  PRInt32 listCount = arrayOfNodes.Count();
  PRInt32 i;
  for (i=listCount-1; i>=0; i--)
  {
    nsIDOMNode* testNode = arrayOfNodes[i];
    if (!mHTMLEditor->IsEditable(testNode))
    {
      arrayOfNodes.RemoveObjectAt(i);
    }
  }
  
  
  listCount = arrayOfNodes.Count();
  
  
  nsCOMPtr<nsIDOMNode> curParent;
  for (i=0; i<listCount; i++)
  {
    
    nsIDOMNode* curNode = arrayOfNodes[i];
    PRInt32 offset;
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;
    
    if (nsHTMLEditUtils::IsListItem(curNode))  
    {
      PRBool bOutOfList;
      do
      {
        res = PopListItem(curNode, &bOutOfList);
        if (NS_FAILED(res)) return res;
      } while (!bOutOfList); 
    }
    else if (nsHTMLEditUtils::IsList(curNode)) 
    {
      res = RemoveListStructure(curNode);
      if (NS_FAILED(res)) return res;
    }
  }
  return res;
}


nsresult
nsHTMLEditRules::WillMakeDefListItem(nsISelection *aSelection, 
                                     const nsAString *aItemType, 
                                     PRBool aEntireList, 
                                     PRBool *aCancel,
                                     PRBool *aHandled)
{
  
  NS_NAMED_LITERAL_STRING(listType, "dl");
  return WillMakeList(aSelection, &listType, aEntireList, nsnull, aCancel, aHandled, aItemType);
}

nsresult
nsHTMLEditRules::WillMakeBasicBlock(nsISelection *aSelection, 
                                    const nsAString *aBlockType, 
                                    PRBool *aCancel,
                                    PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;
  
  nsresult res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;
  
  
  *aCancel = PR_FALSE;
  res = NormalizeSelection(aSelection);
  if (NS_FAILED(res)) return res;
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
  *aHandled = PR_TRUE;
  nsString tString(*aBlockType);

  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  res = GetNodesFromSelection(aSelection, kMakeBasicBlock, arrayOfNodes);
  if (NS_FAILED(res)) return res;

  
  PRInt32 listCount = arrayOfNodes.Count();
  PRInt32 i;
  for (i=listCount-1; i>=0; i--)
  {
    if (!mHTMLEditor->IsEditable(arrayOfNodes[i]))
    {
      arrayOfNodes.RemoveObjectAt(i);
    }
  }
  
  
  listCount = arrayOfNodes.Count();
  
  
  if (ListIsEmptyLine(arrayOfNodes))
  {
    nsCOMPtr<nsIDOMNode> parent, theBlock;
    PRInt32 offset;
    
    
    res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    if (tString.EqualsLiteral("normal") ||
        tString.IsEmpty() ) 
    {
      nsCOMPtr<nsIDOMNode> curBlock = parent;
      if (!IsBlockNode(curBlock))
        curBlock = mHTMLEditor->GetBlockNodeParent(parent);
      nsCOMPtr<nsIDOMNode> curBlockPar;
      if (!curBlock) return NS_ERROR_NULL_POINTER;
      curBlock->GetParentNode(getter_AddRefs(curBlockPar));
      if (nsHTMLEditUtils::IsFormatNode(curBlock))
      {
        
        
        nsCOMPtr<nsIDOMNode> brNode;
        res = mHTMLEditor->GetNextHTMLNode(parent, offset, address_of(brNode));
        if (NS_FAILED(res)) return res;        
        if (brNode && nsTextEditUtils::IsBreak(brNode))
        {
          res = mHTMLEditor->DeleteNode(brNode);
          if (NS_FAILED(res)) return res; 
        }
        
        res = mHTMLEditor->SplitNodeDeep(curBlock, parent, offset, &offset, PR_TRUE);
        if (NS_FAILED(res)) return res;
        
        res = mHTMLEditor->CreateBR(curBlockPar, offset, address_of(brNode));
        if (NS_FAILED(res)) return res;
        
        res = aSelection->Collapse(curBlockPar, offset);
        selectionResetter.Abort();  
        *aHandled = PR_TRUE;
      }
      
    }
    else  
    {   
      
      nsCOMPtr<nsIDOMNode> brNode;
      res = mHTMLEditor->GetNextHTMLNode(parent, offset, address_of(brNode), PR_TRUE);
      if (NS_FAILED(res)) return res;
      if (brNode && nsTextEditUtils::IsBreak(brNode))
      {
        res = mHTMLEditor->DeleteNode(brNode);
        if (NS_FAILED(res)) return res;
      }
      
      res = SplitAsNeeded(aBlockType, address_of(parent), &offset);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->CreateNode(*aBlockType, parent, offset, getter_AddRefs(theBlock));
      if (NS_FAILED(res)) return res;
      
      mNewBlock = theBlock;
      
      for (PRInt32 j = arrayOfNodes.Count() - 1; j >= 0; --j) 
      {
        nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[0];
        res = mHTMLEditor->DeleteNode(curNode);
        if (NS_FAILED(res)) return res;
        res = arrayOfNodes.RemoveObjectAt(0);
        if (NS_FAILED(res)) return res;
      }
      
      res = aSelection->Collapse(theBlock,0);
      selectionResetter.Abort();  
      *aHandled = PR_TRUE;
    }
    return res;    
  }
  else
  {
    
    
    
    if (tString.EqualsLiteral("blockquote"))
      res = MakeBlockquote(arrayOfNodes);
    else if (tString.EqualsLiteral("normal") ||
             tString.IsEmpty() )
      res = RemoveBlockStyle(arrayOfNodes);
    else
      res = ApplyBlockStyle(arrayOfNodes, aBlockType);
    return res;
  }
  return res;
}

nsresult 
nsHTMLEditRules::DidMakeBasicBlock(nsISelection *aSelection,
                                   nsRulesInfo *aInfo, nsresult aResult)
{
  if (!aSelection) return NS_ERROR_NULL_POINTER;
  
  PRBool isCollapsed;
  nsresult res = aSelection->GetIsCollapsed(&isCollapsed);
  if (NS_FAILED(res)) return res;
  if (!isCollapsed) return NS_OK;

  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  res = nsEditor::GetStartNodeAndOffset(aSelection, address_of(parent), &offset);
  if (NS_FAILED(res)) return res;
  res = InsertMozBRIfNeeded(parent);
  return res;
}

nsresult
nsHTMLEditRules::WillIndent(nsISelection *aSelection, PRBool *aCancel, PRBool * aHandled)
{
  PRBool useCSS;
  nsresult res;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);
  
  if (useCSS) {
    res = WillCSSIndent(aSelection, aCancel, aHandled);
  }
  else {
    res = WillHTMLIndent(aSelection, aCancel, aHandled);
  }
  return res;
}

nsresult
nsHTMLEditRules::WillCSSIndent(nsISelection *aSelection, PRBool *aCancel, PRBool * aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  nsresult res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;

  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;

  res = NormalizeSelection(aSelection);
  if (NS_FAILED(res)) return res;
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  nsCOMArray<nsIDOMRange>  arrayOfRanges;
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  
  
  
  
  PRBool bCollapsed;
  nsCOMPtr<nsIDOMNode> liNode;
  res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (bCollapsed) 
  {
    nsCOMPtr<nsIDOMNode> node, block;
    PRInt32 offset;
    nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(node), &offset);
    if (NS_FAILED(res)) return res;
    if (IsBlockNode(node)) 
      block = node;
    else
      block = mHTMLEditor->GetBlockNodeParent(node);
    if (block && nsHTMLEditUtils::IsListItem(block))
      liNode = block;
  }
  
  if (liNode)
  {
    arrayOfNodes.AppendObject(liNode);
  }
  else
  {
    
    
    
    
    res = GetNodesFromSelection(aSelection, kIndent, arrayOfNodes);
    if (NS_FAILED(res)) return res;
  }
  
  NS_NAMED_LITERAL_STRING(quoteType, "blockquote");
  
  if (ListIsEmptyLine(arrayOfNodes))
  {
    nsCOMPtr<nsIDOMNode> parent, theBlock;
    PRInt32 offset;
    nsAutoString quoteType(NS_LITERAL_STRING("div"));
    
    res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    
    res = SplitAsNeeded(&quoteType, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->CreateNode(quoteType, parent, offset, getter_AddRefs(theBlock));
    if (NS_FAILED(res)) return res;
    
    mNewBlock = theBlock;
    RelativeChangeIndentationOfElementNode(theBlock, +1);
    
    for (PRInt32 j = arrayOfNodes.Count() - 1; j >= 0; --j) 
    {
      nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[0];
      res = mHTMLEditor->DeleteNode(curNode);
      if (NS_FAILED(res)) return res;
      res = arrayOfNodes.RemoveObjectAt(0);
      if (NS_FAILED(res)) return res;
    }
    
    res = aSelection->Collapse(theBlock,0);
    selectionResetter.Abort();  
    *aHandled = PR_TRUE;
    return res;
  }
  
  
  
  nsVoidArray transitionList;
  res = MakeTransitionList(arrayOfNodes, transitionList);
  if (NS_FAILED(res)) return res;                                 
  
  
  
  PRInt32 i;
  nsCOMPtr<nsIDOMNode> curParent;
  nsCOMPtr<nsIDOMNode> curQuote;
  nsCOMPtr<nsIDOMNode> curList;
  PRInt32 listCount = arrayOfNodes.Count();
  for (i=0; i<listCount; i++)
  {
    
    nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];

    
    if (!mHTMLEditor->IsEditable(curNode)) continue;

    PRInt32 offset;
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;
    
    
    if (nsHTMLEditUtils::IsList(curParent))
    {
      if (!curList || transitionList[i])
      {
        nsAutoString listTag;
        nsEditor::GetTagString(curParent,listTag);
        ToLowerCase(listTag);
        
        res = SplitAsNeeded(&listTag, address_of(curParent), &offset);
        if (NS_FAILED(res)) return res;
        res = mHTMLEditor->CreateNode(listTag, curParent, offset, getter_AddRefs(curList));
        if (NS_FAILED(res)) return res;
        
        
        mNewBlock = curList;
      }
      
      PRUint32 listLen;
      res = mHTMLEditor->GetLengthOfDOMNode(curList, listLen);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->MoveNode(curNode, curList, listLen);
      if (NS_FAILED(res)) return res;
    }
    
    else 
    {
      if (IsBlockNode(curNode)) {
        RelativeChangeIndentationOfElementNode(curNode, +1);
        curQuote = nsnull;
      }
      else {
        if (!curQuote) 
        {
          NS_NAMED_LITERAL_STRING(divquoteType, "div");
          res = SplitAsNeeded(&divquoteType, address_of(curParent), &offset);
          if (NS_FAILED(res)) return res;
          res = mHTMLEditor->CreateNode(divquoteType, curParent, offset, getter_AddRefs(curQuote));
          if (NS_FAILED(res)) return res;
          RelativeChangeIndentationOfElementNode(curQuote, +1);
          
          mNewBlock = curQuote;
          
        }
        
        
        PRUint32 quoteLen;
        res = mHTMLEditor->GetLengthOfDOMNode(curQuote, quoteLen);
        if (NS_FAILED(res)) return res;
        res = mHTMLEditor->MoveNode(curNode, curQuote, quoteLen);
        if (NS_FAILED(res)) return res;
      }
    }
  }
  return res;
}

nsresult
nsHTMLEditRules::WillHTMLIndent(nsISelection *aSelection, PRBool *aCancel, PRBool * aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  nsresult res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;

  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;

  res = NormalizeSelection(aSelection);
  if (NS_FAILED(res)) return res;
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  
  
  
  
  
  
  nsCOMArray<nsIDOMRange> arrayOfRanges;
  res = GetPromotedRanges(aSelection, arrayOfRanges, kIndent);
  if (NS_FAILED(res)) return res;
  
  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  res = GetNodesForOperation(arrayOfRanges, arrayOfNodes, kIndent);
  if (NS_FAILED(res)) return res;                                 
                                     
  NS_NAMED_LITERAL_STRING(quoteType, "blockquote");

  
  if (ListIsEmptyLine(arrayOfNodes))
  {
    nsCOMPtr<nsIDOMNode> parent, theBlock;
    PRInt32 offset;
    
    
    res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    
    res = SplitAsNeeded(&quoteType, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->CreateNode(quoteType, parent, offset, getter_AddRefs(theBlock));
    if (NS_FAILED(res)) return res;
    
    mNewBlock = theBlock;
    
    for (PRInt32 j = arrayOfNodes.Count() - 1; j >= 0; --j) 
    {
      nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[0];
      res = mHTMLEditor->DeleteNode(curNode);
      if (NS_FAILED(res)) return res;
      res = arrayOfNodes.RemoveObjectAt(0);
      if (NS_FAILED(res)) return res;
    }
    
    res = aSelection->Collapse(theBlock,0);
    selectionResetter.Abort();  
    *aHandled = PR_TRUE;
    return res;
  }

  
  
  PRInt32 i;
  nsCOMPtr<nsIDOMNode> curParent, curQuote, curList, indentedLI, sibling;
  PRInt32 listCount = arrayOfNodes.Count();
  for (i=0; i<listCount; i++)
  {
    
    nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];

    
    if (!mHTMLEditor->IsEditable(curNode)) continue;

    PRInt32 offset;
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;
     
    
    if (nsHTMLEditUtils::IsList(curParent))
    {
      
      
      if (curList)
      {
        sibling = nsnull;
        mHTMLEditor->GetPriorHTMLSibling(curNode, address_of(sibling));
      }
      
      if (!curList || (sibling && sibling != curList) )
      {
        nsAutoString listTag;
        nsEditor::GetTagString(curParent,listTag);
        ToLowerCase(listTag);
        
        res = SplitAsNeeded(&listTag, address_of(curParent), &offset);
        if (NS_FAILED(res)) return res;
        res = mHTMLEditor->CreateNode(listTag, curParent, offset, getter_AddRefs(curList));
        if (NS_FAILED(res)) return res;
        
        
        mNewBlock = curList;
      }
      
      res = mHTMLEditor->MoveNode(curNode, curList, -1);
      if (NS_FAILED(res)) return res;
      
      curQuote = nsnull;
    }
    
    else 
    {
      
      
      
      
      
      
      nsCOMPtr<nsIDOMNode> listitem=IsInListItem(curNode);
      if (listitem)
      {
        if (indentedLI == listitem) continue;  
        res = nsEditor::GetNodeLocation(listitem, address_of(curParent), &offset);
        if (NS_FAILED(res)) return res;
        
        
        if (curList)
        {
          sibling = nsnull;
          mHTMLEditor->GetPriorHTMLSibling(curNode, address_of(sibling));
        }
         
        if (!curList || (sibling && sibling != curList) )
        {
          nsAutoString listTag;
          nsEditor::GetTagString(curParent,listTag);
          ToLowerCase(listTag);
          
          res = SplitAsNeeded(&listTag, address_of(curParent), &offset);
          if (NS_FAILED(res)) return res;
          res = mHTMLEditor->CreateNode(listTag, curParent, offset, getter_AddRefs(curList));
          if (NS_FAILED(res)) return res;
        }
        res = mHTMLEditor->MoveNode(listitem, curList, -1);
        if (NS_FAILED(res)) return res;
        
        indentedLI = listitem;
      }
      
      else
      {
        
        
        
        
        if (curQuote)
        {
          PRBool bInDifTblElems;
          res = InDifferentTableElements(curQuote, curNode, &bInDifTblElems);
          if (NS_FAILED(res)) return res;
          if (bInDifTblElems)
            curQuote = nsnull;
        }
        
        if (!curQuote) 
        {
          res = SplitAsNeeded(&quoteType, address_of(curParent), &offset);
          if (NS_FAILED(res)) return res;
          res = mHTMLEditor->CreateNode(quoteType, curParent, offset, getter_AddRefs(curQuote));
          if (NS_FAILED(res)) return res;
          
          mNewBlock = curQuote;
          
        }
          
        
        res = mHTMLEditor->MoveNode(curNode, curQuote, -1);
        if (NS_FAILED(res)) return res;
        
        curList = nsnull;
      }
    }
  }
  return res;
}


nsresult
nsHTMLEditRules::WillOutdent(nsISelection *aSelection, PRBool *aCancel, PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> rememberedLeftBQ, rememberedRightBQ;
  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);

  res = NormalizeSelection(aSelection);
  if (NS_FAILED(res)) return res;
  
  {
    nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
    
    
    
    
    
    nsCOMArray<nsIDOMNode> arrayOfNodes;
    res = GetNodesFromSelection(aSelection, kOutdent, arrayOfNodes);
    if (NS_FAILED(res)) return res;

    
    

    nsCOMPtr<nsIDOMNode> curBlockQuote, firstBQChild, lastBQChild;
    PRBool curBlockQuoteIsIndentedWithCSS = PR_FALSE;
    PRInt32 listCount = arrayOfNodes.Count();
    PRInt32 i;
    nsCOMPtr<nsIDOMNode> curParent;
    for (i=0; i<listCount; i++)
    {
      
      nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];
      PRInt32 offset;
      res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
      if (NS_FAILED(res)) return res;
      
      
      if (nsHTMLEditUtils::IsBlockquote(curNode)) 
      {
        
        
        if (curBlockQuote)
        {
          res = OutdentPartOfBlock(curBlockQuote, firstBQChild, lastBQChild,
                                   curBlockQuoteIsIndentedWithCSS,
                                   address_of(rememberedLeftBQ),
                                   address_of(rememberedRightBQ));
          if (NS_FAILED(res)) return res;
          curBlockQuote = 0;  firstBQChild = 0;  lastBQChild = 0;
          curBlockQuoteIsIndentedWithCSS = PR_FALSE;
        }
        res = mHTMLEditor->RemoveBlockContainer(curNode);
        if (NS_FAILED(res)) return res;
        continue;
      }
      
      if (nsHTMLEditUtils::IsListItem(curNode)) 
      {
        
        
        
        if (curBlockQuote)
        {
          res = OutdentPartOfBlock(curBlockQuote, firstBQChild, lastBQChild,
                                   curBlockQuoteIsIndentedWithCSS,
                                   address_of(rememberedLeftBQ),
                                   address_of(rememberedRightBQ));
          if (NS_FAILED(res)) return res;
          curBlockQuote = 0;  firstBQChild = 0;  lastBQChild = 0;
          curBlockQuoteIsIndentedWithCSS = PR_FALSE;
        }
        PRBool bOutOfList;
        res = PopListItem(curNode, &bOutOfList);
        if (NS_FAILED(res)) return res;
        continue;
      }
      
      if (curBlockQuote)
      {
        
        if (nsEditorUtils::IsDescendantOf(curNode, curBlockQuote))
        {
          lastBQChild = curNode;
          continue;  
        }
        else
        {
          
          
          
          res = OutdentPartOfBlock(curBlockQuote, firstBQChild, lastBQChild,
                                   curBlockQuoteIsIndentedWithCSS,
                                   address_of(rememberedLeftBQ),
                                   address_of(rememberedRightBQ));
          if (NS_FAILED(res)) return res;
          curBlockQuote = 0;  firstBQChild = 0;  lastBQChild = 0;
          curBlockQuoteIsIndentedWithCSS = PR_FALSE;
          
        }
      }
      
      
      nsCOMPtr<nsIDOMNode> n = curNode;
      nsCOMPtr<nsIDOMNode> tmp;
      curBlockQuoteIsIndentedWithCSS = PR_FALSE;
      
      
      while (!nsTextEditUtils::IsBody(n) &&   
             (nsHTMLEditUtils::IsTable(n) || !nsHTMLEditUtils::IsTableElement(n)))
      {
        n->GetParentNode(getter_AddRefs(tmp));
        n = tmp;
        if (nsHTMLEditUtils::IsBlockquote(n))
        {
          
          curBlockQuote = n;
          firstBQChild  = curNode;
          lastBQChild   = curNode;
          break;
        }
        else if (useCSS)
        {
          nsIAtom* marginProperty = MarginPropertyAtomForIndent(mHTMLEditor->mHTMLCSSUtils, curNode);
          nsAutoString value;
          mHTMLEditor->mHTMLCSSUtils->GetSpecifiedProperty(n, marginProperty, value);
          float f;
          nsIAtom * unit;
          mHTMLEditor->mHTMLCSSUtils->ParseLength(value, &f, &unit);
          if (f > 0)
          {
            curBlockQuote = n;
            firstBQChild  = curNode;
            lastBQChild   = curNode;
            curBlockQuoteIsIndentedWithCSS = PR_TRUE;
            break;
          }
        }
      }

      if (!curBlockQuote)
      {
        
        if (nsHTMLEditUtils::IsList(curParent))  
        {
          if (nsHTMLEditUtils::IsList(curNode))  
          {
            res = mHTMLEditor->RemoveBlockContainer(curNode);
            if (NS_FAILED(res)) return res;
          }
          
        }
        else if (nsHTMLEditUtils::IsList(curNode)) 
        {
          nsCOMPtr<nsIDOMNode> child;
          curNode->GetLastChild(getter_AddRefs(child));
          while (child)
          {
            if (nsHTMLEditUtils::IsListItem(child))
            {
              PRBool bOutOfList;
              res = PopListItem(child, &bOutOfList);
              if (NS_FAILED(res)) return res;
            }
            else if (nsHTMLEditUtils::IsList(child))
            {
              
              
              
              

              res = mHTMLEditor->MoveNode(child, curParent, offset + 1);
              if (NS_FAILED(res)) return res;
            }
            else
            {
              
              res = mHTMLEditor->DeleteNode(child);
              if (NS_FAILED(res)) return res;
            }
            curNode->GetLastChild(getter_AddRefs(child));
          }
          
          res = mHTMLEditor->RemoveBlockContainer(curNode);
          if (NS_FAILED(res)) return res;
        }
        else if (useCSS) {
          RelativeChangeIndentationOfElementNode(curNode, -1);
        }
      }
    }
    if (curBlockQuote)
    {
      
      res = OutdentPartOfBlock(curBlockQuote, firstBQChild, lastBQChild,
                               curBlockQuoteIsIndentedWithCSS,
                               address_of(rememberedLeftBQ),
                               address_of(rememberedRightBQ));
      if (NS_FAILED(res)) return res;
    }
  }
  
  
  if (rememberedLeftBQ || rememberedRightBQ)
  {
    PRBool bCollapsed;
    res = aSelection->GetIsCollapsed(&bCollapsed);
    if (bCollapsed)
    {
      
      nsCOMPtr<nsIDOMNode> sNode;
      PRInt32 sOffset;
      mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(sNode), &sOffset);
      if (rememberedLeftBQ &&
          ((sNode == rememberedLeftBQ) || nsEditorUtils::IsDescendantOf(sNode, rememberedLeftBQ)))
      {
        
        nsEditor::GetNodeLocation(rememberedLeftBQ, address_of(sNode), &sOffset);
        sOffset++;
        aSelection->Collapse(sNode, sOffset);
      }
      
      mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(sNode), &sOffset);
      if (rememberedRightBQ &&
          ((sNode == rememberedRightBQ) || nsEditorUtils::IsDescendantOf(sNode, rememberedRightBQ)))
      {
        
        nsEditor::GetNodeLocation(rememberedRightBQ, address_of(sNode), &sOffset);
        aSelection->Collapse(sNode, sOffset);
      }
    }
  }
  return res;
}








nsresult 
nsHTMLEditRules::RemovePartOfBlock(nsIDOMNode *aBlock, 
                                   nsIDOMNode *aStartChild, 
                                   nsIDOMNode *aEndChild,
                                   nsCOMPtr<nsIDOMNode> *aLeftNode,
                                   nsCOMPtr<nsIDOMNode> *aRightNode)
{
  nsCOMPtr<nsIDOMNode> middleNode;
  nsresult res = SplitBlock(aBlock, aStartChild, aEndChild,
                            aLeftNode, aRightNode,
                            address_of(middleNode));
  if (NS_FAILED(res)) return res;
  

  return mHTMLEditor->RemoveBlockContainer(aBlock);
}

nsresult 
nsHTMLEditRules::SplitBlock(nsIDOMNode *aBlock, 
                            nsIDOMNode *aStartChild, 
                            nsIDOMNode *aEndChild,
                            nsCOMPtr<nsIDOMNode> *aLeftNode,
                            nsCOMPtr<nsIDOMNode> *aRightNode,
                            nsCOMPtr<nsIDOMNode> *aMiddleNode)
{
  if (!aBlock || !aStartChild || !aEndChild)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIDOMNode> startParent, endParent, leftNode, rightNode;
  PRInt32 startOffset, endOffset, offset;
  nsresult res;

  
  res = nsEditor::GetNodeLocation(aStartChild, address_of(startParent), &startOffset);
  if (NS_FAILED(res)) return res;
  
  
  res = mHTMLEditor->SplitNodeDeep(aBlock, startParent, startOffset, &offset, 
                                   PR_TRUE, address_of(leftNode), address_of(rightNode));
  if (NS_FAILED(res)) return res;
  if (rightNode)  aBlock = rightNode;

  
  if (aLeftNode) 
    *aLeftNode = leftNode;

  
  res = nsEditor::GetNodeLocation(aEndChild, address_of(endParent), &endOffset);
  if (NS_FAILED(res)) return res;
  endOffset++;  

  
  res = mHTMLEditor->SplitNodeDeep(aBlock, endParent, endOffset, &offset, 
                                   PR_TRUE, address_of(leftNode), address_of(rightNode));
  if (NS_FAILED(res)) return res;
  if (leftNode)  aBlock = leftNode;
  
  
  if (aRightNode) 
    *aRightNode = rightNode;

  if (aMiddleNode)
    *aMiddleNode = aBlock;

  return NS_OK;
}

nsresult
nsHTMLEditRules::OutdentPartOfBlock(nsIDOMNode *aBlock, 
                                    nsIDOMNode *aStartChild, 
                                    nsIDOMNode *aEndChild,
                                    PRBool aIsBlockIndentedWithCSS,
                                    nsCOMPtr<nsIDOMNode> *aLeftNode,
                                    nsCOMPtr<nsIDOMNode> *aRightNode)
{
  nsCOMPtr<nsIDOMNode> middleNode;
  nsresult res = SplitBlock(aBlock, aStartChild, aEndChild, 
                            aLeftNode,
                            aRightNode,
                            address_of(middleNode));
  if (NS_FAILED(res)) return res;
  if (aIsBlockIndentedWithCSS)
    res = RelativeChangeIndentationOfElementNode(middleNode, -1);
  else
    res = mHTMLEditor->RemoveBlockContainer(middleNode);
  return res;
}





nsresult 
nsHTMLEditRules::ConvertListType(nsIDOMNode *aList, 
                                 nsCOMPtr<nsIDOMNode> *outList,
                                 const nsAString& aListType, 
                                 const nsAString& aItemType) 
{
  if (!aList || !outList) return NS_ERROR_NULL_POINTER;
  *outList = aList;  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> child, temp;
  aList->GetFirstChild(getter_AddRefs(child));
  while (child)
  {
    if (nsHTMLEditUtils::IsListItem(child) && !nsEditor::NodeIsTypeString(child, aItemType))
    {
      res = mHTMLEditor->ReplaceContainer(child, address_of(temp), aItemType);
      if (NS_FAILED(res)) return res;
      child = temp;
    }
    else if (nsHTMLEditUtils::IsList(child) && !nsEditor::NodeIsTypeString(child, aListType))
    {
      res = ConvertListType(child, address_of(temp), aListType, aItemType);
      if (NS_FAILED(res)) return res;
      child = temp;
    }
    child->GetNextSibling(getter_AddRefs(temp));
    child = temp;
  }
  if (!nsEditor::NodeIsTypeString(aList, aListType))
  {
    res = mHTMLEditor->ReplaceContainer(aList, outList, aListType);
  }
  return res;
}







nsresult 
nsHTMLEditRules::CreateStyleForInsertText(nsISelection *aSelection, nsIDOMDocument *aDoc) 
{
  if (!aSelection || !aDoc) return NS_ERROR_NULL_POINTER;
  if (!mHTMLEditor->mTypeInState) return NS_ERROR_NULL_POINTER;
  
  PRBool weDidSometing = PR_FALSE;
  nsCOMPtr<nsIDOMNode> node, tmp;
  PRInt32 offset;
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(node), &offset);
  if (NS_FAILED(res)) return res;
  PropItem *item = nsnull;
  
  
  if (mDidDeleteSelection && 
      ((mTheAction == nsEditor::kOpInsertText ) ||
       (mTheAction == nsEditor::kOpInsertIMEText) ||
       (mTheAction == nsEditor::kOpInsertBreak) ||
       (mTheAction == nsEditor::kOpDeleteSelection)))
  {
    res = ReapplyCachedStyles();
    if (NS_FAILED(res)) return res;
  }
  
  res = ClearCachedStyles();  
  if (NS_FAILED(res)) return res;  

  
  
  PRInt32 j, defcon = mHTMLEditor->mDefaultStyles.Count();
  for (j=0; j<defcon; j++)
  {
    PropItem *propItem = (PropItem*)mHTMLEditor->mDefaultStyles[j];
    if (!propItem) 
      return NS_ERROR_NULL_POINTER;
    PRBool bFirst, bAny, bAll;

    
    
    
    
    
    
    
    nsAutoString curValue;
    res = mHTMLEditor->GetInlinePropertyBase(propItem->tag, &(propItem->attr), nsnull, 
                                             &bFirst, &bAny, &bAll, &curValue, PR_FALSE);
    if (NS_FAILED(res)) return res;
    
    if (!bAny)  
    {
      mHTMLEditor->mTypeInState->SetProp(propItem->tag, propItem->attr, propItem->value);
    }
  }
  
  
  mHTMLEditor->mTypeInState->TakeClearProperty(&item);
  while (item)
  {
    nsCOMPtr<nsIDOMNode> leftNode, rightNode, secondSplitParent, newSelParent, savedBR;
    res = mHTMLEditor->SplitStyleAbovePoint(address_of(node), &offset, item->tag, &item->attr, address_of(leftNode), address_of(rightNode));
    if (NS_FAILED(res)) return res;
    PRBool bIsEmptyNode;
    if (leftNode)
    {
      mHTMLEditor->IsEmptyNode(leftNode, &bIsEmptyNode, PR_FALSE, PR_TRUE);
      if (bIsEmptyNode)
      {
        
        res = mEditor->DeleteNode(leftNode);
        if (NS_FAILED(res)) return res;
      }
    }
    if (rightNode)
    {
      secondSplitParent = mHTMLEditor->GetLeftmostChild(rightNode);
      
      if (!secondSplitParent) secondSplitParent = rightNode;
      if (!mHTMLEditor->IsContainer(secondSplitParent))
      {
        if (nsTextEditUtils::IsBreak(secondSplitParent))
          savedBR = secondSplitParent;

        secondSplitParent->GetParentNode(getter_AddRefs(tmp));
        secondSplitParent = tmp;
      }
      offset = 0;
      res = mHTMLEditor->SplitStyleAbovePoint(address_of(secondSplitParent), &offset, item->tag, &(item->attr), address_of(leftNode), address_of(rightNode));
      if (NS_FAILED(res)) return res;
      
      if (!leftNode) return NS_ERROR_FAILURE;
      newSelParent = mHTMLEditor->GetLeftmostChild(leftNode);
      if (!newSelParent) newSelParent = leftNode;
      
      
      if (savedBR)
      {
        res = mEditor->MoveNode(savedBR, newSelParent, 0);
        if (NS_FAILED(res)) return res;
      }
      mHTMLEditor->IsEmptyNode(rightNode, &bIsEmptyNode, PR_FALSE, PR_TRUE);
      if (bIsEmptyNode)
      {
        
        res = mEditor->DeleteNode(rightNode);
        if (NS_FAILED(res)) return res;
      }
      
      PRInt32 newSelOffset = 0;
      {
        
        
        
        
        nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater, address_of(newSelParent), &newSelOffset);
        res = mHTMLEditor->RemoveStyleInside(leftNode, item->tag, &(item->attr));
        if (NS_FAILED(res)) return res;
      }
      
      node = newSelParent;
      offset = newSelOffset;
    }
    
    delete item;
    mHTMLEditor->mTypeInState->TakeClearProperty(&item);
    weDidSometing = PR_TRUE;
  }
  
  
  PRInt32 relFontSize;
  
  res = mHTMLEditor->mTypeInState->TakeRelativeFontSize(&relFontSize);
  if (NS_FAILED(res)) return res;
  res = mHTMLEditor->mTypeInState->TakeSetProperty(&item);
  if (NS_FAILED(res)) return res;
  
  if (item || relFontSize) 
  {                        
    if (mHTMLEditor->IsTextNode(node))
    {
      
      res = mHTMLEditor->SplitNodeDeep(node, node, offset, &offset);
      if (NS_FAILED(res)) return res;
      node->GetParentNode(getter_AddRefs(tmp));
      node = tmp;
    }
    nsCOMPtr<nsIDOMNode> newNode;
    nsCOMPtr<nsIDOMText> nodeAsText;
    res = aDoc->CreateTextNode(EmptyString(), getter_AddRefs(nodeAsText));
    if (NS_FAILED(res)) return res;
    if (!nodeAsText) return NS_ERROR_NULL_POINTER;
    newNode = do_QueryInterface(nodeAsText);
    res = mHTMLEditor->InsertNode(newNode, node, offset);
    if (NS_FAILED(res)) return res;
    node = newNode;
    offset = 0;
    weDidSometing = PR_TRUE;

    if (relFontSize)
    {
      PRInt32 j, dir;
      
      if (relFontSize > 0) dir=1;
      else dir = -1;
      for (j=0; j<abs(relFontSize); j++)
      {
        res = mHTMLEditor->RelativeFontChangeOnTextNode(dir, nodeAsText, 0, -1);
        if (NS_FAILED(res)) return res;
      }
    }
    
    while (item)
    {
      res = mHTMLEditor->SetInlinePropertyOnNode(node, item->tag, &item->attr, &item->value);
      if (NS_FAILED(res)) return res;
      
      delete item;
      mHTMLEditor->mTypeInState->TakeSetProperty(&item);
    }
  }
  if (weDidSometing)
    return aSelection->Collapse(node, offset);
    
  return res;
}







nsresult 
nsHTMLEditRules::IsEmptyBlock(nsIDOMNode *aNode, 
                              PRBool *outIsEmptyBlock, 
                              PRBool aMozBRDoesntCount,
                              PRBool aListItemsNotEmpty) 
{
  if (!aNode || !outIsEmptyBlock) return NS_ERROR_NULL_POINTER;
  *outIsEmptyBlock = PR_TRUE;
  

  nsCOMPtr<nsIDOMNode> nodeToTest;
  if (IsBlockNode(aNode)) nodeToTest = do_QueryInterface(aNode);



  if (!nodeToTest) return NS_ERROR_NULL_POINTER;
  return mHTMLEditor->IsEmptyNode(nodeToTest, outIsEmptyBlock,
                     aMozBRDoesntCount, aListItemsNotEmpty);
}


nsresult
nsHTMLEditRules::WillAlign(nsISelection *aSelection, 
                           const nsAString *alignType, 
                           PRBool *aCancel,
                           PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }

  nsresult res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;

  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;

  res = NormalizeSelection(aSelection);
  if (NS_FAILED(res)) return res;
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);

  
  
  
  
  *aHandled = PR_TRUE;
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  res = GetNodesFromSelection(aSelection, kAlign, arrayOfNodes);
  if (NS_FAILED(res)) return res;

  
  
  PRBool emptyDiv = PR_FALSE;
  PRInt32 listCount = arrayOfNodes.Count();
  if (!listCount) emptyDiv = PR_TRUE;
  if (listCount == 1)
  {
    nsCOMPtr<nsIDOMNode> theNode = arrayOfNodes[0];

    if (nsHTMLEditUtils::SupportsAlignAttr(theNode))
    {
      
      
      
      
      nsCOMPtr<nsIDOMElement> theElem = do_QueryInterface(theNode);
      res = AlignBlock(theElem, alignType, PR_TRUE);
      if (NS_FAILED(res)) return res;
      return NS_OK;
    }

    if (nsTextEditUtils::IsBreak(theNode))
    {
      
      
      
      
      
      
      
      
      
      
      
      

      nsCOMPtr<nsIDOMNode> parent;
      PRInt32 offset;
      res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(parent), &offset);

      if (!nsHTMLEditUtils::IsTableElement(parent) || nsHTMLEditUtils::IsTableCellOrCaption(parent))
        emptyDiv = PR_TRUE;
    }
  }
  if (emptyDiv)
  {
    PRInt32 offset;
    nsCOMPtr<nsIDOMNode> brNode, parent, theDiv, sib;
    NS_NAMED_LITERAL_STRING(divType, "div");
    res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    res = SplitAsNeeded(&divType, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    
    
    res = mHTMLEditor->GetNextHTMLNode(parent, offset, address_of(brNode));
    if (NS_FAILED(res)) return res;
    if (brNode && nsTextEditUtils::IsBreak(brNode))
    {
      
      
      
      res = mHTMLEditor->GetNextHTMLSibling(parent, offset, address_of(sib));
      if (NS_FAILED(res)) return res;
      if (!IsBlockNode(sib))
      {
        res = mHTMLEditor->DeleteNode(brNode);
        if (NS_FAILED(res)) return res;
      }
    }
    res = mHTMLEditor->CreateNode(divType, parent, offset, getter_AddRefs(theDiv));
    if (NS_FAILED(res)) return res;
    
    mNewBlock = theDiv;
    
    nsCOMPtr<nsIDOMElement> divElem = do_QueryInterface(theDiv);
    res = AlignBlock(divElem, alignType, PR_TRUE);
    if (NS_FAILED(res)) return res;
    *aHandled = PR_TRUE;
    
    res = CreateMozBR(theDiv, 0, address_of(brNode));
    if (NS_FAILED(res)) return res;
    res = aSelection->Collapse(theDiv, 0);
    selectionResetter.Abort();  
    return res;
  }

  
  

  nsVoidArray transitionList;
  res = MakeTransitionList(arrayOfNodes, transitionList);
  if (NS_FAILED(res)) return res;                                 

  
  

  PRInt32 i;
  nsCOMPtr<nsIDOMNode> curParent;
  nsCOMPtr<nsIDOMNode> curDiv;
  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);
  for (i=0; i<listCount; i++)
  {
    
    nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];
    PRInt32 offset;
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;

    
    
    
    
    if (nsHTMLEditUtils::SupportsAlignAttr(curNode))
    {
      nsCOMPtr<nsIDOMElement> curElem = do_QueryInterface(curNode);
      res = AlignBlock(curElem, alignType, PR_FALSE);
      if (NS_FAILED(res)) return res;
      
      curDiv = 0;
      continue;
    }

    
    
    if (nsEditor::IsTextNode(curNode) &&
       ((nsHTMLEditUtils::IsTableElement(curParent) && !nsHTMLEditUtils::IsTableCellOrCaption(curParent)) ||
        nsHTMLEditUtils::IsList(curParent)))
      continue;

    
    
    
    if ( nsHTMLEditUtils::IsListItem(curNode)
         || nsHTMLEditUtils::IsList(curNode))
    {
      res = RemoveAlignment(curNode, *alignType, PR_TRUE);
      if (NS_FAILED(res)) return res;
      if (useCSS) {
        nsCOMPtr<nsIDOMElement> curElem = do_QueryInterface(curNode);
        NS_NAMED_LITERAL_STRING(attrName, "align");
        PRInt32 count;
        mHTMLEditor->mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(curNode, nsnull,
                                                                &attrName, alignType,
                                                                &count, PR_FALSE);
        curDiv = 0;
        continue;
      }
      else if (nsHTMLEditUtils::IsList(curParent)) {
        
        
        res = AlignInnerBlocks(curNode, alignType);
        if (NS_FAILED(res)) return res;
        curDiv = 0;
        continue;
      }
      
    }      

    
    
    if (!curDiv || transitionList[i])
    {
      NS_NAMED_LITERAL_STRING(divType, "div");
      res = SplitAsNeeded(&divType, address_of(curParent), &offset);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->CreateNode(divType, curParent, offset, getter_AddRefs(curDiv));
      if (NS_FAILED(res)) return res;
      
      mNewBlock = curDiv;
      
      nsCOMPtr<nsIDOMElement> divElem = do_QueryInterface(curDiv);
      res = AlignBlock(divElem, alignType, PR_TRUE);



      
    }

    
    res = mHTMLEditor->MoveNode(curNode, curDiv, -1);
    if (NS_FAILED(res)) return res;
  }

  return res;
}





nsresult
nsHTMLEditRules::AlignInnerBlocks(nsIDOMNode *aNode, const nsAString *alignType)
{
  if (!aNode || !alignType) return NS_ERROR_NULL_POINTER;
  nsresult res;
  
  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  nsTableCellAndListItemFunctor functor;
  nsDOMIterator iter;
  res = iter.Init(aNode);
  if (NS_FAILED(res)) return res;
  res = iter.AppendList(functor, arrayOfNodes);
  if (NS_FAILED(res)) return res;
  
  
  PRInt32 listCount = arrayOfNodes.Count();
  PRInt32 j;

  for (j = 0; j < listCount; j++)
  {
    nsIDOMNode* node = arrayOfNodes[0];
    res = AlignBlockContents(node, alignType);
    if (NS_FAILED(res)) return res;
    arrayOfNodes.RemoveObjectAt(0);
  }

  return res;  
}





nsresult
nsHTMLEditRules::AlignBlockContents(nsIDOMNode *aNode, const nsAString *alignType)
{
  if (!aNode || !alignType) return NS_ERROR_NULL_POINTER;
  nsresult res;
  nsCOMPtr <nsIDOMNode> firstChild, lastChild, divNode;
  
  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);

  res = mHTMLEditor->GetFirstEditableChild(aNode, address_of(firstChild));
  if (NS_FAILED(res)) return res;
  res = mHTMLEditor->GetLastEditableChild(aNode, address_of(lastChild));
  if (NS_FAILED(res)) return res;
  NS_NAMED_LITERAL_STRING(attr, "align");
  if (!firstChild)
  {
    
  }
  else if ((firstChild==lastChild) && nsHTMLEditUtils::IsDiv(firstChild))
  {
    
    
    nsCOMPtr<nsIDOMElement> divElem = do_QueryInterface(firstChild);
    if (useCSS) {
      res = mHTMLEditor->SetAttributeOrEquivalent(divElem, attr, *alignType, PR_FALSE); 
    }
    else {
      res = mHTMLEditor->SetAttribute(divElem, attr, *alignType);
    }
    if (NS_FAILED(res)) return res;
  }
  else
  {
    
    res = mHTMLEditor->CreateNode(NS_LITERAL_STRING("div"), aNode, 0, getter_AddRefs(divNode));
    if (NS_FAILED(res)) return res;
    
    nsCOMPtr<nsIDOMElement> divElem = do_QueryInterface(divNode);
    if (useCSS) {
      res = mHTMLEditor->SetAttributeOrEquivalent(divElem, attr, *alignType, PR_FALSE); 
    }
    else {
      res = mHTMLEditor->SetAttribute(divElem, attr, *alignType);
    }
    if (NS_FAILED(res)) return res;
    
    while (lastChild && (lastChild != divNode))
    {
      res = mHTMLEditor->MoveNode(lastChild, divNode, 0);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->GetLastEditableChild(aNode, address_of(lastChild));
      if (NS_FAILED(res)) return res;
    }
  }
  return res;
}





nsresult
nsHTMLEditRules::CheckForEmptyBlock(nsIDOMNode *aStartNode, 
                                    nsIDOMNode *aBodyNode,
                                    nsISelection *aSelection,
                                    PRBool *aHandled)
{
  
  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> block, emptyBlock;
  if (IsBlockNode(aStartNode)) 
    block = aStartNode;
  else
    block = mHTMLEditor->GetBlockNodeParent(aStartNode);
  PRBool bIsEmptyNode;
  if (block != aBodyNode)  
  {
    res = mHTMLEditor->IsEmptyNode(block, &bIsEmptyNode, PR_TRUE, PR_FALSE);
    if (NS_FAILED(res)) return res;
    while (bIsEmptyNode && !nsHTMLEditUtils::IsTableElement(block) && (block != aBodyNode))
    {
      emptyBlock = block;
      block = mHTMLEditor->GetBlockNodeParent(emptyBlock);
      res = mHTMLEditor->IsEmptyNode(block, &bIsEmptyNode, PR_TRUE, PR_FALSE);
      if (NS_FAILED(res)) return res;
    }
  }
  
  if (emptyBlock)
  {
    nsCOMPtr<nsIDOMNode> blockParent;
    PRInt32 offset;
    res = nsEditor::GetNodeLocation(emptyBlock, address_of(blockParent), &offset);
    if (NS_FAILED(res)) return res;
    if (!blockParent || offset < 0) return NS_ERROR_FAILURE;

    if (nsHTMLEditUtils::IsListItem(emptyBlock))
    {
      
      PRBool bIsFirst;
      res = mHTMLEditor->IsFirstEditableChild(emptyBlock, &bIsFirst);
      if (NS_FAILED(res)) return res;
      if (bIsFirst)
      {
        nsCOMPtr<nsIDOMNode> listParent;
        PRInt32 listOffset;
        res = nsEditor::GetNodeLocation(blockParent, address_of(listParent), &listOffset);
        if (NS_FAILED(res)) return res;
        if (!listParent || listOffset < 0) return NS_ERROR_FAILURE;
        
        if (!nsHTMLEditUtils::IsList(listParent))
        {
          
          nsCOMPtr<nsIDOMNode> brNode;
          res = mHTMLEditor->CreateBR(listParent, listOffset, address_of(brNode));
          if (NS_FAILED(res)) return res;
          
          res = aSelection->Collapse(listParent, listOffset);
          if (NS_FAILED(res)) return res;
        }
        
      }
    }
    else
    {
      
      res = aSelection->Collapse(blockParent, offset+1);
      if (NS_FAILED(res)) return res;
    }
    res = mHTMLEditor->DeleteNode(emptyBlock);
    *aHandled = PR_TRUE;
  }
  return res;
}

nsresult
nsHTMLEditRules::CheckForInvisibleBR(nsIDOMNode *aBlock, 
                                     BRLocation aWhere, 
                                     nsCOMPtr<nsIDOMNode> *outBRNode,
                                     PRInt32 aOffset)
{
  if (!aBlock || !outBRNode) return NS_ERROR_NULL_POINTER;
  *outBRNode = nsnull;

  nsCOMPtr<nsIDOMNode> testNode;
  PRInt32 testOffset = 0;
  PRBool runTest = PR_FALSE;

  if (aWhere == kBlockEnd)
  {
    nsCOMPtr<nsIDOMNode> rightmostNode;
    rightmostNode = mHTMLEditor->GetRightmostChild(aBlock, PR_TRUE); 

    if (rightmostNode)
    {
      nsCOMPtr<nsIDOMNode> nodeParent;
      PRInt32 nodeOffset;

      if (NS_SUCCEEDED(nsEditor::GetNodeLocation(rightmostNode,
                                                 address_of(nodeParent), 
                                                 &nodeOffset)))
      {
        runTest = PR_TRUE;
        testNode = nodeParent;
        
        testOffset = nodeOffset + 1;
      }
    }
  }
  else if (aOffset)
  {
    runTest = PR_TRUE;
    testNode = aBlock;
    
    testOffset = aOffset;
  }

  if (runTest)
  {
    nsWSRunObject wsTester(mHTMLEditor, testNode, testOffset);
    if (nsWSRunObject::eBreak == wsTester.mStartReason)
    {
      *outBRNode = wsTester.mStartReasonNode;
    }
  }

  return NS_OK;
}










nsresult
nsHTMLEditRules::GetInnerContent(nsIDOMNode *aNode, nsCOMArray<nsIDOMNode> &outArrayOfNodes, 
                                 PRInt32 *aIndex, PRBool aList, PRBool aTbl)
{
  if (!aNode || !aIndex) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> node;
  
  nsresult res = mHTMLEditor->GetFirstEditableChild(aNode, address_of(node));
  while (NS_SUCCEEDED(res) && node)
  {
    if (  ( aList && (nsHTMLEditUtils::IsList(node)     || 
                      nsHTMLEditUtils::IsListItem(node) ) )
       || ( aTbl && nsHTMLEditUtils::IsTableElement(node) )  )
    {
      res = GetInnerContent(node, outArrayOfNodes, aIndex, aList, aTbl);
      if (NS_FAILED(res)) return res;
    }
    else
    {
      outArrayOfNodes.InsertObjectAt(node, *aIndex);
      (*aIndex)++;
    }
    nsCOMPtr<nsIDOMNode> tmp;
    res = node->GetNextSibling(getter_AddRefs(tmp));
    node = tmp;
  }

  return res;
}





PRBool
nsHTMLEditRules::ExpandSelectionForDeletion(nsISelection *aSelection)
{
  if (!aSelection) 
    return NS_ERROR_NULL_POINTER;
  
  
  PRBool bCollapsed;
  nsresult res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (bCollapsed) return res;

  PRInt32 rangeCount;
  res = aSelection->GetRangeCount(&rangeCount);
  if (NS_FAILED(res)) return res;
  
  
  if (rangeCount != 1) return NS_OK;
  
  
  nsCOMPtr<nsIDOMRange> range;
  res = aSelection->GetRangeAt(0, getter_AddRefs(range));
  if (NS_FAILED(res)) return res;
  if (!range) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> selStartNode, selEndNode, selCommon;
  PRInt32 selStartOffset, selEndOffset;
  
  res = range->GetStartContainer(getter_AddRefs(selStartNode));
  if (NS_FAILED(res)) return res;
  res = range->GetStartOffset(&selStartOffset);
  if (NS_FAILED(res)) return res;
  res = range->GetEndContainer(getter_AddRefs(selEndNode));
  if (NS_FAILED(res)) return res;
  res = range->GetEndOffset(&selEndOffset);
  if (NS_FAILED(res)) return res;

  
  res = range->GetCommonAncestorContainer(getter_AddRefs(selCommon));
  if (NS_FAILED(res)) return res;
  if (!IsBlockNode(selCommon))
    selCommon = nsHTMLEditor::GetBlockNodeParent(selCommon);

  
  PRBool stillLooking = PR_TRUE;
  nsCOMPtr<nsIDOMNode> visNode, firstBRParent;
  PRInt32 visOffset=0, firstBROffset=0;
  PRInt16 wsType;
  nsIDOMElement *rootElement = mHTMLEditor->GetRoot();
  if (!rootElement)
    return NS_ERROR_FAILURE;

  
  if ((selStartNode!=selCommon) && (selStartNode!=rootElement))
  {
    while (stillLooking)
    {
      nsWSRunObject wsObj(mHTMLEditor, selStartNode, selStartOffset);
      res = wsObj.PriorVisibleNode(selStartNode, selStartOffset, address_of(visNode), &visOffset, &wsType);
      if (NS_FAILED(res)) return res;
      if (wsType == nsWSRunObject::eThisBlock)
      {
        
        
        if ( nsHTMLEditUtils::IsTableElement(wsObj.mStartReasonNode) ||
            (selCommon == wsObj.mStartReasonNode)                    ||
            (rootElement == wsObj.mStartReasonNode) )
        {
          stillLooking = PR_FALSE;
        }
        else
        { 
          nsEditor::GetNodeLocation(wsObj.mStartReasonNode, address_of(selStartNode), &selStartOffset);
        }
      }
      else
      {
        stillLooking = PR_FALSE;
      }
    }
  }
  
  stillLooking = PR_TRUE;
  
  if ((selEndNode!=selCommon) && (selEndNode!=rootElement))
  {
    while (stillLooking)
    {
      nsWSRunObject wsObj(mHTMLEditor, selEndNode, selEndOffset);
      res = wsObj.NextVisibleNode(selEndNode, selEndOffset, address_of(visNode), &visOffset, &wsType);
      if (NS_FAILED(res)) return res;
      if (wsType == nsWSRunObject::eBreak)
      {
        if (mHTMLEditor->IsVisBreak(wsObj.mEndReasonNode))
        {
          stillLooking = PR_FALSE;
        }
        else
        { 
          if (!firstBRParent)
          {
            firstBRParent = selEndNode;
            firstBROffset = selEndOffset;
          }
          nsEditor::GetNodeLocation(wsObj.mEndReasonNode, address_of(selEndNode), &selEndOffset);
          ++selEndOffset;
        }
      }
      else if (wsType == nsWSRunObject::eThisBlock)
      {
        
        
        if ( nsHTMLEditUtils::IsTableElement(wsObj.mEndReasonNode) ||
            (selCommon == wsObj.mEndReasonNode)                    ||
            (rootElement == wsObj.mEndReasonNode) )
        {
          stillLooking = PR_FALSE;
        }
        else
        { 
          nsEditor::GetNodeLocation(wsObj.mEndReasonNode, address_of(selEndNode), &selEndOffset);
          ++selEndOffset;
        }
       }
      else
      {
        stillLooking = PR_FALSE;
      }
    }
  }
  
  aSelection->Collapse(selStartNode, selStartOffset);
  
  
  
  
  PRBool doEndExpansion = PR_TRUE;
  if (firstBRParent)
  {
    
    nsCOMPtr<nsIDOMNode> brBlock = firstBRParent;
    if (!IsBlockNode(brBlock))
      brBlock = nsHTMLEditor::GetBlockNodeParent(brBlock);
    PRBool nodeBefore=PR_FALSE, nodeAfter=PR_FALSE;
    
    
    nsCOMPtr<nsIDOMRange> range = do_CreateInstance("@mozilla.org/content/range;1");
    if (!range) return NS_ERROR_NULL_POINTER;
    res = range->SetStart(selStartNode, selStartOffset);
    if (NS_FAILED(res)) return res;
    res = range->SetEnd(selEndNode, selEndOffset);
    if (NS_FAILED(res)) return res;
    
    
    nsCOMPtr<nsIContent> brContentBlock = do_QueryInterface(brBlock);
    res = mHTMLEditor->sRangeHelper->CompareNodeToRange(brContentBlock, range, &nodeBefore, &nodeAfter);
    
    
    if (nodeBefore || nodeAfter)
      doEndExpansion = PR_FALSE;
  }
  if (doEndExpansion)
  {
    res = aSelection->Extend(selEndNode, selEndOffset);
  }
  else
  {
    
    res = aSelection->Extend(firstBRParent, firstBROffset);
  }
  
  return res;
}

#ifdef XXX_DEAD_CODE



PRBool
nsHTMLEditRules::AtStartOfBlock(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMNode *aBlock)
{
  nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(aNode);
  if (nodeAsText && aOffset) return PR_FALSE;  
  
  nsCOMPtr<nsIDOMNode> priorNode;
  nsresult  res = mHTMLEditor->GetPriorHTMLNode(aNode, aOffset, address_of(priorNode));
  if (NS_FAILED(res)) return PR_TRUE;
  if (!priorNode) return PR_TRUE;
  nsCOMPtr<nsIDOMNode> blockParent = mHTMLEditor->GetBlockNodeParent(priorNode);
  if (blockParent && (blockParent == aBlock)) return PR_FALSE;
  return PR_TRUE;
}





PRBool
nsHTMLEditRules::AtEndOfBlock(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMNode *aBlock)
{
  nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(aNode);
  if (nodeAsText)   
  {
    PRUint32 strLength;
    nodeAsText->GetLength(&strLength);
    if ((PRInt32)strLength > aOffset) return PR_FALSE;  
  }
  nsCOMPtr<nsIDOMNode> nextNode;
  nsresult  res = mHTMLEditor->GetNextHTMLNode(aNode, aOffset, address_of(nextNode));
  if (NS_FAILED(res)) return PR_TRUE;
  if (!nextNode) return PR_TRUE;
  nsCOMPtr<nsIDOMNode> blockParent = mHTMLEditor->GetBlockNodeParent(nextNode);
  if (blockParent && (blockParent == aBlock)) return PR_FALSE;
  return PR_TRUE;
}





nsresult
nsHTMLEditRules::CreateMozDiv(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outDiv)
{
  if (!inParent || !outDiv) return NS_ERROR_NULL_POINTER;
  nsAutoString divType= "div";
  *outDiv = nsnull;
  nsresult res = mHTMLEditor->CreateNode(divType, inParent, inOffset, getter_AddRefs(*outDiv));
  if (NS_FAILED(res)) return res;
  
  nsCOMPtr<nsIDOMElement> mozDivElem = do_QueryInterface(*outDiv);
  res = mHTMLEditor->SetAttribute(mozDivElem, "type", "_moz");
  if (NS_FAILED(res)) return res;
  res = AddTrailerBR(*outDiv);
  return res;
}
#endif    









nsresult 
nsHTMLEditRules::NormalizeSelection(nsISelection *inSelection)
{
  if (!inSelection) return NS_ERROR_NULL_POINTER;

  
  PRBool bCollapsed;
  nsresult res = inSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (bCollapsed) return res;

  PRInt32 rangeCount;
  res = inSelection->GetRangeCount(&rangeCount);
  if (NS_FAILED(res)) return res;
  
  
  if (rangeCount != 1) return NS_OK;
  
  nsCOMPtr<nsIDOMRange> range;
  res = inSelection->GetRangeAt(0, getter_AddRefs(range));
  if (NS_FAILED(res)) return res;
  if (!range) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRInt32 startOffset, endOffset;
  nsCOMPtr<nsIDOMNode> newStartNode, newEndNode;
  PRInt32 newStartOffset, newEndOffset;
  
  res = range->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(res)) return res;
  res = range->GetStartOffset(&startOffset);
  if (NS_FAILED(res)) return res;
  res = range->GetEndContainer(getter_AddRefs(endNode));
  if (NS_FAILED(res)) return res;
  res = range->GetEndOffset(&endOffset);
  if (NS_FAILED(res)) return res;
  
  
  newStartNode = startNode; 
  newStartOffset = startOffset;
  newEndNode = endNode; 
  newEndOffset = endOffset;
  
  
  nsCOMPtr<nsIDOMNode> someNode;
  PRInt32 offset;
  PRInt16 wsType;

  
  nsWSRunObject wsEndObj(mHTMLEditor, endNode, endOffset);
  
  
  res = wsEndObj.PriorVisibleNode(endNode, endOffset, address_of(someNode), &offset, &wsType);
  if (NS_FAILED(res)) return res;
  if ((wsType != nsWSRunObject::eText) && (wsType != nsWSRunObject::eNormalWS))
  {
    
    
    if (wsEndObj.mStartReason == nsWSRunObject::eOtherBlock) 
    {
      
      nsCOMPtr<nsIDOMNode> child = mHTMLEditor->GetRightmostChild(wsEndObj.mStartReasonNode, PR_TRUE);
      if (child)
      {
        res = nsEditor::GetNodeLocation(child, address_of(newEndNode), &newEndOffset);
        if (NS_FAILED(res)) return res;
        ++newEndOffset; 
      }
      
    }
    else if (wsEndObj.mStartReason == nsWSRunObject::eThisBlock)
    {
      
      nsCOMPtr<nsIDOMNode> child;
      res = mHTMLEditor->GetPriorHTMLNode(endNode, endOffset, address_of(child));
      if (child)
      {
        res = nsEditor::GetNodeLocation(child, address_of(newEndNode), &newEndOffset);
        if (NS_FAILED(res)) return res;
        ++newEndOffset; 
      }
      
    }
    else if (wsEndObj.mStartReason == nsWSRunObject::eBreak)
    {                                       
      
      res = nsEditor::GetNodeLocation(wsEndObj.mStartReasonNode, address_of(newEndNode), &newEndOffset);
      if (NS_FAILED(res)) return res;
    }
  }
  
  
  
  nsWSRunObject wsStartObj(mHTMLEditor, startNode, startOffset);
  
  
  res = wsStartObj.NextVisibleNode(startNode, startOffset, address_of(someNode), &offset, &wsType);
  if (NS_FAILED(res)) return res;
  if ((wsType != nsWSRunObject::eText) && (wsType != nsWSRunObject::eNormalWS))
  {
    
    
    if (wsStartObj.mEndReason == nsWSRunObject::eOtherBlock) 
    {
      
      nsCOMPtr<nsIDOMNode> child = mHTMLEditor->GetLeftmostChild(wsStartObj.mEndReasonNode, PR_TRUE);
      if (child)
      {
        res = nsEditor::GetNodeLocation(child, address_of(newStartNode), &newStartOffset);
        if (NS_FAILED(res)) return res;
      }
      
    }
    else if (wsStartObj.mEndReason == nsWSRunObject::eThisBlock)
    {
      
      nsCOMPtr<nsIDOMNode> child;
      res = mHTMLEditor->GetNextHTMLNode(startNode, startOffset, address_of(child));
      if (child)
      {
        res = nsEditor::GetNodeLocation(child, address_of(newStartNode), &newStartOffset);
        if (NS_FAILED(res)) return res;
      }
      
    }
    else if (wsStartObj.mEndReason == nsWSRunObject::eBreak)
    {                                       
      
      res = nsEditor::GetNodeLocation(wsStartObj.mEndReasonNode, address_of(newStartNode), &newStartOffset);
      if (NS_FAILED(res)) return res;
      ++newStartOffset; 
    }
  }
  
  
  
  
  
  
  
  
  
  PRInt16 comp;
  comp = mHTMLEditor->sRangeHelper->ComparePoints(startNode, startOffset, newEndNode, newEndOffset);
  if (comp == 1) return NS_OK;  
  comp = mHTMLEditor->sRangeHelper->ComparePoints(newStartNode, newStartOffset, endNode, endOffset);
  if (comp == 1) return NS_OK;  
  
  
  inSelection->Collapse(newStartNode, newStartOffset);
  inSelection->Extend(newEndNode, newEndOffset);
  return NS_OK;
}





nsresult
nsHTMLEditRules::GetPromotedPoint(RulesEndpoint aWhere, nsIDOMNode *aNode, PRInt32 aOffset, 
                                  PRInt32 actionID, nsCOMPtr<nsIDOMNode> *outNode, PRInt32 *outOffset)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> nearNode, node = aNode;
  nsCOMPtr<nsIDOMNode> parent = aNode;
  PRInt32 pOffset, offset = aOffset;
  
  
  *outNode = node;
  *outOffset = offset;

  
  if (actionID == kInsertText)
  {
    PRBool isSpace, isNBSP; 
    nsCOMPtr<nsIDOMNode> temp;   
    
    
    
    if (aWhere == kStart)
    {
      do
      {
        res = mHTMLEditor->IsPrevCharWhitespace(node, offset, &isSpace, &isNBSP, address_of(temp), &offset);
        if (NS_FAILED(res)) return res;
        if (isSpace || isNBSP) node = temp;
        else break;
      } while (node);
  
      *outNode = node;
      *outOffset = offset;
    }
    else if (aWhere == kEnd)
    {
      do
      {
        res = mHTMLEditor->IsNextCharWhitespace(node, offset, &isSpace, &isNBSP, address_of(temp), &offset);
        if (NS_FAILED(res)) return res;
        if (isSpace || isNBSP) node = temp;
        else break;
      } while (node);
  
      *outNode = node;
      *outOffset = offset;
    }
    return res;
  }
  
  
  
  if (aWhere == kStart)
  {
    
    if (nsEditor::IsTextNode(aNode))  
    {
      res = nsEditor::GetNodeLocation(aNode, address_of(node), &offset);
      if (NS_FAILED(res)) return res;
    }

    
    
    nsCOMPtr<nsIDOMNode> priorNode;
    res = mHTMLEditor->GetPriorHTMLNode(node, offset, address_of(priorNode), PR_TRUE);
      
    while (priorNode && NS_SUCCEEDED(res))
    {
      if (mHTMLEditor->IsVisBreak(priorNode))
        break;
      if (IsBlockNode(priorNode))
        break;
      res = nsEditor::GetNodeLocation(priorNode, address_of(node), &offset);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->GetPriorHTMLNode(node, offset, address_of(priorNode), PR_TRUE);
      if (NS_FAILED(res)) return res;
    }
    
        
    
    
    res = mHTMLEditor->GetPriorHTMLNode(node, offset, address_of(nearNode), PR_TRUE);
    if (NS_FAILED(res)) return res;
    while (!nearNode && !nsTextEditUtils::IsBody(node))
    {
      
      
      
      
      if ((actionID == kOutdent) && nsHTMLEditUtils::IsBlockquote(node))
        break;

      res = nsEditor::GetNodeLocation(node, address_of(parent), &pOffset);
      if (NS_FAILED(res)) return res;
      node = parent;
      offset = pOffset;
      res = mHTMLEditor->GetPriorHTMLNode(node, offset, address_of(nearNode), PR_TRUE);
      if (NS_FAILED(res)) return res;
    } 
    *outNode = node;
    *outOffset = offset;
    return res;
  }
  
  if (aWhere == kEnd)
  {
    
    if (nsEditor::IsTextNode(aNode))  
    {
      res = nsEditor::GetNodeLocation(aNode, address_of(node), &offset);
      if (NS_FAILED(res)) return res;
      offset++; 
    }

    
    
    nsCOMPtr<nsIDOMNode> nextNode;
    res = mHTMLEditor->GetNextHTMLNode(node, offset, address_of(nextNode), PR_TRUE);
      
    while (nextNode && NS_SUCCEEDED(res))
    {
      if (IsBlockNode(nextNode))
        break;
      res = nsEditor::GetNodeLocation(nextNode, address_of(node), &offset);
      if (NS_FAILED(res)) return res;
      offset++;
      if (mHTMLEditor->IsVisBreak(nextNode))
        break;
      res = mHTMLEditor->GetNextHTMLNode(node, offset, address_of(nextNode), PR_TRUE);
      if (NS_FAILED(res)) return res;
    }
    
    
    
    res = mHTMLEditor->GetNextHTMLNode(node, offset, address_of(nearNode), PR_TRUE);
    if (NS_FAILED(res)) return res;
    while (!nearNode && !nsTextEditUtils::IsBody(node))
    {
      res = nsEditor::GetNodeLocation(node, address_of(parent), &pOffset);
      if (NS_FAILED(res)) return res;
      node = parent;
      offset = pOffset+1;  
      res = mHTMLEditor->GetNextHTMLNode(node, offset, address_of(nearNode), PR_TRUE);
      if (NS_FAILED(res)) return res;
    } 
    *outNode = node;
    *outOffset = offset;
    return res;
  }
  
  return res;
}






nsresult 
nsHTMLEditRules::GetPromotedRanges(nsISelection *inSelection, 
                                   nsCOMArray<nsIDOMRange> &outArrayOfRanges, 
                                   PRInt32 inOperationType)
{
  if (!inSelection) return NS_ERROR_NULL_POINTER;

  PRInt32 rangeCount;
  nsresult res = inSelection->GetRangeCount(&rangeCount);
  if (NS_FAILED(res)) return res;
  
  PRInt32 i;
  nsCOMPtr<nsIDOMRange> selectionRange;
  nsCOMPtr<nsIDOMRange> opRange;

  for (i = 0; i < rangeCount; i++)
  {
    res = inSelection->GetRangeAt(i, getter_AddRefs(selectionRange));
    if (NS_FAILED(res)) return res;

    
    res = selectionRange->CloneRange(getter_AddRefs(opRange));
    if (NS_FAILED(res)) return res;

    
    
    
    
    res = PromoteRange(opRange, inOperationType);
    if (NS_FAILED(res)) return res;
      
    
    outArrayOfRanges.AppendObject(opRange);
  }
  return res;
}






nsresult 
nsHTMLEditRules::PromoteRange(nsIDOMRange *inRange, 
                              PRInt32 inOperationType)
{
  if (!inRange) return NS_ERROR_NULL_POINTER;
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRInt32 startOffset, endOffset;
  
  res = inRange->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(res)) return res;
  res = inRange->GetStartOffset(&startOffset);
  if (NS_FAILED(res)) return res;
  res = inRange->GetEndContainer(getter_AddRefs(endNode));
  if (NS_FAILED(res)) return res;
  res = inRange->GetEndOffset(&endOffset);
  if (NS_FAILED(res)) return res;
  
  
  
  
  
  if ( (startNode == endNode) && (startOffset == endOffset))
  {
    nsCOMPtr<nsIDOMNode> block;
    if (IsBlockNode(startNode)) 
      block = startNode;
    else
      block = mHTMLEditor->GetBlockNodeParent(startNode);
    if (block)
    {
      PRBool bIsEmptyNode = PR_FALSE;
      
      nsIDOMElement *rootElement = mHTMLEditor->GetRoot();
      if (!rootElement) return NS_ERROR_UNEXPECTED;
      nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(rootElement);
      if (block != rootNode)
      {
        
        res = mHTMLEditor->IsEmptyNode(block, &bIsEmptyNode, PR_TRUE, PR_FALSE);
      }
      if (bIsEmptyNode)
      {
        PRUint32 numChildren;
        nsEditor::GetLengthOfDOMNode(block, numChildren); 
        startNode = block;
        endNode = block;
        startOffset = 0;
        endOffset = numChildren;
      }
    }
  }

  
  
  
  
  nsCOMPtr<nsIDOMNode> opStartNode;
  nsCOMPtr<nsIDOMNode> opEndNode;
  PRInt32 opStartOffset, opEndOffset;
  nsCOMPtr<nsIDOMRange> opRange;
  
  res = GetPromotedPoint( kStart, startNode, startOffset, inOperationType, address_of(opStartNode), &opStartOffset);
  if (NS_FAILED(res)) return res;
  res = GetPromotedPoint( kEnd, endNode, endOffset, inOperationType, address_of(opEndNode), &opEndOffset);
  if (NS_FAILED(res)) return res;
  res = inRange->SetStart(opStartNode, opStartOffset);
  if (NS_FAILED(res)) return res;
  res = inRange->SetEnd(opEndNode, opEndOffset);
  return res;
} 





nsresult 
nsHTMLEditRules::GetNodesForOperation(nsCOMArray<nsIDOMRange>& inArrayOfRanges, 
                                      nsCOMArray<nsIDOMNode>& outArrayOfNodes, 
                                      PRInt32 inOperationType,
                                      PRBool aDontTouchContent)
{
  PRInt32 rangeCount = inArrayOfRanges.Count();
  
  PRInt32 i;
  nsCOMPtr<nsIDOMRange> opRange;

  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);

  nsresult res = NS_OK;
  
  
  
  
  if (!aDontTouchContent)
  {
    nsVoidArray rangeItemArray;
    
    
    
    for (i = 0; i < (PRInt32)rangeCount; i++)
    {
      opRange = inArrayOfRanges[0];
      nsRangeStore *item = new nsRangeStore();
      if (!item) return NS_ERROR_NULL_POINTER;
      item->StoreRange(opRange);
      mHTMLEditor->mRangeUpdater.RegisterRangeItem(item);
      rangeItemArray.AppendElement((void*)item);
      inArrayOfRanges.RemoveObjectAt(0);
    }    
    
    for (i = rangeCount-1; i >= 0; i--)
    {
      nsRangeStore *item = (nsRangeStore*)rangeItemArray.ElementAt(i);
      res = BustUpInlinesAtRangeEndpoints(*item);
      if (NS_FAILED(res)) return res;    
    } 
    
    for (i = 0; i < rangeCount; i++)
    {
      nsRangeStore *item = (nsRangeStore*)rangeItemArray.ElementAt(0);
      if (!item) return NS_ERROR_NULL_POINTER;
      rangeItemArray.RemoveElementAt(0);
      mHTMLEditor->mRangeUpdater.DropRangeItem(item);
      res = item->GetRange(address_of(opRange));
      if (NS_FAILED(res)) return res;
      delete item;
      inArrayOfRanges.AppendObject(opRange);
    }    
  }
  
  for (i = 0; i < rangeCount; i++)
  {
    opRange = inArrayOfRanges[i];
    
    nsTrivialFunctor functor;
    nsDOMSubtreeIterator iter;
    res = iter.Init(opRange);
    if (NS_FAILED(res)) return res;
    res = iter.AppendList(functor, outArrayOfNodes);
    if (NS_FAILED(res)) return res;    
  }    

  
  
  if (inOperationType == kMakeBasicBlock)
  {
    PRInt32 listCount = outArrayOfNodes.Count();
    for (i=listCount-1; i>=0; i--)
    {
      nsCOMPtr<nsIDOMNode> node = outArrayOfNodes[i];
      if (nsHTMLEditUtils::IsListItem(node))
      {
        PRInt32 j=i;
        outArrayOfNodes.RemoveObjectAt(i);
        res = GetInnerContent(node, outArrayOfNodes, &j);
        if (NS_FAILED(res)) return res;
      }
    }
  }
  
  
  else if ( (inOperationType == kOutdent)  ||
            (inOperationType == kIndent)   ||
            (inOperationType == kSetAbsolutePosition))
  {
    PRInt32 listCount = outArrayOfNodes.Count();
    for (i=listCount-1; i>=0; i--)
    {
      nsCOMPtr<nsIDOMNode> node = outArrayOfNodes[i];
      if (nsHTMLEditUtils::IsTableElementButNotTable(node))
      {
        PRInt32 j=i;
        outArrayOfNodes.RemoveObjectAt(i);
        res = GetInnerContent(node, outArrayOfNodes, &j);
        if (NS_FAILED(res)) return res;
      }
    }
  }
  
  if (inOperationType == kOutdent && !useCSS) 
  {
    PRInt32 listCount = outArrayOfNodes.Count();
    for (i=listCount-1; i>=0; i--)
    {
      nsCOMPtr<nsIDOMNode> node = outArrayOfNodes[i];
      if (nsHTMLEditUtils::IsDiv(node))
      {
        PRInt32 j=i;
        outArrayOfNodes.RemoveObjectAt(i);
        res = GetInnerContent(node, outArrayOfNodes, &j, PR_FALSE, PR_FALSE);
        if (NS_FAILED(res)) return res;
      }
    }
  }


  
  
  if ( (inOperationType == kMakeBasicBlock)   ||
       (inOperationType == kMakeList)         ||
       (inOperationType == kAlign)            ||
       (inOperationType == kSetAbsolutePosition) ||
       (inOperationType == kIndent)           ||
       (inOperationType == kOutdent) )
  {
    PRInt32 listCount = outArrayOfNodes.Count();
    for (i=listCount-1; i>=0; i--)
    {
      nsCOMPtr<nsIDOMNode> node = outArrayOfNodes[i];
      if (!aDontTouchContent && IsInlineNode(node) 
           && mHTMLEditor->IsContainer(node) && !mHTMLEditor->IsTextNode(node))
      {
        nsCOMArray<nsIDOMNode> arrayOfInlines;
        res = BustUpInlinesAtBRs(node, arrayOfInlines);
        if (NS_FAILED(res)) return res;
        
        outArrayOfNodes.RemoveObjectAt(i);
        outArrayOfNodes.InsertObjectsAt(arrayOfInlines, i);
      }
    }
  }
  return res;
}






nsresult 
nsHTMLEditRules::GetChildNodesForOperation(nsIDOMNode *inNode, 
                                           nsCOMArray<nsIDOMNode>& outArrayOfNodes)
{
  if (!inNode) return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIDOMNodeList> childNodes;
  nsresult res = inNode->GetChildNodes(getter_AddRefs(childNodes));
  if (NS_FAILED(res)) return res;
  if (!childNodes) return NS_ERROR_NULL_POINTER;
  PRUint32 childCount;
  res = childNodes->GetLength(&childCount);
  if (NS_FAILED(res)) return res;
  
  PRUint32 i;
  nsCOMPtr<nsIDOMNode> node;
  for (i = 0; i < childCount; i++)
  {
    res = childNodes->Item( i, getter_AddRefs(node));
    if (!node) return NS_ERROR_FAILURE;
    if (!outArrayOfNodes.AppendObject(node))
      return NS_ERROR_FAILURE;
  }
  return res;
}






nsresult 
nsHTMLEditRules::GetListActionNodes(nsCOMArray<nsIDOMNode> &outArrayOfNodes, 
                                    PRBool aEntireList,
                                    PRBool aDontTouchContent)
{
  nsresult res = NS_OK;
  
  nsCOMPtr<nsISelection>selection;
  res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
  if (!selPriv)
    return NS_ERROR_FAILURE;
  
  
  if (aEntireList)
  {       
    nsCOMPtr<nsIEnumerator> enumerator;
    res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
    if (NS_FAILED(res)) return res;
    if (!enumerator) return NS_ERROR_UNEXPECTED;

    for (enumerator->First(); NS_OK!=enumerator->IsDone(); enumerator->Next())
    {
      nsCOMPtr<nsISupports> currentItem;
      res = enumerator->CurrentItem(getter_AddRefs(currentItem));
      if (NS_FAILED(res)) return res;
      if (!currentItem) return NS_ERROR_UNEXPECTED;

      nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
      nsCOMPtr<nsIDOMNode> commonParent, parent, tmp;
      range->GetCommonAncestorContainer(getter_AddRefs(commonParent));
      if (commonParent)
      {
        parent = commonParent;
        while (parent)
        {
          if (nsHTMLEditUtils::IsList(parent))
          {
            outArrayOfNodes.AppendObject(parent);
            break;
          }
          parent->GetParentNode(getter_AddRefs(tmp));
          parent = tmp;
        }
      }
    }
    
    
    if (outArrayOfNodes.Count()) return NS_OK;
  }
  
  
  res = GetNodesFromSelection(selection, kMakeList, outArrayOfNodes, aDontTouchContent);
  if (NS_FAILED(res)) return res;                                 
               
  
  PRInt32 listCount = outArrayOfNodes.Count();
  PRInt32 i;
  for (i=listCount-1; i>=0; i--)
  {
    nsCOMPtr<nsIDOMNode> testNode = outArrayOfNodes[i];

    
    if (!mHTMLEditor->IsEditable(testNode))
    {
      outArrayOfNodes.RemoveObjectAt(i);
    }
    
    
    
    if (nsHTMLEditUtils::IsTableElementButNotTable(testNode))
    {
      PRInt32 j=i;
      outArrayOfNodes.RemoveObjectAt(i);
      res = GetInnerContent(testNode, outArrayOfNodes, &j, PR_FALSE);
      if (NS_FAILED(res)) return res;
    }
  }

  
  
  res = LookInsideDivBQandList(outArrayOfNodes);
  return res;
}





nsresult 
nsHTMLEditRules::LookInsideDivBQandList(nsCOMArray<nsIDOMNode>& aNodeArray)
{
  
  
  nsresult res = NS_OK;
  PRInt32 listCount = aNodeArray.Count();
  if (listCount == 1)
  {
    nsCOMPtr<nsIDOMNode> curNode = aNodeArray[0];
    
    while (nsHTMLEditUtils::IsDiv(curNode)
           || nsHTMLEditUtils::IsList(curNode)
           || nsHTMLEditUtils::IsBlockquote(curNode))
    {
      
      PRUint32 numChildren;
      res = mHTMLEditor->CountEditableChildren(curNode, numChildren);
      if (NS_FAILED(res)) return res;
      
      if (numChildren == 1)
      {
        
        nsCOMPtr <nsIDOMNode> tmpNode = nsEditor::GetChildAt(curNode, 0);
        if (nsHTMLEditUtils::IsDiv(tmpNode)
            || nsHTMLEditUtils::IsList(tmpNode)
            || nsHTMLEditUtils::IsBlockquote(tmpNode))
        {
          
          curNode = tmpNode;
        }
        else break;
      }
      else break;
    }
    
    
    aNodeArray.RemoveObjectAt(0);
    if ((nsHTMLEditUtils::IsDiv(curNode) || nsHTMLEditUtils::IsBlockquote(curNode)))
    {
      PRInt32 j=0;
      res = GetInnerContent(curNode, aNodeArray, &j, PR_FALSE, PR_FALSE);
    }
    else
    {
      aNodeArray.AppendObject(curNode);
    }
  }
  return res;
}





nsresult 
nsHTMLEditRules::GetDefinitionListItemTypes(nsIDOMNode *aNode, PRBool &aDT, PRBool &aDD)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;
  aDT = aDD = PR_FALSE;
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> child, temp;
  res = aNode->GetFirstChild(getter_AddRefs(child));
  while (child && NS_SUCCEEDED(res))
  {
    if (nsEditor::NodeIsType(child, nsEditProperty::dt)) aDT = PR_TRUE;
    else if (nsEditor::NodeIsType(child, nsEditProperty::dd)) aDD = PR_TRUE;
    res = child->GetNextSibling(getter_AddRefs(temp));
    child = temp;
  }
  return res;
}




nsresult 
nsHTMLEditRules::GetParagraphFormatNodes(nsCOMArray<nsIDOMNode>& outArrayOfNodes,
                                         PRBool aDontTouchContent)
{  
  nsCOMPtr<nsISelection>selection;
  nsresult res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;

  
  res = GetNodesFromSelection(selection, kMakeBasicBlock, outArrayOfNodes, aDontTouchContent);
  if (NS_FAILED(res)) return res;

  
  PRInt32 listCount = outArrayOfNodes.Count();
  PRInt32 i;
  for (i=listCount-1; i>=0; i--)
  {
    nsCOMPtr<nsIDOMNode> testNode = outArrayOfNodes[i];

    
    if (!mHTMLEditor->IsEditable(testNode))
    {
      outArrayOfNodes.RemoveObjectAt(i);
    }
    
    
    
    if (nsHTMLEditUtils::IsTableElement(testNode) ||
        nsHTMLEditUtils::IsList(testNode) || 
        nsHTMLEditUtils::IsListItem(testNode) )
    {
      PRInt32 j=i;
      outArrayOfNodes.RemoveObjectAt(i);
      res = GetInnerContent(testNode, outArrayOfNodes, &j);
      if (NS_FAILED(res)) return res;
    }
  }
  return res;
}





nsresult 
nsHTMLEditRules::BustUpInlinesAtRangeEndpoints(nsRangeStore &item)
{
  nsresult res = NS_OK;
  PRBool isCollapsed = ((item.startNode == item.endNode) && (item.startOffset == item.endOffset));

  nsCOMPtr<nsIDOMNode> endInline = GetHighestInlineParent(item.endNode);
  
  
  if (endInline && !isCollapsed)
  {
    nsCOMPtr<nsIDOMNode> resultEndNode;
    PRInt32 resultEndOffset;
    endInline->GetParentNode(getter_AddRefs(resultEndNode));
    res = mHTMLEditor->SplitNodeDeep(endInline, item.endNode, item.endOffset,
                          &resultEndOffset, PR_TRUE);
    if (NS_FAILED(res)) return res;
    
    item.endNode = resultEndNode; item.endOffset = resultEndOffset;
  }

  nsCOMPtr<nsIDOMNode> startInline = GetHighestInlineParent(item.startNode);

  if (startInline)
  {
    nsCOMPtr<nsIDOMNode> resultStartNode;
    PRInt32 resultStartOffset;
    startInline->GetParentNode(getter_AddRefs(resultStartNode));
    res = mHTMLEditor->SplitNodeDeep(startInline, item.startNode, item.startOffset,
                          &resultStartOffset, PR_TRUE);
    if (NS_FAILED(res)) return res;
    
    item.startNode = resultStartNode; item.startOffset = resultStartOffset;
  }
  
  return res;
}






nsresult 
nsHTMLEditRules::BustUpInlinesAtBRs(nsIDOMNode *inNode, 
                                    nsCOMArray<nsIDOMNode>& outArrayOfNodes)
{
  if (!inNode) return NS_ERROR_NULL_POINTER;

  
  
  nsCOMArray<nsIDOMNode> arrayOfBreaks;
  nsBRNodeFunctor functor;
  nsDOMIterator iter;
  nsresult res = iter.Init(inNode);
  if (NS_FAILED(res)) return res;
  res = iter.AppendList(functor, arrayOfBreaks);
  if (NS_FAILED(res)) return res;
  
  
  PRInt32 listCount = arrayOfBreaks.Count();
  if (!listCount)
  {
    if (!outArrayOfNodes.AppendObject(inNode))
      return NS_ERROR_FAILURE;
  }
  else
  {
    
    nsCOMPtr<nsIDOMNode> breakNode;
    nsCOMPtr<nsIDOMNode> inlineParentNode;
    nsCOMPtr<nsIDOMNode> leftNode;
    nsCOMPtr<nsIDOMNode> rightNode;
    nsCOMPtr<nsIDOMNode> splitDeepNode = inNode;
    nsCOMPtr<nsIDOMNode> splitParentNode;
    PRInt32 splitOffset, resultOffset, i;
    inNode->GetParentNode(getter_AddRefs(inlineParentNode));
    
    for (i=0; i< listCount; i++)
    {
      breakNode = arrayOfBreaks[i];
      if (!breakNode) return NS_ERROR_NULL_POINTER;
      if (!splitDeepNode) return NS_ERROR_NULL_POINTER;
      res = nsEditor::GetNodeLocation(breakNode, address_of(splitParentNode), &splitOffset);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->SplitNodeDeep(splitDeepNode, splitParentNode, splitOffset,
                          &resultOffset, PR_FALSE, address_of(leftNode), address_of(rightNode));
      if (NS_FAILED(res)) return res;
      
      if (leftNode)
      {
        
        
        
        if (!outArrayOfNodes.AppendObject(leftNode))
          return NS_ERROR_FAILURE;
      }
      
      res = mHTMLEditor->MoveNode(breakNode, inlineParentNode, resultOffset);
      if (NS_FAILED(res)) return res;
      if (!outArrayOfNodes.AppendObject(breakNode))
        return  NS_ERROR_FAILURE;
      
      splitDeepNode = rightNode;
    }
    
    if (rightNode)
    {
      if (!outArrayOfNodes.AppendObject(rightNode))
        return NS_ERROR_FAILURE;
    }
  }
  return res;
}


nsCOMPtr<nsIDOMNode> 
nsHTMLEditRules::GetHighestInlineParent(nsIDOMNode* aNode)
{
  if (!aNode) return nsnull;
  if (IsBlockNode(aNode)) return nsnull;
  nsCOMPtr<nsIDOMNode> inlineNode, node=aNode;

  while (node && IsInlineNode(node))
  {
    inlineNode = node;
    inlineNode->GetParentNode(getter_AddRefs(node));
  }
  return inlineNode;
}






nsresult 
nsHTMLEditRules::GetNodesFromPoint(DOMPoint point,
                                   PRInt32 operation,
                                   nsCOMArray<nsIDOMNode> &arrayOfNodes,
                                   PRBool dontTouchContent)
{
  nsresult res;

  
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 offset;
  point.GetPoint(node, offset);
  
  
  nsCOMPtr<nsIDOMRange> range = do_CreateInstance("@mozilla.org/content/range;1");
  res = range->SetStart(node, offset);
  if (NS_FAILED(res)) return res;
  


  
  
  res = PromoteRange(range, operation);
  if (NS_FAILED(res)) return res;
      
  
  nsCOMArray<nsIDOMRange> arrayOfRanges;
  
  
  arrayOfRanges.AppendObject(range);
  
  
  res = GetNodesForOperation(arrayOfRanges, arrayOfNodes, operation, dontTouchContent); 
  return res;
}






nsresult 
nsHTMLEditRules::GetNodesFromSelection(nsISelection *selection,
                                       PRInt32 operation,
                                       nsCOMArray<nsIDOMNode>& arrayOfNodes,
                                       PRBool dontTouchContent)
{
  if (!selection) return NS_ERROR_NULL_POINTER;
  nsresult res;
  
  
  nsCOMArray<nsIDOMRange> arrayOfRanges;
  res = GetPromotedRanges(selection, arrayOfRanges, operation);
  if (NS_FAILED(res)) return res;
  
  
  res = GetNodesForOperation(arrayOfRanges, arrayOfNodes, operation, dontTouchContent); 
  return res;
}







nsresult 
nsHTMLEditRules::MakeTransitionList(nsCOMArray<nsIDOMNode>& inArrayOfNodes, 
                                    nsVoidArray &inTransitionArray)
{
  PRInt32 listCount = inArrayOfNodes.Count();
  PRInt32 i;
  nsVoidArray transitionList;
  nsCOMPtr<nsIDOMNode> prevElementParent;
  nsCOMPtr<nsIDOMNode> curElementParent;
  
  for (i=0; i<listCount; i++)
  {
    nsIDOMNode* transNode = inArrayOfNodes[i];
    transNode->GetParentNode(getter_AddRefs(curElementParent));
    if (curElementParent != prevElementParent)
    {
      
      inTransitionArray.InsertElementAt((void*)PR_TRUE,i);  
    }
    else
    {
      
      inTransitionArray.InsertElementAt((void*)PR_FALSE,i); 
    }
    prevElementParent = curElementParent;
  }
  return NS_OK;
}






 





nsCOMPtr<nsIDOMNode> 
nsHTMLEditRules::IsInListItem(nsIDOMNode *aNode)
{
  if (!aNode) return nsnull;  
  if (nsHTMLEditUtils::IsListItem(aNode)) return aNode;
  
  nsCOMPtr<nsIDOMNode> parent, tmp;
  aNode->GetParentNode(getter_AddRefs(parent));
  
  while (parent)
  {
    if (nsHTMLEditUtils::IsTableElement(parent)) return nsnull;
    if (nsHTMLEditUtils::IsListItem(parent)) return parent;
    tmp=parent; tmp->GetParentNode(getter_AddRefs(parent));
  }
  return nsnull;
}





nsresult 
nsHTMLEditRules::ReturnInHeader(nsISelection *aSelection, 
                                nsIDOMNode *aHeader, 
                                nsIDOMNode *aNode, 
                                PRInt32 aOffset)
{
  if (!aSelection || !aHeader || !aNode) return NS_ERROR_NULL_POINTER;  
  
  
  nsCOMPtr<nsIDOMNode> headerParent;
  PRInt32 offset;
  nsresult res = nsEditor::GetNodeLocation(aHeader, address_of(headerParent), &offset);
  if (NS_FAILED(res)) return res;

  
  nsCOMPtr<nsIDOMNode> selNode = aNode;
  res = nsWSRunObject::PrepareToSplitAcrossBlocks(mHTMLEditor, address_of(selNode), &aOffset);
  if (NS_FAILED(res)) return res;

  
  PRInt32 newOffset;
  res = mHTMLEditor->SplitNodeDeep( aHeader, selNode, aOffset, &newOffset);
  if (NS_FAILED(res)) return res;

  
  nsCOMPtr<nsIDOMNode> prevItem;
  mHTMLEditor->GetPriorHTMLSibling(aHeader, address_of(prevItem));
  if (prevItem && nsHTMLEditUtils::IsHeader(prevItem))
  {
    PRBool bIsEmptyNode;
    res = mHTMLEditor->IsEmptyNode(prevItem, &bIsEmptyNode);
    if (NS_FAILED(res)) return res;
    if (bIsEmptyNode)
    {
      nsCOMPtr<nsIDOMNode> brNode;
      res = CreateMozBR(prevItem, 0, address_of(brNode));
      if (NS_FAILED(res)) return res;
    }
  }
  
  
  PRBool isEmpty;
  res = IsEmptyBlock(aHeader, &isEmpty, PR_TRUE);
  if (NS_FAILED(res)) return res;
  if (isEmpty)
  {
    res = mHTMLEditor->DeleteNode(aHeader);
    if (NS_FAILED(res)) return res;
    
    
    nsCOMPtr<nsIDOMNode> sibling;
    res = mHTMLEditor->GetNextHTMLSibling(headerParent, offset+1, address_of(sibling));
    if (NS_FAILED(res)) return res;
    if (!sibling || !nsTextEditUtils::IsBreak(sibling))
    {
      res = CreateMozBR(headerParent, offset+1, address_of(sibling));
      if (NS_FAILED(res)) return res;
    }
    res = nsEditor::GetNodeLocation(sibling, address_of(headerParent), &offset);
    if (NS_FAILED(res)) return res;
    
    res = aSelection->Collapse(headerParent,offset+1);
  }
  else
  {
    
    res = aSelection->Collapse(aHeader,0);
  }
  return res;
}




nsresult 
nsHTMLEditRules::ReturnInParagraph(nsISelection *aSelection, 
                                   nsIDOMNode *aPara, 
                                   nsIDOMNode *aNode, 
                                   PRInt32 aOffset,
                                   PRBool *aCancel,
                                   PRBool *aHandled)
{
  if (!aSelection || !aPara || !aNode || !aCancel || !aHandled) 
    { return NS_ERROR_NULL_POINTER; }
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;

  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  nsresult res = nsEditor::GetNodeLocation(aNode, address_of(parent), &offset);
  if (NS_FAILED(res)) return res;

  PRBool  doesCRCreateNewP;
  res = mHTMLEditor->GetReturnInParagraphCreatesNewParagraph(&doesCRCreateNewP);
  if (NS_FAILED(res)) return res;

  PRBool newBRneeded = PR_FALSE;
  nsCOMPtr<nsIDOMNode> sibling;

  if (mHTMLEditor->IsTextNode(aNode))
  {
    nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(aNode);
    PRUint32 strLength;
    res = textNode->GetLength(&strLength);
    if (NS_FAILED(res)) return res;
    
    
    if (!aOffset)
    {
      
      mHTMLEditor->GetPriorHTMLSibling(aNode, address_of(sibling));
      if (!sibling ||
          !mHTMLEditor->IsVisBreak(sibling) || nsTextEditUtils::HasMozAttr(sibling))
      {
        newBRneeded = PR_TRUE;
      }
    }
    else if (aOffset == (PRInt32)strLength)
    {
      
      
      res = mHTMLEditor->GetNextHTMLSibling(aNode, address_of(sibling));
      if (!sibling ||
          !mHTMLEditor->IsVisBreak(sibling) || nsTextEditUtils::HasMozAttr(sibling)) 
      {
        newBRneeded = PR_TRUE;
        offset++;
      }
    }
    else
    {
      if (doesCRCreateNewP)
      {
        nsCOMPtr<nsIDOMNode> tmp;
        res = mEditor->SplitNode(aNode, aOffset, getter_AddRefs(tmp));
        if (NS_FAILED(res)) return res;
        aNode = tmp;
      }

      newBRneeded = PR_TRUE;
      offset++;
    }
  }
  else
  {
    
    
    nsCOMPtr<nsIDOMNode> nearNode, selNode = aNode;
    res = mHTMLEditor->GetPriorHTMLNode(aNode, aOffset, address_of(nearNode));
    if (NS_FAILED(res)) return res;
    if (!nearNode || !mHTMLEditor->IsVisBreak(nearNode) || nsTextEditUtils::HasMozAttr(nearNode)) 
    {
      
      res = mHTMLEditor->GetNextHTMLNode(aNode, aOffset, address_of(nearNode));
      if (NS_FAILED(res)) return res;
      if (!nearNode || !mHTMLEditor->IsVisBreak(nearNode) || nsTextEditUtils::HasMozAttr(nearNode)) 
      {
        newBRneeded = PR_TRUE;
      }
    }
    if (!newBRneeded)
      sibling = nearNode;
  }
  if (newBRneeded)
  {
    
    if (!doesCRCreateNewP)
      return NS_OK;

    nsCOMPtr<nsIDOMNode> brNode;
    res =  mHTMLEditor->CreateBR(parent, offset, address_of(brNode));
    sibling = brNode;
  }
  nsCOMPtr<nsIDOMNode> selNode = aNode;
  *aHandled = PR_TRUE;
  return SplitParagraph(aPara, sibling, aSelection, address_of(selNode), &aOffset);
}




nsresult 
nsHTMLEditRules::SplitParagraph(nsIDOMNode *aPara,
                                nsIDOMNode *aBRNode, 
                                nsISelection *aSelection,
                                nsCOMPtr<nsIDOMNode> *aSelNode, 
                                PRInt32 *aOffset)
{
  if (!aPara || !aBRNode || !aSelNode || !*aSelNode || !aOffset || !aSelection) 
    return NS_ERROR_NULL_POINTER;
  nsresult res = NS_OK;
  
  
  PRInt32 newOffset;
  
  nsCOMPtr<nsIDOMNode> leftPara, rightPara;
  res = nsWSRunObject::PrepareToSplitAcrossBlocks(mHTMLEditor, aSelNode, aOffset);
  if (NS_FAILED(res)) return res;
  
  res = mHTMLEditor->SplitNodeDeep(aPara, *aSelNode, *aOffset, &newOffset, PR_FALSE,
                                   address_of(leftPara), address_of(rightPara));
  if (NS_FAILED(res)) return res;
  
  if (mHTMLEditor->IsVisBreak(aBRNode))
  {
    res = mHTMLEditor->DeleteNode(aBRNode);  
    if (NS_FAILED(res)) return res;
  }
  
  
  res = InsertMozBRIfNeeded(leftPara);
  if (NS_FAILED(res)) return res;
  res = InsertMozBRIfNeeded(rightPara);
  if (NS_FAILED(res)) return res;

  
  
  nsCOMPtr<nsIDOMNode> child = mHTMLEditor->GetLeftmostChild(rightPara, PR_TRUE);
  if (mHTMLEditor->IsTextNode(child) || mHTMLEditor->IsContainer(child))
  {
    aSelection->Collapse(child,0);
  }
  else
  {
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    res = nsEditor::GetNodeLocation(child, address_of(parent), &offset);
    aSelection->Collapse(parent,offset);
  }
  return res;
}





nsresult 
nsHTMLEditRules::ReturnInListItem(nsISelection *aSelection, 
                                  nsIDOMNode *aListItem, 
                                  nsIDOMNode *aNode, 
                                  PRInt32 aOffset)
{
  if (!aSelection || !aListItem || !aNode) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelection> selection(aSelection);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> listitem;
  
  
  NS_PRECONDITION(PR_TRUE == nsHTMLEditUtils::IsListItem(aListItem),
                  "expected a list item and didn't get one");
  
  
  PRBool isEmpty;
  res = IsEmptyBlock(aListItem, &isEmpty, PR_TRUE, PR_FALSE);
  if (NS_FAILED(res)) return res;
  if (isEmpty && mReturnInEmptyLIKillsList)   
  {
    nsCOMPtr<nsIDOMNode> list, listparent;
    PRInt32 offset, itemOffset;
    res = nsEditor::GetNodeLocation(aListItem, address_of(list), &itemOffset);
    if (NS_FAILED(res)) return res;
    res = nsEditor::GetNodeLocation(list, address_of(listparent), &offset);
    if (NS_FAILED(res)) return res;
    
    
    PRBool bIsLast;
    res = mHTMLEditor->IsLastEditableChild(aListItem, &bIsLast);
    if (NS_FAILED(res)) return res;
    if (!bIsLast)
    {
      
      nsCOMPtr<nsIDOMNode> tempNode;
      res = mHTMLEditor->SplitNode(list, itemOffset, getter_AddRefs(tempNode));
      if (NS_FAILED(res)) return res;
    }
    
    if (nsHTMLEditUtils::IsList(listparent))  
    {
      
      res = mHTMLEditor->MoveNode(aListItem,listparent,offset+1);
      if (NS_FAILED(res)) return res;
      res = aSelection->Collapse(aListItem,0);
    }
    else
    {
      
      res = mHTMLEditor->DeleteNode(aListItem);
      if (NS_FAILED(res)) return res;
      
      
      nsCOMPtr<nsIDOMNode> brNode;
      res = CreateMozBR(listparent, offset+1, address_of(brNode));
      if (NS_FAILED(res)) return res;
      
      
      selPriv->SetInterlinePosition(PR_TRUE);
      res = aSelection->Collapse(listparent,offset+1);
    }
    return res;
  }
  
  
  
  nsCOMPtr<nsIDOMNode> selNode = aNode;
  res = nsWSRunObject::PrepareToSplitAcrossBlocks(mHTMLEditor, address_of(selNode), &aOffset);
  if (NS_FAILED(res)) return res;
  
  PRInt32 newOffset;
  res = mHTMLEditor->SplitNodeDeep( aListItem, selNode, aOffset, &newOffset, PR_FALSE);
  if (NS_FAILED(res)) return res;
  
  
  
  nsCOMPtr<nsIDOMNode> prevItem;
  mHTMLEditor->GetPriorHTMLSibling(aListItem, address_of(prevItem));

  if (prevItem && nsHTMLEditUtils::IsListItem(prevItem))
  {
    PRBool bIsEmptyNode;
    res = mHTMLEditor->IsEmptyNode(prevItem, &bIsEmptyNode);
    if (NS_FAILED(res)) return res;
    if (bIsEmptyNode)
    {
      nsCOMPtr<nsIDOMNode> brNode;
      res = CreateMozBR(prevItem, 0, address_of(brNode));
      if (NS_FAILED(res)) return res;
    }
    else 
    {
      res = mHTMLEditor->IsEmptyNode(aListItem, &bIsEmptyNode, PR_TRUE);
      if (NS_FAILED(res)) return res;
      if (bIsEmptyNode) 
      {
        nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(aListItem);
        if (nodeAtom == nsEditProperty::dd || nodeAtom == nsEditProperty::dt)
        {
          nsCOMPtr<nsIDOMNode> list;
          PRInt32 itemOffset;
          res = nsEditor::GetNodeLocation(aListItem, address_of(list), &itemOffset);
          if (NS_FAILED(res)) return res;

          nsAutoString listTag((nodeAtom == nsEditProperty::dt) ? NS_LITERAL_STRING("dd") : NS_LITERAL_STRING("dt"));
          nsCOMPtr<nsIDOMNode> newListItem;
          res = mHTMLEditor->CreateNode(listTag, list, itemOffset+1, getter_AddRefs(newListItem));
          if (NS_FAILED(res)) return res;
          res = mEditor->DeleteNode(aListItem);
          if (NS_FAILED(res)) return res;
          return aSelection->Collapse(newListItem, 0);
        }

        nsCOMPtr<nsIDOMNode> brNode;
        res = mHTMLEditor->CopyLastEditableChildStyles(prevItem, aListItem, getter_AddRefs(brNode));
        if (NS_FAILED(res)) return res;
        if (brNode) 
        {
          nsCOMPtr<nsIDOMNode> brParent;
          PRInt32 offset;
          res = nsEditor::GetNodeLocation(brNode, address_of(brParent), &offset);
          return aSelection->Collapse(brParent, offset);
        }
      }
      else
      {
        nsWSRunObject wsObj(mHTMLEditor, aListItem, 0);
        nsCOMPtr<nsIDOMNode> visNode;
        PRInt32 visOffset = 0;
        PRInt16 wsType;
        res = wsObj.NextVisibleNode(aListItem, 0, address_of(visNode), &visOffset, &wsType);
        if (NS_FAILED(res)) return res;
        if ( (wsType==nsWSRunObject::eSpecial)  || 
             (wsType==nsWSRunObject::eBreak)    ||
             nsHTMLEditUtils::IsHR(visNode) ) 
        {
          nsCOMPtr<nsIDOMNode> parent;
          PRInt32 offset;
          res = nsEditor::GetNodeLocation(visNode, address_of(parent), &offset);
          if (NS_FAILED(res)) return res;
          return aSelection->Collapse(parent, offset);
        }
        else
        {
          return aSelection->Collapse(visNode, visOffset);
        }
      }
    }
  }
  res = aSelection->Collapse(aListItem,0);
  return res;
}





nsresult 
nsHTMLEditRules::MakeBlockquote(nsCOMArray<nsIDOMNode>& arrayOfNodes)
{
  
  
  
  
  
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> curNode, curParent, curBlock, newBlock;
  PRInt32 offset;
  PRInt32 listCount = arrayOfNodes.Count();
  
  nsCOMPtr<nsIDOMNode> prevParent;
  
  PRInt32 i;
  for (i=0; i<listCount; i++)
  {
    
    curNode = arrayOfNodes[i];
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;

    
    if (nsHTMLEditUtils::IsTableElementButNotTable(curNode) || 
        nsHTMLEditUtils::IsListItem(curNode))
    {
      curBlock = 0;  
      
      nsCOMArray<nsIDOMNode> childArray;
      res = GetChildNodesForOperation(curNode, childArray);
      if (NS_FAILED(res)) return res;
      res = MakeBlockquote(childArray);
      if (NS_FAILED(res)) return res;
    }
    
    
    
    if (prevParent)
    {
      nsCOMPtr<nsIDOMNode> temp;
      curNode->GetParentNode(getter_AddRefs(temp));
      if (temp != prevParent)
      {
        curBlock = 0;  
        prevParent = temp;
      }
    }
    else     

    {
      curNode->GetParentNode(getter_AddRefs(prevParent));
    }
    
    
    if (!curBlock)
    {
      NS_NAMED_LITERAL_STRING(quoteType, "blockquote");
      res = SplitAsNeeded(&quoteType, address_of(curParent), &offset);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->CreateNode(quoteType, curParent, offset, getter_AddRefs(curBlock));
      if (NS_FAILED(res)) return res;
      
      mNewBlock = curBlock;
      
    }
      
    res = mHTMLEditor->MoveNode(curNode, curBlock, -1);
    if (NS_FAILED(res)) return res;
  }
  return res;
}






nsresult 
nsHTMLEditRules::RemoveBlockStyle(nsCOMArray<nsIDOMNode>& arrayOfNodes)
{
  
  
  
  
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> curNode, curParent, curBlock, firstNode, lastNode;
  PRInt32 offset;
  PRInt32 listCount = arrayOfNodes.Count();
    
  PRInt32 i;
  for (i=0; i<listCount; i++)
  {
    
    curNode = arrayOfNodes[i];
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;
    nsAutoString curNodeTag, curBlockTag;
    nsEditor::GetTagString(curNode, curNodeTag);
    ToLowerCase(curNodeTag);
 
    
    if (nsHTMLEditUtils::IsFormatNode(curNode))
    {
      
      if (curBlock)
      {
        res = RemovePartOfBlock(curBlock, firstNode, lastNode);
        if (NS_FAILED(res)) return res;
        curBlock = 0;  firstNode = 0;  lastNode = 0;
      }
      
      res = mHTMLEditor->RemoveBlockContainer(curNode); 
      if (NS_FAILED(res)) return res;
    }
    else if (nsHTMLEditUtils::IsTable(curNode)                    || 
             nsHTMLEditUtils::IsTableRow(curNode)                 ||
             (curNodeTag.EqualsLiteral("tbody"))      ||
             (curNodeTag.EqualsLiteral("td"))         ||
             nsHTMLEditUtils::IsList(curNode)                     ||
             (curNodeTag.EqualsLiteral("li"))         ||
             nsHTMLEditUtils::IsBlockquote(curNode)               ||
             nsHTMLEditUtils::IsDiv(curNode))
    {
      
      if (curBlock)
      {
        res = RemovePartOfBlock(curBlock, firstNode, lastNode);
        if (NS_FAILED(res)) return res;
        curBlock = 0;  firstNode = 0;  lastNode = 0;
      }
      
      nsCOMArray<nsIDOMNode> childArray;
      res = GetChildNodesForOperation(curNode, childArray);
      if (NS_FAILED(res)) return res;
      res = RemoveBlockStyle(childArray);
      if (NS_FAILED(res)) return res;
    }
    else if (IsInlineNode(curNode))
    {
      if (curBlock)
      {
        
        if (nsEditorUtils::IsDescendantOf(curNode, curBlock))
        {
          lastNode = curNode;
          continue;  
        }
        else
        {
          
          
          
          res = RemovePartOfBlock(curBlock, firstNode, lastNode);
          if (NS_FAILED(res)) return res;
          curBlock = 0;  firstNode = 0;  lastNode = 0;
          
        }
      }
      curBlock = mHTMLEditor->GetBlockNodeParent(curNode);
      if (nsHTMLEditUtils::IsFormatNode(curBlock))
      {
        firstNode = curNode;  
        lastNode = curNode;
      }
      else
        curBlock = 0;  
    }
    else
    { 
      
      if (curBlock)
      {
        res = RemovePartOfBlock(curBlock, firstNode, lastNode);
        if (NS_FAILED(res)) return res;
        curBlock = 0;  firstNode = 0;  lastNode = 0;
      }
    }
  }
  
  if (curBlock)
  {
    res = RemovePartOfBlock(curBlock, firstNode, lastNode);
    if (NS_FAILED(res)) return res;
    curBlock = 0;  firstNode = 0;  lastNode = 0;
  }
  return res;
}






nsresult 
nsHTMLEditRules::ApplyBlockStyle(nsCOMArray<nsIDOMNode>& arrayOfNodes, const nsAString *aBlockTag)
{
  
  
  
  
  if (!aBlockTag) return NS_ERROR_NULL_POINTER;
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> curNode, curParent, curBlock, newBlock;
  PRInt32 offset;
  PRInt32 listCount = arrayOfNodes.Count();
  nsString tString(*aBlockTag);

  
  PRInt32 j;
  for (j=listCount-1; j>=0; j--)
  {
    if (!mHTMLEditor->IsEditable(arrayOfNodes[j]))
    {
      arrayOfNodes.RemoveObjectAt(j);
    }
  }
  
  
  listCount = arrayOfNodes.Count();
  
  PRInt32 i;
  for (i=0; i<listCount; i++)
  {
    
    curNode = arrayOfNodes[i];
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;
    nsAutoString curNodeTag;
    nsEditor::GetTagString(curNode, curNodeTag);
    ToLowerCase(curNodeTag);
 
    
    if (curNodeTag == *aBlockTag)
    {
      curBlock = 0;  
      continue;  
    }
        
    
    
    
    if (nsHTMLEditUtils::IsMozDiv(curNode)     ||
        nsHTMLEditUtils::IsFormatNode(curNode))
    {
      curBlock = 0;  
      res = mHTMLEditor->ReplaceContainer(curNode, address_of(newBlock), *aBlockTag,
                                          nsnull, nsnull, PR_TRUE);
      if (NS_FAILED(res)) return res;
    }
    else if (nsHTMLEditUtils::IsTable(curNode)                    || 
             (curNodeTag.EqualsLiteral("tbody"))      ||
             (curNodeTag.EqualsLiteral("tr"))         ||
             (curNodeTag.EqualsLiteral("td"))         ||
             nsHTMLEditUtils::IsList(curNode)                     ||
             (curNodeTag.EqualsLiteral("li"))         ||
             nsHTMLEditUtils::IsBlockquote(curNode)               ||
             nsHTMLEditUtils::IsDiv(curNode))
    {
      curBlock = 0;  
      
      nsCOMArray<nsIDOMNode> childArray;
      res = GetChildNodesForOperation(curNode, childArray);
      if (NS_FAILED(res)) return res;
      PRInt32 childCount = childArray.Count();
      if (childCount)
      {
        res = ApplyBlockStyle(childArray, aBlockTag);
        if (NS_FAILED(res)) return res;
      }
      else
      {
        
        res = SplitAsNeeded(aBlockTag, address_of(curParent), &offset);
        if (NS_FAILED(res)) return res;
        nsCOMPtr<nsIDOMNode> theBlock;
        res = mHTMLEditor->CreateNode(*aBlockTag, curParent, offset, getter_AddRefs(theBlock));
        if (NS_FAILED(res)) return res;
        
        mNewBlock = theBlock;
      }
    }
    
    
    else if (curNodeTag.EqualsLiteral("br"))
    {
      if (curBlock)
      {
        curBlock = 0;  
        res = mHTMLEditor->DeleteNode(curNode);
        if (NS_FAILED(res)) return res;
      }
      else
      {
        
        
        res = SplitAsNeeded(aBlockTag, address_of(curParent), &offset);
        if (NS_FAILED(res)) return res;
        res = mHTMLEditor->CreateNode(*aBlockTag, curParent, offset, getter_AddRefs(curBlock));
        if (NS_FAILED(res)) return res;
        
        mNewBlock = curBlock;
        
        res = mHTMLEditor->MoveNode(curNode, curBlock, -1);
        if (NS_FAILED(res)) return res;
      }
    }
        
    
    
    
    
    
    
    
    
    else if (IsInlineNode(curNode))
    {
      
      if (tString.LowerCaseEqualsLiteral("pre") 
        && (!mHTMLEditor->IsEditable(curNode)))
        continue; 
      
      
      if (!curBlock)
      {
        res = SplitAsNeeded(aBlockTag, address_of(curParent), &offset);
        if (NS_FAILED(res)) return res;
        res = mHTMLEditor->CreateNode(*aBlockTag, curParent, offset, getter_AddRefs(curBlock));
        if (NS_FAILED(res)) return res;
        
        mNewBlock = curBlock;
        
      }
      
      
      
 
      
      
      res = mHTMLEditor->MoveNode(curNode, curBlock, -1);
      if (NS_FAILED(res)) return res;
    }
  }
  return res;
}






nsresult 
nsHTMLEditRules::SplitAsNeeded(const nsAString *aTag, 
                               nsCOMPtr<nsIDOMNode> *inOutParent,
                               PRInt32 *inOutOffset)
{
  if (!aTag || !inOutParent || !inOutOffset) return NS_ERROR_NULL_POINTER;
  if (!*inOutParent) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> tagParent, temp, splitNode, parent = *inOutParent;
  nsresult res = NS_OK;
   
  
  while (!tagParent)
  {
    
    
    if (!parent) break;
    if (mHTMLEditor->CanContainTag(parent, *aTag))
    {
      tagParent = parent;
      break;
    }
    splitNode = parent;
    parent->GetParentNode(getter_AddRefs(temp));
    parent = temp;
  }
  if (!tagParent)
  {
    
    return NS_ERROR_FAILURE;
  }
  if (splitNode)
  {
    
    res = mHTMLEditor->SplitNodeDeep(splitNode, *inOutParent, *inOutOffset, inOutOffset);
    if (NS_FAILED(res)) return res;
    *inOutParent = tagParent;
  }
  return res;
}      





nsresult 
nsHTMLEditRules::JoinNodesSmart( nsIDOMNode *aNodeLeft, 
                                 nsIDOMNode *aNodeRight, 
                                 nsCOMPtr<nsIDOMNode> *aOutMergeParent, 
                                 PRInt32 *aOutMergeOffset)
{
  
  if (!aNodeLeft ||  
      !aNodeRight || 
      !aOutMergeParent ||
      !aOutMergeOffset) 
    return NS_ERROR_NULL_POINTER;
  
  nsresult res = NS_OK;
  
  
  PRInt32 parOffset;
  nsCOMPtr<nsIDOMNode> parent, rightParent;
  res = nsEditor::GetNodeLocation(aNodeLeft, address_of(parent), &parOffset);
  if (NS_FAILED(res)) return res;
  aNodeRight->GetParentNode(getter_AddRefs(rightParent));

  
  
  if (parent != rightParent)
  {
    res = mHTMLEditor->MoveNode(aNodeRight, parent, parOffset);
    if (NS_FAILED(res)) return res;
  }
  
  
  *aOutMergeParent = aNodeRight;
  res = mHTMLEditor->GetLengthOfDOMNode(aNodeLeft, *((PRUint32*)aOutMergeOffset));
  if (NS_FAILED(res)) return res;

  
  if (nsHTMLEditUtils::IsList(aNodeLeft) ||
      mHTMLEditor->IsTextNode(aNodeLeft))
  {
    
    res = mHTMLEditor->JoinNodes(aNodeLeft, aNodeRight, parent);
    if (NS_FAILED(res)) return res;
    return res;
  }
  else
  {
    
    nsCOMPtr<nsIDOMNode> lastLeft, firstRight;
    res = mHTMLEditor->GetLastEditableChild(aNodeLeft, address_of(lastLeft));
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->GetFirstEditableChild(aNodeRight, address_of(firstRight));
    if (NS_FAILED(res)) return res;

    
    res = mHTMLEditor->JoinNodes(aNodeLeft, aNodeRight, parent);
    if (NS_FAILED(res)) return res;

    if (lastLeft && firstRight &&
        mHTMLEditor->NodesSameType(lastLeft, firstRight) &&
        (nsEditor::IsTextNode(lastLeft) ||
         mHTMLEditor->mHTMLCSSUtils->ElementsSameStyle(lastLeft, firstRight)))
      return JoinNodesSmart(lastLeft, firstRight, aOutMergeParent, aOutMergeOffset);
  }
  return res;
}


nsresult 
nsHTMLEditRules::GetTopEnclosingMailCite(nsIDOMNode *aNode, 
                                         nsCOMPtr<nsIDOMNode> *aOutCiteNode,
                                         PRBool aPlainText)
{
  
  if (!aNode || !aOutCiteNode) 
    return NS_ERROR_NULL_POINTER;
  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> node, parentNode;
  node = do_QueryInterface(aNode);
  
  while (node)
  {
    if ( (aPlainText && nsHTMLEditUtils::IsPre(node)) ||
         nsHTMLEditUtils::IsMailCite(node) )
      *aOutCiteNode = node;
    if (nsTextEditUtils::IsBody(node)) break;
    
    res = node->GetParentNode(getter_AddRefs(parentNode));
    if (NS_FAILED(res)) return res;
    node = parentNode;
  }

  return res;
}


nsresult 
nsHTMLEditRules::CacheInlineStyles(nsIDOMNode *aNode)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;

  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);

  PRInt32 j;
  for (j=0; j<SIZE_STYLE_TABLE; j++)
  {
    PRBool isSet = PR_FALSE;
    nsAutoString outValue;
    nsCOMPtr<nsIDOMNode> resultNode;
    if (!useCSS)
    {
      mHTMLEditor->IsTextPropertySetByContent(aNode, mCachedStyles[j].tag, &(mCachedStyles[j].attr), nsnull,
                               isSet, getter_AddRefs(resultNode), &outValue);
    }
    else
    {
      mHTMLEditor->mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(aNode, mCachedStyles[j].tag, &(mCachedStyles[j].attr),
                                                    isSet, outValue, COMPUTED_STYLE_TYPE);
    }
    if (isSet)
    {
      mCachedStyles[j].mPresent = PR_TRUE;
      mCachedStyles[j].value.Assign(outValue);
    }
  }
  return NS_OK;
}


nsresult 
nsHTMLEditRules::ReapplyCachedStyles()
{
  
  
  
  
  
  
  
  mHTMLEditor->mTypeInState->Reset();

  
  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);

  
  nsCOMPtr<nsISelection>selection;
  nsresult res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  nsCOMPtr<nsIDOMNode> selNode;
  PRInt32 selOffset;
  res = mHTMLEditor->GetStartNodeAndOffset(selection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;

  res = NS_OK;
  PRInt32 j;
  for (j=0; j<SIZE_STYLE_TABLE; j++)
  {
    if (mCachedStyles[j].mPresent)
    {
      PRBool bFirst, bAny, bAll;
      bFirst = bAny = bAll = PR_FALSE;
      
      nsAutoString curValue;
      if (useCSS) 
      {
        mHTMLEditor->mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(selNode, mCachedStyles[j].tag, &(mCachedStyles[j].attr),
                                                    bAny, curValue, COMPUTED_STYLE_TYPE);
      }
      if (!bAny) 
      {
        res = mHTMLEditor->GetInlinePropertyBase(mCachedStyles[j].tag, &(mCachedStyles[j].attr), &(mCachedStyles[j].value), 
                                                        &bFirst, &bAny, &bAll, &curValue, PR_FALSE);
        if (NS_FAILED(res)) return res;
      }
      
      if (!bAny) 
      {
        mHTMLEditor->mTypeInState->SetProp(mCachedStyles[j].tag, mCachedStyles[j].attr, mCachedStyles[j].value);
      }
    }
  }
  return NS_OK;
}


nsresult
nsHTMLEditRules::ClearCachedStyles()
{
  
  
  PRInt32 j;
  for (j=0; j<SIZE_STYLE_TABLE; j++)
  {
    mCachedStyles[j].mPresent = PR_FALSE;
    mCachedStyles[j].value.Truncate(0);
  }
  return NS_OK;
}


nsresult 
nsHTMLEditRules::AdjustSpecialBreaks(PRBool aSafeToAskFrames)
{
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  nsCOMPtr<nsISupports> isupports;
  PRInt32 nodeCount,j;
  
  
  nsEmptyFunctor functor(mHTMLEditor);
  nsDOMIterator iter;
  nsresult res = iter.Init(mDocChangeRange);
  if (NS_FAILED(res)) return res;
  res = iter.AppendList(functor, arrayOfNodes);
  if (NS_FAILED(res)) return res;

  
  nodeCount = arrayOfNodes.Count();
  for (j = 0; j < nodeCount; j++)
  {
    
    
    
    
    PRUint32 len;
    nsCOMPtr<nsIDOMNode> brNode, theNode = arrayOfNodes[0];
    arrayOfNodes.RemoveObjectAt(0);
    res = nsEditor::GetLengthOfDOMNode(theNode, len);
    if (NS_FAILED(res)) return res;
    res = CreateMozBR(theNode, (PRInt32)len, address_of(brNode));
    if (NS_FAILED(res)) return res;
  }
  
  return res;
}

nsresult 
nsHTMLEditRules::AdjustWhitespace(nsISelection *aSelection)
{
  
  nsCOMPtr<nsIDOMNode> selNode;
  PRInt32 selOffset;
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  
  
  return nsWSRunObject(mHTMLEditor, selNode, selOffset).AdjustWhitespace();
}

nsresult 
nsHTMLEditRules::PinSelectionToNewBlock(nsISelection *aSelection)
{
  if (!aSelection) return NS_ERROR_NULL_POINTER;
  PRBool bCollapsed;
  nsresult res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (!bCollapsed) return res;

  
  nsCOMPtr<nsIDOMNode> selNode, temp;
  PRInt32 selOffset;
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  temp = selNode;
  
  
  nsCOMPtr<nsIDOMRange> range = do_CreateInstance("@mozilla.org/content/range;1");
  res = range->SetStart(selNode, selOffset);
  if (NS_FAILED(res)) return res;
  res = range->SetEnd(selNode, selOffset);
  if (NS_FAILED(res)) return res;
  nsCOMPtr<nsIContent> block (do_QueryInterface(mNewBlock));
  if (!block) return NS_ERROR_NO_INTERFACE;
  PRBool nodeBefore, nodeAfter;
  res = mHTMLEditor->sRangeHelper->CompareNodeToRange(block, range, &nodeBefore, &nodeAfter);
  if (NS_FAILED(res)) return res;
  
  if (nodeBefore && nodeAfter)
    return NS_OK;  
  else if (nodeBefore)
  {
    
    nsCOMPtr<nsIDOMNode> tmp = mNewBlock;
    mHTMLEditor->GetLastEditableChild(mNewBlock, address_of(tmp));
    PRUint32 endPoint;
    if (mHTMLEditor->IsTextNode(tmp) || mHTMLEditor->IsContainer(tmp))
    {
      res = nsEditor::GetLengthOfDOMNode(tmp, endPoint);
      if (NS_FAILED(res)) return res;
    }
    else
    {
      nsCOMPtr<nsIDOMNode> tmp2;
      res = nsEditor::GetNodeLocation(tmp, address_of(tmp2), (PRInt32*)&endPoint);
      if (NS_FAILED(res)) return res;
      tmp = tmp2;
      endPoint++;  
    }
    return aSelection->Collapse(tmp, (PRInt32)endPoint);
  }
  else
  {
    
    nsCOMPtr<nsIDOMNode> tmp = mNewBlock;
    mHTMLEditor->GetFirstEditableChild(mNewBlock, address_of(tmp));
    PRInt32 offset;
    if (!(mHTMLEditor->IsTextNode(tmp) || mHTMLEditor->IsContainer(tmp)))
    {
      nsCOMPtr<nsIDOMNode> tmp2;
      res = nsEditor::GetNodeLocation(tmp, address_of(tmp2), &offset);
      if (NS_FAILED(res)) return res;
      tmp = tmp2;
    }
    return aSelection->Collapse(tmp, 0);
  }
}

nsresult 
nsHTMLEditRules::CheckInterlinePosition(nsISelection *aSelection)
{
  if (!aSelection) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelection> selection(aSelection);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  
  PRBool bCollapsed;
  nsresult res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (!bCollapsed) return res;

  
  nsCOMPtr<nsIDOMNode> selNode, node;
  PRInt32 selOffset;
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  
  
  mHTMLEditor->GetPriorHTMLSibling(selNode, selOffset, address_of(node));
  if (node && IsBlockNode(node))
  {
    selPriv->SetInterlinePosition(PR_TRUE);
    return NS_OK;
  }

  
  mHTMLEditor->GetNextHTMLSibling(selNode, selOffset, address_of(node));
  if (node && IsBlockNode(node))
  {
    selPriv->SetInterlinePosition(PR_FALSE);
    return NS_OK;
  }
  
  
  mHTMLEditor->GetPriorHTMLNode(selNode, selOffset, address_of(node), PR_TRUE);
  if (node && nsTextEditUtils::IsBreak(node))
      selPriv->SetInterlinePosition(PR_TRUE);
  return NS_OK;
}

nsresult 
nsHTMLEditRules::AdjustSelection(nsISelection *aSelection, nsIEditor::EDirection aAction)
{
  if (!aSelection) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelection> selection(aSelection);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
 
  
  
  
  PRBool bCollapsed;
  nsresult res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (!bCollapsed) return res;

  
  nsCOMPtr<nsIDOMNode> selNode, temp;
  PRInt32 selOffset;
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  temp = selNode;
  
  
  while (!mHTMLEditor->IsEditable(selNode))
  {
    
    res = nsEditor::GetNodeLocation(temp, address_of(selNode), &selOffset);
    if (NS_FAILED(res)) return res;
    if (!selNode) return NS_ERROR_FAILURE;
    temp = selNode;
  }
  
  
  
  nsCOMPtr<nsIDOMNode> theblock;
  if (IsBlockNode(selNode)) theblock = selNode;
  else theblock = mHTMLEditor->GetBlockNodeParent(selNode);
  PRBool bIsEmptyNode;
  res = mHTMLEditor->IsEmptyNode(theblock, &bIsEmptyNode, PR_FALSE, PR_FALSE);
  if (NS_FAILED(res)) return res;
  
  if (bIsEmptyNode && mHTMLEditor->CanContainTag(selNode, NS_LITERAL_STRING("br")))
  {
    nsIDOMElement *rootElement = mHTMLEditor->GetRoot();
    if (!rootElement) return NS_ERROR_FAILURE;
    nsCOMPtr<nsIDOMNode> rootNode(do_QueryInterface(rootElement));
    if (selNode == rootNode)
    {
      
      
      
      return NS_OK;
    }

    nsCOMPtr<nsIDOMNode> brNode;
    
    return CreateMozBR(selNode, selOffset, address_of(brNode));
  }
  
  
  nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(selNode);
  if (textNode)
    return NS_OK; 
  
  
  
  
  

  nsCOMPtr<nsIDOMNode> nearNode;
  res = mHTMLEditor->GetPriorHTMLNode(selNode, selOffset, address_of(nearNode));
  if (NS_FAILED(res)) return res;
  if (nearNode) 
  {
    
    nsCOMPtr<nsIDOMNode> block, nearBlock;
    if (IsBlockNode(selNode)) block = selNode;
    else block = mHTMLEditor->GetBlockNodeParent(selNode);
    nearBlock = mHTMLEditor->GetBlockNodeParent(nearNode);
    if (block == nearBlock)
    {
      if (nearNode && nsTextEditUtils::IsBreak(nearNode) )
      {   
        if (!mHTMLEditor->IsVisBreak(nearNode))
        {
          
          
          
          nsCOMPtr<nsIDOMNode> brNode;
          res = CreateMozBR(selNode, selOffset, address_of(brNode));
          if (NS_FAILED(res)) return res;
          res = nsEditor::GetNodeLocation(brNode, address_of(selNode), &selOffset);
          if (NS_FAILED(res)) return res;
          
          selPriv->SetInterlinePosition(PR_TRUE);
          res = aSelection->Collapse(selNode,selOffset);
          if (NS_FAILED(res)) return res;
        }
        else
        {
          nsCOMPtr<nsIDOMNode> nextNode;
          mHTMLEditor->GetNextHTMLNode(nearNode, address_of(nextNode), PR_TRUE);
          if (nextNode && nsTextEditUtils::IsMozBR(nextNode))
          {
            
            
            selPriv->SetInterlinePosition(PR_TRUE);
          }
        }
      }
    }
  }

  
  res = mHTMLEditor->GetPriorHTMLNode(selNode, selOffset, address_of(nearNode), PR_TRUE);
  if (NS_FAILED(res)) return res;
  if (nearNode && (nsTextEditUtils::IsBreak(nearNode)
                   || nsEditor::IsTextNode(nearNode)
                   || nsHTMLEditUtils::IsImage(nearNode)
                   || nsHTMLEditUtils::IsHR(nearNode)))
    return NS_OK; 
  res = mHTMLEditor->GetNextHTMLNode(selNode, selOffset, address_of(nearNode), PR_TRUE);
  if (NS_FAILED(res)) return res;
  if (nearNode && (nsTextEditUtils::IsBreak(nearNode)
                   || nsEditor::IsTextNode(nearNode)
                   || nsHTMLEditUtils::IsImage(nearNode)
                   || nsHTMLEditUtils::IsHR(nearNode)))
    return NS_OK; 

  
  
  res = FindNearSelectableNode(selNode, selOffset, aAction, address_of(nearNode));
  if (NS_FAILED(res)) return res;

  if (nearNode)
  {
    
    textNode = do_QueryInterface(nearNode);
    if (textNode)
    {
      PRInt32 offset = 0;
      
      if (aAction == nsIEditor::ePrevious)
        textNode->GetLength((PRUint32*)&offset);
      res = aSelection->Collapse(nearNode,offset);
    }
    else  
    {
      res = nsEditor::GetNodeLocation(nearNode, address_of(selNode), &selOffset);
      if (NS_FAILED(res)) return res;
      if (aAction == nsIEditor::ePrevious) selOffset++;  
      res = aSelection->Collapse(selNode, selOffset);
    }
  }
  return res;
}


nsresult
nsHTMLEditRules::FindNearSelectableNode(nsIDOMNode *aSelNode, 
                                        PRInt32 aSelOffset, 
                                        nsIEditor::EDirection &aDirection,
                                        nsCOMPtr<nsIDOMNode> *outSelectableNode)
{
  if (!aSelNode || !outSelectableNode) return NS_ERROR_NULL_POINTER;
  *outSelectableNode = nsnull;
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> nearNode, curNode;
  if (aDirection == nsIEditor::ePrevious)
    res = mHTMLEditor->GetPriorHTMLNode(aSelNode, aSelOffset, address_of(nearNode));
  else
    res = mHTMLEditor->GetNextHTMLNode(aSelNode, aSelOffset, address_of(nearNode));
  if (NS_FAILED(res)) return res;
  
  if (!nearNode) 
  {
    if (aDirection == nsIEditor::ePrevious)
      aDirection = nsIEditor::eNext;
    else
      aDirection = nsIEditor::ePrevious;
    
    if (aDirection == nsIEditor::ePrevious)
      res = mHTMLEditor->GetPriorHTMLNode(aSelNode, aSelOffset, address_of(nearNode));
    else
      res = mHTMLEditor->GetNextHTMLNode(aSelNode, aSelOffset, address_of(nearNode));
    if (NS_FAILED(res)) return res;
  }
  
  
  
  while (nearNode && !(mHTMLEditor->IsTextNode(nearNode)
                       || nsTextEditUtils::IsBreak(nearNode)
                       || nsHTMLEditUtils::IsImage(nearNode)))
  {
    curNode = nearNode;
    if (aDirection == nsIEditor::ePrevious)
      res = mHTMLEditor->GetPriorHTMLNode(curNode, address_of(nearNode));
    else
      res = mHTMLEditor->GetNextHTMLNode(curNode, address_of(nearNode));
    if (NS_FAILED(res)) return res;
  }
  
  if (nearNode)
  {
    
    PRBool bInDifTblElems;
    res = InDifferentTableElements(nearNode, aSelNode, &bInDifTblElems);
    if (NS_FAILED(res)) return res;
    if (bInDifTblElems) return NS_OK;  
    
    
    *outSelectableNode = do_QueryInterface(nearNode);
  }
  return res;
}


nsresult
nsHTMLEditRules::InDifferentTableElements(nsIDOMNode *aNode1, nsIDOMNode *aNode2, PRBool *aResult)
{
  NS_ASSERTION(aNode1 && aNode2 && aResult, "null args");
  if (!aNode1 || !aNode2 || !aResult) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> tn1, tn2, node = aNode1, temp;
  *aResult = PR_FALSE;
  
  while (node && !nsHTMLEditUtils::IsTableElement(node))
  {
    node->GetParentNode(getter_AddRefs(temp));
    node = temp;
  }
  tn1 = node;
  
  node = aNode2;
  while (node && !nsHTMLEditUtils::IsTableElement(node))
  {
    node->GetParentNode(getter_AddRefs(temp));
    node = temp;
  }
  tn2 = node;
  
  *aResult = (tn1 != tn2);
  
  return NS_OK;
}


nsresult 
nsHTMLEditRules::RemoveEmptyNodes()
{
  nsCOMArray<nsIDOMNode> arrayOfEmptyNodes, arrayOfEmptyCites;
  nsCOMPtr<nsISupports> isupports;
  PRInt32 nodeCount,j;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIContentIterator> iter =
                  do_CreateInstance("@mozilla.org/content/post-content-iterator;1");
  if (!iter) return NS_ERROR_NULL_POINTER;
  
  nsresult res = iter->Init(mDocChangeRange);
  if (NS_FAILED(res)) return res;
  
  nsVoidArray skipList;

  
  while (!iter->IsDone())
  {
    nsCOMPtr<nsIDOMNode> node, parent;

    node = do_QueryInterface(iter->GetCurrentNode());
    if (!node)
      return NS_ERROR_FAILURE;

    node->GetParentNode(getter_AddRefs(parent));
    
    PRInt32 idx = skipList.IndexOf((void*)node);
    if (idx>=0)
    {
      
      
      skipList.ReplaceElementAt((void*)parent, idx);
    }
    else
    {
      PRBool bIsCandidate = PR_FALSE;
      PRBool bIsEmptyNode = PR_FALSE;
      PRBool bIsMailCite = PR_FALSE;

      
      if (!nsTextEditUtils::IsBody(node))
      {
        
        if (  (bIsMailCite = nsHTMLEditUtils::IsMailCite(node))  ||
              nsEditor::NodeIsType(node, nsEditProperty::a)      ||
              nsHTMLEditUtils::IsInlineStyle(node)               ||
              nsHTMLEditUtils::IsList(node)                      ||
              nsHTMLEditUtils::IsDiv(node)  )
        {
          bIsCandidate = PR_TRUE;
        }
        
        else if (nsHTMLEditUtils::IsFormatNode(node) ||
            nsHTMLEditUtils::IsListItem(node)  ||
            nsHTMLEditUtils::IsBlockquote(node) )
        {
          
          
          
          PRBool bIsSelInNode;
          res = SelectionEndpointInNode(node, &bIsSelInNode);
          if (NS_FAILED(res)) return res;
          if (!bIsSelInNode)
          {
            bIsCandidate = PR_TRUE;
          }
        }
      }
      
      if (bIsCandidate)
      {
        if (bIsMailCite)  
          res = mHTMLEditor->IsEmptyNode(node, &bIsEmptyNode, PR_TRUE, PR_TRUE);  
        else  
          res = mHTMLEditor->IsEmptyNode(node, &bIsEmptyNode, PR_FALSE, PR_TRUE);
        if (NS_FAILED(res)) return res;
        if (bIsEmptyNode)
        {
          if (bIsMailCite)  
          {
            arrayOfEmptyCites.AppendObject(node);
          }
          else
          {
            arrayOfEmptyNodes.AppendObject(node);
          }
        }
      }
      
      if (!bIsEmptyNode)
      {
        
        skipList.AppendElement((void*)parent);
      }
    }

    iter->Next();
  }
  
  
  nodeCount = arrayOfEmptyNodes.Count();
  for (j = 0; j < nodeCount; j++)
  {
    nsCOMPtr<nsIDOMNode> delNode = arrayOfEmptyNodes[0];
    arrayOfEmptyNodes.RemoveObjectAt(0);
    res = mHTMLEditor->DeleteNode(delNode);
    if (NS_FAILED(res)) return res;
  }
  
  
  
  nodeCount = arrayOfEmptyCites.Count();
  for (j = 0; j < nodeCount; j++)
  {
    nsCOMPtr<nsIDOMNode> delNode = arrayOfEmptyCites[0];
    arrayOfEmptyCites.RemoveObjectAt(0);
    PRBool bIsEmptyNode;
    res = mHTMLEditor->IsEmptyNode(delNode, &bIsEmptyNode, PR_FALSE, PR_TRUE);
    if (NS_FAILED(res)) return res;
    if (!bIsEmptyNode)
    {
      
      
      nsCOMPtr<nsIDOMNode> parent, brNode;
      PRInt32 offset;
      res = nsEditor::GetNodeLocation(delNode, address_of(parent), &offset);
      if (NS_FAILED(res)) return res;
      res = mHTMLEditor->CreateBR(parent, offset, address_of(brNode));
      if (NS_FAILED(res)) return res;
    }
    res = mHTMLEditor->DeleteNode(delNode);
    if (NS_FAILED(res)) return res;
  }
  
  return res;
}

nsresult
nsHTMLEditRules::SelectionEndpointInNode(nsIDOMNode *aNode, PRBool *aResult)
{
  if (!aNode || !aResult) return NS_ERROR_NULL_POINTER;
  
  *aResult = PR_FALSE;
  
  nsCOMPtr<nsISelection>selection;
  nsresult res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  nsCOMPtr<nsISelectionPrivate>selPriv(do_QueryInterface(selection));
  
  nsCOMPtr<nsIEnumerator> enumerator;
  res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
  if (NS_FAILED(res)) return res;
  if (!enumerator) return NS_ERROR_UNEXPECTED;

  for (enumerator->First(); NS_OK!=enumerator->IsDone(); enumerator->Next())
  {
    nsCOMPtr<nsISupports> currentItem;
    res = enumerator->CurrentItem(getter_AddRefs(currentItem));
    if (NS_FAILED(res)) return res;
    if (!currentItem) return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
    nsCOMPtr<nsIDOMNode> startParent, endParent;
    range->GetStartContainer(getter_AddRefs(startParent));
    if (startParent)
    {
      if (aNode == startParent)
      {
        *aResult = PR_TRUE;
        return NS_OK;
      }
      if (nsEditorUtils::IsDescendantOf(startParent, aNode)) 
      {
        *aResult = PR_TRUE;
        return NS_OK;
      }
    }
    range->GetEndContainer(getter_AddRefs(endParent));
    if (startParent == endParent) continue;
    if (endParent)
    {
      if (aNode == endParent) 
      {
        *aResult = PR_TRUE;
        return NS_OK;
      }
      if (nsEditorUtils::IsDescendantOf(endParent, aNode))
      {
        *aResult = PR_TRUE;
        return NS_OK;
      }
    }
  }
  return res;
}





PRBool 
nsHTMLEditRules::IsEmptyInline(nsIDOMNode *aNode)
{
  if (aNode && IsInlineNode(aNode) && mHTMLEditor->IsContainer(aNode)) 
  {
    PRBool bEmpty;
    mHTMLEditor->IsEmptyNode(aNode, &bEmpty);
    return bEmpty;
  }
  return PR_FALSE;
}


PRBool 
nsHTMLEditRules::ListIsEmptyLine(nsCOMArray<nsIDOMNode> &arrayOfNodes)
{
  
  
  
  PRInt32 listCount = arrayOfNodes.Count();
  if (!listCount) return PR_TRUE;
  nsCOMPtr<nsIDOMNode> somenode;
  PRInt32 j, brCount=0;
  for (j = 0; j < listCount; j++)
  {
    somenode = arrayOfNodes[j];
    if (somenode && mHTMLEditor->IsEditable(somenode))
    {
      if (nsTextEditUtils::IsBreak(somenode))
      {
        
        if (brCount) return PR_FALSE;
        brCount++;
      }
      else if (IsEmptyInline(somenode)) 
      {
        
      }
      else return PR_FALSE;
    }
  }
  return PR_TRUE;
}


nsresult 
nsHTMLEditRules::PopListItem(nsIDOMNode *aListItem, PRBool *aOutOfList)
{
  
  if (!aListItem || !aOutOfList) 
    return NS_ERROR_NULL_POINTER;
  
  
  *aOutOfList = PR_FALSE;
  
  nsCOMPtr<nsIDOMNode> curParent;
  nsCOMPtr<nsIDOMNode> curNode( do_QueryInterface(aListItem));
  PRInt32 offset;
  nsresult res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
  if (NS_FAILED(res)) return res;
    
  if (!nsHTMLEditUtils::IsListItem(curNode))
    return NS_ERROR_FAILURE;
    
  
  
  nsCOMPtr<nsIDOMNode> curParPar;
  PRInt32 parOffset;
  res = nsEditor::GetNodeLocation(curParent, address_of(curParPar), &parOffset);
  if (NS_FAILED(res)) return res;
  
  PRBool bIsFirstListItem;
  res = mHTMLEditor->IsFirstEditableChild(curNode, &bIsFirstListItem);
  if (NS_FAILED(res)) return res;

  PRBool bIsLastListItem;
  res = mHTMLEditor->IsLastEditableChild(curNode, &bIsLastListItem);
  if (NS_FAILED(res)) return res;
    
  if (!bIsFirstListItem && !bIsLastListItem)
  {
    
    nsCOMPtr<nsIDOMNode> newBlock;
    res = mHTMLEditor->SplitNode(curParent, offset, getter_AddRefs(newBlock));
    if (NS_FAILED(res)) return res;
  }
  
  if (!bIsFirstListItem) parOffset++;
  
  res = mHTMLEditor->MoveNode(curNode, curParPar, parOffset);
  if (NS_FAILED(res)) return res;
    
  
  if (!nsHTMLEditUtils::IsList(curParPar)
      && nsHTMLEditUtils::IsListItem(curNode)) 
  {
    res = mHTMLEditor->RemoveBlockContainer(curNode);
    if (NS_FAILED(res)) return res;
    *aOutOfList = PR_TRUE;
  }
  return res;
}

nsresult
nsHTMLEditRules::RemoveListStructure(nsIDOMNode *aList)
{
  NS_ENSURE_ARG_POINTER(aList);

  nsresult res;

  nsCOMPtr<nsIDOMNode> child;
  aList->GetFirstChild(getter_AddRefs(child));

  while (child)
  {
    if (nsHTMLEditUtils::IsListItem(child))
    {
      PRBool bOutOfList;
      do
      {
        res = PopListItem(child, &bOutOfList);
        if (NS_FAILED(res)) return res;
      } while (!bOutOfList);   
    }
    else if (nsHTMLEditUtils::IsList(child))
    {
      res = RemoveListStructure(child);
      if (NS_FAILED(res)) return res;
    }
    else
    {
      
      res = mHTMLEditor->DeleteNode(child);
      if (NS_FAILED(res)) return res;
    }
    aList->GetFirstChild(getter_AddRefs(child));
  }
  
  res = mHTMLEditor->RemoveBlockContainer(aList);
  if (NS_FAILED(res)) return res;

  return res;
}


nsresult 
nsHTMLEditRules::ConfirmSelectionInBody()
{
  nsresult res = NS_OK;

  
  nsIDOMElement *rootElement = mHTMLEditor->GetRoot();
  if (!rootElement) return NS_ERROR_UNEXPECTED;

  
  nsCOMPtr<nsISelection>selection;
  res = mHTMLEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  
  
  nsCOMPtr<nsIDOMNode> selNode, temp, parent;
  PRInt32 selOffset;
  res = mHTMLEditor->GetStartNodeAndOffset(selection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  temp = selNode;
  
  
  while (temp && !nsTextEditUtils::IsBody(temp))
  {
    res = temp->GetParentNode(getter_AddRefs(parent));
    temp = parent;
  }
  
  
  if (!temp) 
  {


    selection->Collapse(rootElement, 0);
  }
  
  
  res = mHTMLEditor->GetEndNodeAndOffset(selection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  temp = selNode;
  
  
  while (temp && !nsTextEditUtils::IsBody(temp))
  {
    res = temp->GetParentNode(getter_AddRefs(parent));
    temp = parent;
  }
  
  
  if (!temp) 
  {


    selection->Collapse(rootElement, 0);
  }
  
  return res;
}


nsresult 
nsHTMLEditRules::UpdateDocChangeRange(nsIDOMRange *aRange)
{
  nsresult res = NS_OK;

  
  
  
  
  nsCOMPtr<nsIDOMNode> startNode;
  res = aRange->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(res)) return res;
  if (!mHTMLEditor->IsDescendantOfBody(startNode))
  {
    
    return NS_OK;
  }
  
  if (!mDocChangeRange)
  {
    
    res = aRange->CloneRange(getter_AddRefs(mDocChangeRange));
    return res;
  }
  else
  {
    PRInt16 result;
    
    
    res = mDocChangeRange->CompareBoundaryPoints(nsIDOMRange::START_TO_START, aRange, &result);
    if (res == NS_ERROR_NOT_INITIALIZED) {
      
      
      
      
      result = 1;
      res = NS_OK;
    }
    if (NS_FAILED(res)) return res;
    if (result > 0)  
    {
      PRInt32 startOffset;
      res = aRange->GetStartOffset(&startOffset);
      if (NS_FAILED(res)) return res;
      res = mDocChangeRange->SetStart(startNode, startOffset);
      if (NS_FAILED(res)) return res;
    }
    
    
    res = mDocChangeRange->CompareBoundaryPoints(nsIDOMRange::END_TO_END, aRange, &result);
    if (NS_FAILED(res)) return res;
    if (result < 0)  
    {
      nsCOMPtr<nsIDOMNode> endNode;
      PRInt32 endOffset;
      res = aRange->GetEndContainer(getter_AddRefs(endNode));
      if (NS_FAILED(res)) return res;
      res = aRange->GetEndOffset(&endOffset);
      if (NS_FAILED(res)) return res;
      res = mDocChangeRange->SetEnd(endNode, endOffset);
      if (NS_FAILED(res)) return res;
    }
  }
  return res;
}

nsresult 
nsHTMLEditRules::InsertMozBRIfNeeded(nsIDOMNode *aNode)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;
  if (!IsBlockNode(aNode)) return NS_OK;
  
  PRBool isEmpty;
  nsCOMPtr<nsIDOMNode> brNode;
  nsresult res = mHTMLEditor->IsEmptyNode(aNode, &isEmpty);
  if (NS_FAILED(res)) return res;
  if (isEmpty)
  {
    res = CreateMozBR(aNode, 0, address_of(brNode));
  }
  return res;
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  nsIEditActionListener methods 
#pragma mark -
#endif

NS_IMETHODIMP 
nsHTMLEditRules::WillCreateNode(const nsAString& aTag, nsIDOMNode *aParent, PRInt32 aPosition)
{
  return NS_OK;  
}

NS_IMETHODIMP 
nsHTMLEditRules::DidCreateNode(const nsAString& aTag, 
                               nsIDOMNode *aNode, 
                               nsIDOMNode *aParent, 
                               PRInt32 aPosition, 
                               nsresult aResult)
{
  if (!mListenerEnabled) return NS_OK;
  
  nsresult res = mUtilRange->SelectNode(aNode);
  if (NS_FAILED(res)) return res;
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aPosition)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidInsertNode(nsIDOMNode *aNode, 
                               nsIDOMNode *aParent, 
                               PRInt32 aPosition, 
                               nsresult aResult)
{
  if (!mListenerEnabled) return NS_OK;
  nsresult res = mUtilRange->SelectNode(aNode);
  if (NS_FAILED(res)) return res;
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillDeleteNode(nsIDOMNode *aChild)
{
  if (!mListenerEnabled) return NS_OK;
  nsresult res = mUtilRange->SelectNode(aChild);
  if (NS_FAILED(res)) return res;
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidDeleteNode(nsIDOMNode *aChild, nsresult aResult)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillSplitNode(nsIDOMNode *aExistingRightNode, PRInt32 aOffset)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidSplitNode(nsIDOMNode *aExistingRightNode, 
                              PRInt32 aOffset, 
                              nsIDOMNode *aNewLeftNode, 
                              nsresult aResult)
{
  if (!mListenerEnabled) return NS_OK;
  nsresult res = mUtilRange->SetStart(aNewLeftNode, 0);
  if (NS_FAILED(res)) return res;
  res = mUtilRange->SetEnd(aExistingRightNode, 0);
  if (NS_FAILED(res)) return res;
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillJoinNodes(nsIDOMNode *aLeftNode, nsIDOMNode *aRightNode, nsIDOMNode *aParent)
{
  if (!mListenerEnabled) return NS_OK;
  
  nsresult res = nsEditor::GetLengthOfDOMNode(aLeftNode, mJoinOffset);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidJoinNodes(nsIDOMNode  *aLeftNode, 
                              nsIDOMNode *aRightNode, 
                              nsIDOMNode *aParent, 
                              nsresult aResult)
{
  if (!mListenerEnabled) return NS_OK;
  
  nsresult res = mUtilRange->SetStart(aRightNode, mJoinOffset);
  if (NS_FAILED(res)) return res;
  res = mUtilRange->SetEnd(aRightNode, mJoinOffset);
  if (NS_FAILED(res)) return res;
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsAString &aString)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidInsertText(nsIDOMCharacterData *aTextNode, 
                                  PRInt32 aOffset, 
                                  const nsAString &aString, 
                                  nsresult aResult)
{
  if (!mListenerEnabled) return NS_OK;
  PRInt32 length = aString.Length();
  nsCOMPtr<nsIDOMNode> theNode = do_QueryInterface(aTextNode);
  nsresult res = mUtilRange->SetStart(theNode, aOffset);
  if (NS_FAILED(res)) return res;
  res = mUtilRange->SetEnd(theNode, aOffset+length);
  if (NS_FAILED(res)) return res;
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidDeleteText(nsIDOMCharacterData *aTextNode, 
                                  PRInt32 aOffset, 
                                  PRInt32 aLength, 
                                  nsresult aResult)
{
  if (!mListenerEnabled) return NS_OK;
  nsCOMPtr<nsIDOMNode> theNode = do_QueryInterface(aTextNode);
  nsresult res = mUtilRange->SetStart(theNode, aOffset);
  if (NS_FAILED(res)) return res;
  res = mUtilRange->SetEnd(theNode, aOffset);
  if (NS_FAILED(res)) return res;
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}

NS_IMETHODIMP
nsHTMLEditRules::WillDeleteRange(nsIDOMRange *aRange)
{
  if (!mListenerEnabled) return NS_OK;
  
  return UpdateDocChangeRange(aRange);
}

NS_IMETHODIMP
nsHTMLEditRules::DidDeleteRange(nsIDOMRange *aRange)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditRules::WillDeleteSelection(nsISelection *aSelection)
{
  if (!mListenerEnabled) return NS_OK;
  
  nsCOMPtr<nsIDOMNode> selNode;
  PRInt32 selOffset;

  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  res = mUtilRange->SetStart(selNode, selOffset);
  if (NS_FAILED(res)) return res;
  res = mHTMLEditor->GetEndNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  res = mUtilRange->SetEnd(selNode, selOffset);
  if (NS_FAILED(res)) return res;
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}

NS_IMETHODIMP
nsHTMLEditRules::DidDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}





nsresult
nsHTMLEditRules::RemoveAlignment(nsIDOMNode * aNode, const nsAString & aAlignType, PRBool aChildrenOnly)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;

  if (mHTMLEditor->IsTextNode(aNode) || nsHTMLEditUtils::IsTable(aNode)) return NS_OK;
  nsresult res = NS_OK;

  nsCOMPtr<nsIDOMNode> child = aNode,tmp;
  if (aChildrenOnly)
  {
    aNode->GetFirstChild(getter_AddRefs(child));
  }
  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);

  while (child)
  {
    if (aChildrenOnly) {
      
      child->GetNextSibling(getter_AddRefs(tmp));
    }
    else
    {
      tmp = nsnull;
    }
    PRBool isBlock;
    res = mHTMLEditor->NodeIsBlockStatic(child, &isBlock);
    if (NS_FAILED(res)) return res;

    if ((isBlock && !nsHTMLEditUtils::IsDiv(child)) || nsHTMLEditUtils::IsHR(child))
    {
      
      nsCOMPtr<nsIDOMElement> curElem = do_QueryInterface(child);
      if (nsHTMLEditUtils::SupportsAlignAttr(child))
      {
        
        res = mHTMLEditor->RemoveAttribute(curElem, NS_LITERAL_STRING("align"));
        if (NS_FAILED(res)) return res;
      }
      if (useCSS)
      {
        if (nsHTMLEditUtils::IsTable(child) || nsHTMLEditUtils::IsHR(child))
        {
          res = mHTMLEditor->SetAttributeOrEquivalent(curElem, NS_LITERAL_STRING("align"), aAlignType, PR_FALSE); 
        }
        else
        {
          nsAutoString dummyCssValue;
          res = mHTMLEditor->mHTMLCSSUtils->RemoveCSSInlineStyle(child, nsEditProperty::cssTextAlign, dummyCssValue);
        }
        if (NS_FAILED(res)) return res;
      }
      if (!nsHTMLEditUtils::IsTable(child))
      {
        
        res = RemoveAlignment(child, aAlignType, PR_TRUE);
        if (NS_FAILED(res)) return res;
      }
    }
    else if (nsEditor::NodeIsType(child, nsEditProperty::center)
             || nsHTMLEditUtils::IsDiv(child))
    {
      
      
      res = RemoveAlignment(child, aAlignType, PR_TRUE);
      if (NS_FAILED(res)) return res;

      if (useCSS && nsHTMLEditUtils::IsDiv(child))
      {
        
        
        nsAutoString dummyCssValue;
        res = mHTMLEditor->mHTMLCSSUtils->RemoveCSSInlineStyle(child, nsEditProperty::cssTextAlign, dummyCssValue);
        if (NS_FAILED(res)) return res;
        nsCOMPtr<nsIDOMElement> childElt = do_QueryInterface(child);
        PRBool hasStyleOrIdOrClass;
        res = mHTMLEditor->HasStyleOrIdOrClass(childElt, &hasStyleOrIdOrClass);
        if (NS_FAILED(res)) return res;
        if (!hasStyleOrIdOrClass)
        {
          
          
          res = MakeSureElemStartsOrEndsOnCR(child);
          if (NS_FAILED(res)) return res;
          res = mHTMLEditor->RemoveContainer(child);
          if (NS_FAILED(res)) return res;
        }
      }
      else
      {
        
        
        res = MakeSureElemStartsOrEndsOnCR(child);
        if (NS_FAILED(res)) return res;

        
        res = mHTMLEditor->RemoveContainer(child);
        if (NS_FAILED(res)) return res;
      }
    }
    child = tmp;
  }
  return NS_OK;
}




nsresult
nsHTMLEditRules::MakeSureElemStartsOrEndsOnCR(nsIDOMNode *aNode, PRBool aStarts)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> child;
  nsresult res;
  if (aStarts)
  {
    res = mHTMLEditor->GetFirstEditableChild(aNode, address_of(child));
  }
  else
  {
    res = mHTMLEditor->GetLastEditableChild(aNode, address_of(child));
  }
  if (NS_FAILED(res)) return res;
  if (!child) return NS_OK;
  PRBool isChildBlock;
  res = mHTMLEditor->NodeIsBlockStatic(child, &isChildBlock);
  if (NS_FAILED(res)) return res;
  PRBool foundCR = PR_FALSE;
  if (isChildBlock || nsTextEditUtils::IsBreak(child))
  {
    foundCR = PR_TRUE;
  }
  else
  {
    nsCOMPtr<nsIDOMNode> sibling;
    if (aStarts)
    {
      res = mHTMLEditor->GetPriorHTMLSibling(aNode, address_of(sibling));
    }
    else
    {
      res = mHTMLEditor->GetNextHTMLSibling(aNode, address_of(sibling));
    }
    if (NS_FAILED(res)) return res;
    if (sibling)
    {
      PRBool isBlock;
      res = mHTMLEditor->NodeIsBlockStatic(sibling, &isBlock);
      if (NS_FAILED(res)) return res;
      if (isBlock || nsTextEditUtils::IsBreak(sibling))
      {
        foundCR = PR_TRUE;
      }
    }
    else
    {
      foundCR = PR_TRUE;
    }
  }
  if (!foundCR)
  {
    nsCOMPtr<nsIDOMNode> brNode;
    PRInt32 offset = 0;
    if (!aStarts)
    {
      nsCOMPtr<nsIDOMNodeList> childNodes;
      res = aNode->GetChildNodes(getter_AddRefs(childNodes));
      if (NS_FAILED(res)) return res;
      if (!childNodes) return NS_ERROR_NULL_POINTER;
      PRUint32 childCount;
      res = childNodes->GetLength(&childCount);
      if (NS_FAILED(res)) return res;
      offset = childCount;
    }
    res = mHTMLEditor->CreateBR(aNode, offset, address_of(brNode));
    if (NS_FAILED(res)) return res;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditRules::MakeSureElemStartsOrEndsOnCR(nsIDOMNode *aNode)
{
  nsresult res = MakeSureElemStartsOrEndsOnCR(aNode, PR_FALSE);
  if (NS_FAILED(res)) return res;
  res = MakeSureElemStartsOrEndsOnCR(aNode, PR_TRUE);
  return res;
}

nsresult
nsHTMLEditRules::AlignBlock(nsIDOMElement * aElement, const nsAString * aAlignType, PRBool aContentsOnly)
{
  if (!aElement) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
  PRBool isBlock = IsBlockNode(node);
  if (!isBlock && !nsHTMLEditUtils::IsHR(node)) {
    
    return NS_OK;
  }

  nsresult res = RemoveAlignment(node, *aAlignType, aContentsOnly);
  if (NS_FAILED(res)) return res;
  NS_NAMED_LITERAL_STRING(attr, "align");
  PRBool useCSS;
  mHTMLEditor->GetIsCSSEnabled(&useCSS);
  if (useCSS) {
    
    
    res = mHTMLEditor->SetAttributeOrEquivalent(aElement, attr, *aAlignType, PR_FALSE); 
    if (NS_FAILED(res)) return res;
  }
  else {
    
    
    if (nsHTMLEditUtils::SupportsAlignAttr(node)) {
      res = mHTMLEditor->SetAttribute(aElement, attr, *aAlignType);
      if (NS_FAILED(res)) return res;
    }
  }
  return NS_OK;
}

nsresult
nsHTMLEditRules::RelativeChangeIndentationOfElementNode(nsIDOMNode *aNode, PRInt8 aRelativeChange)
{
  NS_ENSURE_ARG_POINTER(aNode);

  if ( !( (aRelativeChange==1) || (aRelativeChange==-1) ) )
    return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
  NS_ASSERTION(element, "not an element node");

  if (element) {
    nsIAtom* marginProperty = MarginPropertyAtomForIndent(mHTMLEditor->mHTMLCSSUtils, element);    
    nsAutoString value;
    nsresult res;
    mHTMLEditor->mHTMLCSSUtils->GetSpecifiedProperty(aNode, marginProperty, value);
    float f;
    nsIAtom * unit;
    mHTMLEditor->mHTMLCSSUtils->ParseLength(value, &f, &unit);
    if (0 == f) {
      NS_IF_RELEASE(unit);
      nsAutoString defaultLengthUnit;
      mHTMLEditor->mHTMLCSSUtils->GetDefaultLengthUnit(defaultLengthUnit);
      unit = NS_NewAtom(defaultLengthUnit);
    }
    nsAutoString unitString;
    unit->ToString(unitString);
    if      (nsEditProperty::cssInUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_IN * aRelativeChange;
    else if (nsEditProperty::cssCmUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_CM * aRelativeChange;
    else if (nsEditProperty::cssMmUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_MM * aRelativeChange;
    else if (nsEditProperty::cssPtUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_PT * aRelativeChange;
    else if (nsEditProperty::cssPcUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_PC * aRelativeChange;
    else if (nsEditProperty::cssEmUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_EM * aRelativeChange;
    else if (nsEditProperty::cssExUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_EX * aRelativeChange;
    else if (nsEditProperty::cssPxUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_PX * aRelativeChange;
    else if (nsEditProperty::cssPercentUnit == unit)
              f += NS_EDITOR_INDENT_INCREMENT_PERCENT * aRelativeChange;    

    NS_IF_RELEASE(unit);

    if (0 < f) {
      nsAutoString newValue;
      newValue.AppendFloat(f);
      newValue.Append(unitString);
      mHTMLEditor->mHTMLCSSUtils->SetCSSProperty(element, marginProperty, newValue, PR_FALSE);
    }
    else {
      mHTMLEditor->mHTMLCSSUtils->RemoveCSSProperty(element, marginProperty, value, PR_FALSE);
      if (nsHTMLEditUtils::IsDiv(aNode)) {
        
        nsCOMPtr<nsIDOMNamedNodeMap> attributeList;
        res = element->GetAttributes(getter_AddRefs(attributeList));
        if (NS_FAILED(res)) return res;
        PRUint32 count;
        attributeList->GetLength(&count);
        if (!count) {
          
          res = mHTMLEditor->RemoveContainer(element);
          if (NS_FAILED(res)) return res;
        }
        else if (1 == count) {
          nsCOMPtr<nsIDOMNode> styleAttributeNode;
          res = attributeList->GetNamedItem(NS_LITERAL_STRING("style"), 
                                            getter_AddRefs(styleAttributeNode));
          if (!styleAttributeNode) {
            res = mHTMLEditor->RemoveContainer(element);
            if (NS_FAILED(res)) return res;
          }
        }
      }
    }
  }
  return NS_OK;
}





nsresult
nsHTMLEditRules::WillAbsolutePosition(nsISelection *aSelection, PRBool *aCancel, PRBool * aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  nsresult res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;

  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;
  
  nsCOMPtr<nsIDOMElement> focusElement;
  res = mHTMLEditor->GetSelectionContainer(getter_AddRefs(focusElement));
  if (focusElement) {
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(focusElement);
    if (nsHTMLEditUtils::IsImage(node)) {
      mNewBlock = node;
      return NS_OK;
    }
  }

  res = NormalizeSelection(aSelection);
  if (NS_FAILED(res)) return res;
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  
  
  
  
  
  
  nsCOMArray<nsIDOMRange> arrayOfRanges;
  res = GetPromotedRanges(aSelection, arrayOfRanges, kSetAbsolutePosition);
  if (NS_FAILED(res)) return res;
  
  
  nsCOMArray<nsIDOMNode> arrayOfNodes;
  res = GetNodesForOperation(arrayOfRanges, arrayOfNodes, kSetAbsolutePosition);
  if (NS_FAILED(res)) return res;                                 
                                     
  NS_NAMED_LITERAL_STRING(divType, "div");


  
  if (ListIsEmptyLine(arrayOfNodes))
  {
    nsCOMPtr<nsIDOMNode> parent, thePositionedDiv;
    PRInt32 offset;
    
    
    res = mHTMLEditor->GetStartNodeAndOffset(aSelection, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    
    res = SplitAsNeeded(&divType, address_of(parent), &offset);
    if (NS_FAILED(res)) return res;
    res = mHTMLEditor->CreateNode(divType, parent, offset, getter_AddRefs(thePositionedDiv));
    if (NS_FAILED(res)) return res;
    
    mNewBlock = thePositionedDiv;
    
    for (PRInt32 j = arrayOfNodes.Count() - 1; j >= 0; --j) 
    {
      nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[0];
      res = mHTMLEditor->DeleteNode(curNode);
      if (NS_FAILED(res)) return res;
      res = arrayOfNodes.RemoveObjectAt(0);
      if (NS_FAILED(res)) return res;
    }
    
    res = aSelection->Collapse(thePositionedDiv,0);
    selectionResetter.Abort();  
    *aHandled = PR_TRUE;
    return res;
  }

  
  
  PRInt32 i;
  nsCOMPtr<nsIDOMNode> curParent, curPositionedDiv, curList, indentedLI, sibling;
  PRInt32 listCount = arrayOfNodes.Count();
  for (i=0; i<listCount; i++)
  {
    
    nsCOMPtr<nsIDOMNode> curNode = arrayOfNodes[i];

    
    if (!mHTMLEditor->IsEditable(curNode)) continue;

    PRInt32 offset;
    res = nsEditor::GetNodeLocation(curNode, address_of(curParent), &offset);
    if (NS_FAILED(res)) return res;
     
    
    if (nsHTMLEditUtils::IsList(curParent))
    {
      
      
      if (curList)
      {
        sibling = nsnull;
        mHTMLEditor->GetPriorHTMLSibling(curNode, address_of(sibling));
      }
      
      if (!curList || (sibling && sibling != curList) )
      {
        nsAutoString listTag;
        nsEditor::GetTagString(curParent,listTag);
        ToLowerCase(listTag);
        
        res = SplitAsNeeded(&listTag, address_of(curParent), &offset);
        if (NS_FAILED(res)) return res;
        if (!curPositionedDiv) {
          PRInt32 parentOffset;
          nsCOMPtr<nsIDOMNode> curParentParent;
          res = nsEditor::GetNodeLocation(curParent, address_of(curParentParent), &parentOffset);
          res = mHTMLEditor->CreateNode(divType, curParentParent, parentOffset, getter_AddRefs(curPositionedDiv));
          mNewBlock = curPositionedDiv;
        }
        res = mHTMLEditor->CreateNode(listTag, curPositionedDiv, -1, getter_AddRefs(curList));
        if (NS_FAILED(res)) return res;
        
        
        
      }
      
      res = mHTMLEditor->MoveNode(curNode, curList, -1);
      if (NS_FAILED(res)) return res;
      
      
    }
    
    else 
    {
      
      
      
      
      
      
      nsCOMPtr<nsIDOMNode> listitem=IsInListItem(curNode);
      if (listitem)
      {
        if (indentedLI == listitem) continue;  
        res = nsEditor::GetNodeLocation(listitem, address_of(curParent), &offset);
        if (NS_FAILED(res)) return res;
        
        
        if (curList)
        {
          sibling = nsnull;
          mHTMLEditor->GetPriorHTMLSibling(curNode, address_of(sibling));
        }
         
        if (!curList || (sibling && sibling != curList) )
        {
          nsAutoString listTag;
          nsEditor::GetTagString(curParent,listTag);
          ToLowerCase(listTag);
          
          res = SplitAsNeeded(&listTag, address_of(curParent), &offset);
          if (NS_FAILED(res)) return res;
          if (!curPositionedDiv) {
          PRInt32 parentOffset;
          nsCOMPtr<nsIDOMNode> curParentParent;
          res = nsEditor::GetNodeLocation(curParent, address_of(curParentParent), &parentOffset);
          res = mHTMLEditor->CreateNode(divType, curParentParent, parentOffset, getter_AddRefs(curPositionedDiv));
            mNewBlock = curPositionedDiv;
          }
          res = mHTMLEditor->CreateNode(listTag, curPositionedDiv, -1, getter_AddRefs(curList));
          if (NS_FAILED(res)) return res;
        }
        res = mHTMLEditor->MoveNode(listitem, curList, -1);
        if (NS_FAILED(res)) return res;
        
        indentedLI = listitem;
      }
      
      else
      {
        

        if (!curPositionedDiv) 
        {
          if (nsHTMLEditUtils::IsDiv(curNode))
          {
            curPositionedDiv = curNode;
            mNewBlock = curPositionedDiv;
            curList = nsnull;
            continue;
          }
          res = SplitAsNeeded(&divType, address_of(curParent), &offset);
          if (NS_FAILED(res)) return res;
          res = mHTMLEditor->CreateNode(divType, curParent, offset, getter_AddRefs(curPositionedDiv));
          if (NS_FAILED(res)) return res;
          
          mNewBlock = curPositionedDiv;
          
        }
          
        
        res = mHTMLEditor->MoveNode(curNode, curPositionedDiv, -1);
        if (NS_FAILED(res)) return res;
        
        curList = nsnull;
      }
    }
  }
  return res;
}

nsresult
nsHTMLEditRules::DidAbsolutePosition()
{
  nsCOMPtr<nsIHTMLAbsPosEditor> absPosHTMLEditor = mHTMLEditor;
  nsCOMPtr<nsIDOMElement> elt = do_QueryInterface(mNewBlock);
  return absPosHTMLEditor->AbsolutelyPositionElement(elt, PR_TRUE);
}

nsresult
nsHTMLEditRules::WillRemoveAbsolutePosition(nsISelection *aSelection, PRBool *aCancel, PRBool * aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  nsresult res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;

  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;

  nsCOMPtr<nsIDOMElement>  elt;
  res = mHTMLEditor->GetAbsolutelyPositionedSelectionContainer(getter_AddRefs(elt));
  if (NS_FAILED(res)) return res;

  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);

  nsCOMPtr<nsIHTMLAbsPosEditor> absPosHTMLEditor = mHTMLEditor;
  return absPosHTMLEditor->AbsolutelyPositionElement(elt, PR_FALSE);
}

nsresult
nsHTMLEditRules::WillRelativeChangeZIndex(nsISelection *aSelection,
                                          PRInt32 aChange,
                                          PRBool *aCancel,
                                          PRBool * aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  nsresult res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;

  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;

  nsCOMPtr<nsIDOMElement>  elt;
  res = mHTMLEditor->GetAbsolutelyPositionedSelectionContainer(getter_AddRefs(elt));
  if (NS_FAILED(res)) return res;

  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);

  nsCOMPtr<nsIHTMLAbsPosEditor> absPosHTMLEditor = mHTMLEditor;
  PRInt32 zIndex;
  return absPosHTMLEditor->RelativeChangeElementZIndex(elt, aChange, &zIndex);
}
