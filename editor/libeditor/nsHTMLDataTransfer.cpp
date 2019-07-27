





#include <string.h>

#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/Base64.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Selection.h"
#include "nsAString.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsDependentSubstring.h"
#include "nsEditRules.h"
#include "nsEditor.h"
#include "nsEditorUtils.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsHTMLEditUtils.h"
#include "nsHTMLEditor.h"
#include "nsIClipboard.h"
#include "nsIContent.h"
#include "nsIContentFilter.h"
#include "nsIDOMComment.h"
#include "mozilla/dom/DOMStringList.h"
#include "mozilla/dom/DataTransfer.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMNode.h"
#include "nsIDocument.h"
#include "nsIEditor.h"
#include "nsIEditorIMESupport.h"
#include "nsIEditorMailSupport.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIMIMEService.h"
#include "nsNameSpaceManager.h"
#include "nsINode.h"
#include "nsIParserUtils.h"
#include "nsIPlaintextEditor.h"
#include "nsISupportsImpl.h"
#include "nsISupportsPrimitives.h"
#include "nsISupportsUtils.h"
#include "nsITransferable.h"
#include "nsIURI.h"
#include "nsIVariant.h"
#include "nsLinebreakConverter.h"
#include "nsLiteralString.h"
#include "nsNetUtil.h"
#include "nsPlaintextEditor.h"
#include "nsRange.h"
#include "nsReadableUtils.h"
#include "nsSelectionState.h"
#include "nsServiceManagerUtils.h"
#include "nsStreamUtils.h"
#include "nsString.h"
#include "nsStringFwd.h"
#include "nsStringIterator.h"
#include "nsSubstringTuple.h"
#include "nsTextEditRules.h"
#include "nsTextEditUtils.h"
#include "nsTreeSanitizer.h"
#include "nsWSRunObject.h"
#include "nsXPCOM.h"
#include "nscore.h"
#include "nsContentUtils.h"

class nsIAtom;
class nsILoadContext;
class nsISupports;

using namespace mozilla;
using namespace mozilla::dom;

#define kInsertCookie  "_moz_Insert Here_moz_"


static bool FindIntegerAfterString(const char *aLeadingString, 
                                     nsCString &aCStr, int32_t &foundNumber);
static nsresult RemoveFragComments(nsCString &theStr);
static void RemoveBodyAndHead(nsIDOMNode *aNode);
static nsresult FindTargetNode(nsIDOMNode *aStart, nsCOMPtr<nsIDOMNode> &aResult);

static nsCOMPtr<nsIDOMNode> GetListParent(nsIDOMNode* aNode)
{
  NS_ENSURE_TRUE(aNode, nullptr);
  nsCOMPtr<nsIDOMNode> parent, tmp;
  aNode->GetParentNode(getter_AddRefs(parent));
  while (parent)
  {
    if (nsHTMLEditUtils::IsList(parent)) {
      return parent;
    }
    parent->GetParentNode(getter_AddRefs(tmp));
    parent = tmp;
  }
  return nullptr;
}

static nsCOMPtr<nsIDOMNode> GetTableParent(nsIDOMNode* aNode)
{
  NS_ENSURE_TRUE(aNode, nullptr);
  nsCOMPtr<nsIDOMNode> parent, tmp;
  aNode->GetParentNode(getter_AddRefs(parent));
  while (parent)
  {
    if (nsHTMLEditUtils::IsTable(parent)) {
      return parent;
    }
    parent->GetParentNode(getter_AddRefs(tmp));
    parent = tmp;
  }
  return nullptr;
}


nsresult
nsHTMLEditor::LoadHTML(const nsAString & aInputString)
{
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);

  
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::loadHTML, nsIEditor::eNext);

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_STATE(selection);

  nsTextRulesInfo ruleInfo(EditAction::loadHTML);
  bool cancel, handled;
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);
  nsresult rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel) {
    return NS_OK; 
  }

  if (!handled)
  {
    
    if (!selection->Collapsed()) {
      rv = DeleteSelection(eNone, eStrip);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    nsRefPtr<nsRange> range = selection->GetRangeAt(0);
    NS_ENSURE_TRUE(range, NS_ERROR_NULL_POINTER);

    
    nsCOMPtr<nsIDOMDocumentFragment> docfrag;
    {
      rv = range->CreateContextualFragment(aInputString, getter_AddRefs(docfrag));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    nsCOMPtr<nsIDOMNode> parent, junk;
    rv = range->GetStartContainer(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);
    int32_t childOffset;
    rv = range->GetStartOffset(&childOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> nodeToInsert;
    docfrag->GetFirstChild(getter_AddRefs(nodeToInsert));
    while (nodeToInsert)
    {
      rv = InsertNode(nodeToInsert, parent, childOffset++);
      NS_ENSURE_SUCCESS(rv, rv);
      docfrag->GetFirstChild(getter_AddRefs(nodeToInsert));
    }
  }

  return mRules->DidDoAction(selection, &ruleInfo, rv);
}


NS_IMETHODIMP nsHTMLEditor::InsertHTML(const nsAString & aInString)
{
  const nsAFlatString& empty = EmptyString();

  return InsertHTMLWithContext(aInString, empty, empty, empty,
                               nullptr,  nullptr, 0, true);
}


nsresult
nsHTMLEditor::InsertHTMLWithContext(const nsAString & aInputString,
                                    const nsAString & aContextStr,
                                    const nsAString & aInfoStr,
                                    const nsAString & aFlavor,
                                    nsIDOMDocument *aSourceDoc,
                                    nsIDOMNode *aDestNode,
                                    int32_t aDestOffset,
                                    bool aDeleteSelection)
{
  return DoInsertHTMLWithContext(aInputString, aContextStr, aInfoStr,
      aFlavor, aSourceDoc, aDestNode, aDestOffset, aDeleteSelection,
       true,  false);
}

nsresult
nsHTMLEditor::DoInsertHTMLWithContext(const nsAString & aInputString,
                                      const nsAString & aContextStr,
                                      const nsAString & aInfoStr,
                                      const nsAString & aFlavor,
                                      nsIDOMDocument *aSourceDoc,
                                      nsIDOMNode *aDestNode,
                                      int32_t aDestOffset,
                                      bool aDeleteSelection,
                                      bool aTrustedInput,
                                      bool aClearStyle)
{
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::htmlPaste, nsIEditor::eNext);

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_STATE(selection);

  
  nsCOMPtr<nsIDOMNode> fragmentAsNode, streamStartParent, streamEndParent;
  int32_t streamStartOffset = 0, streamEndOffset = 0;

  nsresult rv = CreateDOMFragmentFromPaste(aInputString, aContextStr, aInfoStr,
                                           address_of(fragmentAsNode),
                                           address_of(streamStartParent),
                                           address_of(streamEndParent),
                                           &streamStartOffset,
                                           &streamEndOffset,
                                           aTrustedInput);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> targetNode, tempNode;
  int32_t targetOffset=0;

  if (!aDestNode)
  {
    
    
    rv = GetStartNodeAndOffset(selection, getter_AddRefs(targetNode), &targetOffset);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!targetNode || !IsEditable(targetNode)) {
      return NS_ERROR_FAILURE;
    }
  }
  else
  {
    targetNode = aDestNode;
    targetOffset = aDestOffset;
  }

  bool doContinue = true;

  rv = DoContentFilterCallback(aFlavor, aSourceDoc, aDeleteSelection,
                               (nsIDOMNode **)address_of(fragmentAsNode),
                               (nsIDOMNode **)address_of(streamStartParent),
                               &streamStartOffset,
                               (nsIDOMNode **)address_of(streamEndParent),
                               &streamEndOffset,
                               (nsIDOMNode **)address_of(targetNode),
                               &targetOffset, &doContinue);

  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(doContinue, NS_OK);

  
  
  
  
  
  
  if (aDestNode)
  {
    if (aDeleteSelection)
    {
      
      
      nsAutoTrackDOMPoint tracker(mRangeUpdater, &targetNode, &targetOffset);
      rv = DeleteSelection(eNone, eStrip);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = selection->Collapse(targetNode, targetOffset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  

  
  nsCOMArray<nsIDOMNode> nodeList;
  rv = CreateListOfNodesToPaste(fragmentAsNode, nodeList,
                                streamStartParent, streamStartOffset,
                                streamEndParent, streamEndOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nodeList.Count() == 0) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDOMNode> parentNode;
  int32_t offsetOfNewNode;

  
  bool cellSelectionMode = false;
  nsCOMPtr<nsIDOMElement> cell;
  rv = GetFirstSelectedCell(nullptr, getter_AddRefs(cell));
  if (NS_SUCCEEDED(rv) && cell)
  {
    cellSelectionMode = true;
  }

  if (cellSelectionMode)
  {
    
    
    
    
    
    nsIDOMNode* firstNode = nodeList[0];
    if (!nsHTMLEditUtils::IsTableElement(firstNode))
      cellSelectionMode = false;
  }

  if (!cellSelectionMode)
  {
    rv = DeleteSelectionAndPrepareToCreateNode();
    NS_ENSURE_SUCCESS(rv, rv);

    if (aClearStyle) {
      
      nsCOMPtr<nsIDOMNode> tmpNode =
        do_QueryInterface(selection->GetAnchorNode());
      int32_t tmpOffset = static_cast<int32_t>(selection->AnchorOffset());
      rv = ClearStyle(address_of(tmpNode), &tmpOffset, nullptr, nullptr);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else
  {
    
    { 
      
      nsAutoSelectionReset selectionResetter(selection, this);
      rv = DeleteTableCell(1);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    selection->CollapseToStart();
  }

  
  nsTextRulesInfo ruleInfo(EditAction::insertElement);
  bool cancel, handled;
  rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel) {
    return NS_OK; 
  }

  if (!handled)
  {
    
    
    rv = GetStartNodeAndOffset(selection, getter_AddRefs(parentNode), &offsetOfNewNode);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(parentNode, NS_ERROR_FAILURE);

    
    NormalizeEOLInsertPosition(nodeList[0], address_of(parentNode), &offsetOfNewNode);

    
    
    
    nsWSRunObject wsObj(this, parentNode, offsetOfNewNode);
    if (wsObj.mEndReasonNode &&
        nsTextEditUtils::IsBreak(wsObj.mEndReasonNode) &&
        !IsVisBreak(wsObj.mEndReasonNode)) {
      rv = DeleteNode(wsObj.mEndReasonNode);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    bool bStartedInLink = IsInLink(parentNode);

    
    if (IsTextNode(parentNode))
    {
      nsCOMPtr<nsIDOMNode> temp;
      rv = SplitNodeDeep(parentNode, parentNode, offsetOfNewNode, &offsetOfNewNode);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = parentNode->GetParentNode(getter_AddRefs(temp));
      NS_ENSURE_SUCCESS(rv, rv);
      parentNode = temp;
    }

    
    
    nsCOMArray<nsIDOMNode> startListAndTableArray;
    rv = GetListAndTableParents(false, nodeList, startListAndTableArray);
    NS_ENSURE_SUCCESS(rv, rv);

    
    int32_t highWaterMark = -1;
    if (startListAndTableArray.Count() > 0)
    {
      rv = DiscoverPartialListsAndTables(nodeList, startListAndTableArray, &highWaterMark);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    if (highWaterMark >= 0)
    {
      rv = ReplaceOrphanedStructure(false, nodeList, startListAndTableArray, highWaterMark);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    nsCOMArray<nsIDOMNode> endListAndTableArray;
    rv = GetListAndTableParents(true, nodeList, endListAndTableArray);
    NS_ENSURE_SUCCESS(rv, rv);
    highWaterMark = -1;

    
    if (endListAndTableArray.Count() > 0)
    {
      rv = DiscoverPartialListsAndTables(nodeList, endListAndTableArray, &highWaterMark);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    if (highWaterMark >= 0)
    {
      rv = ReplaceOrphanedStructure(true, nodeList, endListAndTableArray, highWaterMark);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    nsCOMPtr<nsIDOMNode> parentBlock, lastInsertNode, insertedContextParent;
    int32_t listCount = nodeList.Count();
    int32_t j;
    if (IsBlockNode(parentNode))
      parentBlock = parentNode;
    else
      parentBlock = GetBlockNodeParent(parentNode);

    for (j=0; j<listCount; j++)
    {
      bool bDidInsert = false;
      nsCOMPtr<nsIDOMNode> curNode = nodeList[j];

      NS_ENSURE_TRUE(curNode, NS_ERROR_FAILURE);
      NS_ENSURE_TRUE(curNode != fragmentAsNode, NS_ERROR_FAILURE);
      NS_ENSURE_TRUE(!nsTextEditUtils::IsBody(curNode), NS_ERROR_FAILURE);

      if (insertedContextParent)
      {
        
        
        if (nsEditorUtils::IsDescendantOf(curNode, insertedContextParent))
          continue;
      }

      
      
      
      if (  (nsHTMLEditUtils::IsTableRow(curNode) && nsHTMLEditUtils::IsTableRow(parentNode))
         && (nsHTMLEditUtils::IsTable(curNode)    || nsHTMLEditUtils::IsTable(parentNode)) )
      {
        nsCOMPtr<nsIDOMNode> child;
        curNode->GetFirstChild(getter_AddRefs(child));
        while (child)
        {
          rv = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, true);
          if (NS_FAILED(rv))
            break;

          bDidInsert = true;
          lastInsertNode = child;
          offsetOfNewNode++;

          curNode->GetFirstChild(getter_AddRefs(child));
        }
      }
      
      
      
      
      else if (nsHTMLEditUtils::IsList(curNode) && 
              (nsHTMLEditUtils::IsList(parentNode)  || nsHTMLEditUtils::IsListItem(parentNode)) )
      {
        nsCOMPtr<nsIDOMNode> child, tmp;
        curNode->GetFirstChild(getter_AddRefs(child));
        while (child)
        {
          if (nsHTMLEditUtils::IsListItem(child) || nsHTMLEditUtils::IsList(child))
          {
            
            
            if (nsHTMLEditUtils::IsListItem(parentNode))
            {
              bool isEmpty;
              rv = IsEmptyNode(parentNode, &isEmpty, true);
              if (NS_SUCCEEDED(rv) && isEmpty)
              {
                int32_t newOffset;
                nsCOMPtr<nsIDOMNode> listNode = GetNodeLocation(parentNode, &newOffset);
                if (listNode)
                {
                  DeleteNode(parentNode);
                  parentNode = listNode;
                  offsetOfNewNode = newOffset;
                }
              }
            }
            rv = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, true);
            if (NS_FAILED(rv))
              break;

            bDidInsert = true;
            lastInsertNode = child;
            offsetOfNewNode++;
          }
          else
          {
            curNode->RemoveChild(child, getter_AddRefs(tmp));
          }
          curNode->GetFirstChild(getter_AddRefs(child));
        }

      } else if (parentBlock && nsHTMLEditUtils::IsPre(parentBlock) &&
                 nsHTMLEditUtils::IsPre(curNode)) {
        
        nsCOMPtr<nsIDOMNode> child, tmp;
        curNode->GetFirstChild(getter_AddRefs(child));
        while (child)
        {
          rv = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, true);
          if (NS_FAILED(rv))
            break;

          bDidInsert = true;
          lastInsertNode = child;
          offsetOfNewNode++;

          curNode->GetFirstChild(getter_AddRefs(child));
        }
      }

      if (!bDidInsert || NS_FAILED(rv))
      {
        
        rv = InsertNodeAtPoint(curNode, address_of(parentNode), &offsetOfNewNode, true);
        if (NS_SUCCEEDED(rv))
        {
          bDidInsert = true;
          lastInsertNode = curNode;
        }

        
        
        nsCOMPtr<nsIDOMNode> parent;
        while (NS_FAILED(rv) && curNode)
        {
          curNode->GetParentNode(getter_AddRefs(parent));
          if (parent && !nsTextEditUtils::IsBody(parent))
          {
            rv = InsertNodeAtPoint(parent, address_of(parentNode), &offsetOfNewNode, true);
            if (NS_SUCCEEDED(rv))
            {
              bDidInsert = true;
              insertedContextParent = parent;
              lastInsertNode = GetChildAt(parentNode, offsetOfNewNode);
            }
          }
          curNode = parent;
        }
      }
      if (lastInsertNode)
      {
        parentNode = GetNodeLocation(lastInsertNode, &offsetOfNewNode);
        offsetOfNewNode++;
      }
    }

    
    if (lastInsertNode) 
    {
      
      nsCOMPtr<nsIDOMNode> selNode, tmp, highTable;
      int32_t selOffset;

      
      if (!nsHTMLEditUtils::IsTable(lastInsertNode))
      {
        nsCOMPtr<nsINode> lastInsertNode_ = do_QueryInterface(lastInsertNode);
        NS_ENSURE_STATE(lastInsertNode_ || !lastInsertNode);
        selNode = GetAsDOMNode(GetLastEditableLeaf(*lastInsertNode_));
        tmp = selNode;
        while (tmp && (tmp != lastInsertNode))
        {
          if (nsHTMLEditUtils::IsTable(tmp))
            highTable = tmp;
          nsCOMPtr<nsIDOMNode> parent = tmp;
          tmp->GetParentNode(getter_AddRefs(parent));
          tmp = parent;
        }
        if (highTable)
          selNode = highTable;
      }
      if (!selNode)
        selNode = lastInsertNode;
      if (IsTextNode(selNode) || (IsContainer(selNode) && !nsHTMLEditUtils::IsTable(selNode)))
      {
        rv = GetLengthOfDOMNode(selNode, (uint32_t&)selOffset);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else 
      {
        tmp = selNode;
        selNode = GetNodeLocation(tmp, &selOffset);
        
        
        NS_ENSURE_STATE(selNode);
        ++selOffset;  
      }

      
      nsWSRunObject wsRunObj(this, selNode, selOffset);
      nsCOMPtr<nsINode> visNode;
      int32_t outVisOffset=0;
      WSType visType;
      nsCOMPtr<nsINode> selNode_(do_QueryInterface(selNode));
      wsRunObj.PriorVisibleNode(selNode_, selOffset, address_of(visNode),
                                &outVisOffset, &visType);
      if (visType == WSType::br) {
        
        
        
        
        if (!IsVisBreak(wsRunObj.mStartReasonNode))
        {
          
          
          selNode = GetNodeLocation(GetAsDOMNode(wsRunObj.mStartReasonNode), &selOffset);
          
          nsWSRunObject wsRunObj(this, selNode, selOffset);
          selNode_ = do_QueryInterface(selNode);
          wsRunObj.PriorVisibleNode(selNode_, selOffset, address_of(visNode),
                                    &outVisOffset, &visType);
          if (visType == WSType::text || visType == WSType::normalWS) {
            selNode = GetAsDOMNode(visNode);
            selOffset = outVisOffset;  
          } else if (visType == WSType::special) {
            
            
            selNode = GetNodeLocation(GetAsDOMNode(wsRunObj.mStartReasonNode), &selOffset);
            ++selOffset;
          }
        }
      }
      selection->Collapse(selNode, selOffset);

      
      nsCOMPtr<nsIDOMNode> link;
      if (!bStartedInLink && IsInLink(selNode, address_of(link)))
      {
        
        
        
        
        nsCOMPtr<nsIDOMNode> leftLink;
        int32_t linkOffset;
        rv = SplitNodeDeep(link, selNode, selOffset, &linkOffset, true, address_of(leftLink));
        NS_ENSURE_SUCCESS(rv, rv);
        if (leftLink) {
          selNode = GetNodeLocation(leftLink, &selOffset);
          selection->Collapse(selNode, selOffset+1);
        }
      }
    }
  }

  return mRules->DidDoAction(selection, &ruleInfo, rv);
}

nsresult
nsHTMLEditor::AddInsertionListener(nsIContentFilter *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  
  if (mContentFilters.IndexOfObject(aListener) == -1)
  {
    NS_ENSURE_TRUE(mContentFilters.AppendObject(aListener), NS_ERROR_FAILURE);
  }

  return NS_OK;
}

nsresult
nsHTMLEditor::RemoveInsertionListener(nsIContentFilter *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_FAILURE);

  NS_ENSURE_TRUE(mContentFilters.RemoveObject(aListener), NS_ERROR_FAILURE);

  return NS_OK;
}
 
nsresult
nsHTMLEditor::DoContentFilterCallback(const nsAString &aFlavor, 
                                      nsIDOMDocument *sourceDoc,
                                      bool aWillDeleteSelection,
                                      nsIDOMNode **aFragmentAsNode, 
                                      nsIDOMNode **aFragStartNode, 
                                      int32_t *aFragStartOffset,
                                      nsIDOMNode **aFragEndNode, 
                                      int32_t *aFragEndOffset,
                                      nsIDOMNode **aTargetNode,
                                      int32_t *aTargetOffset,
                                      bool *aDoContinue)
{
  *aDoContinue = true;

  int32_t i;
  nsIContentFilter *listener;
  for (i=0; i < mContentFilters.Count() && *aDoContinue; i++)
  {
    listener = (nsIContentFilter *)mContentFilters[i];
    if (listener)
      listener->NotifyOfInsertion(aFlavor, nullptr, sourceDoc,
                                  aWillDeleteSelection, aFragmentAsNode,
                                  aFragStartNode, aFragStartOffset, 
                                  aFragEndNode, aFragEndOffset,
                                  aTargetNode, aTargetOffset, aDoContinue);
  }

  return NS_OK;
}

bool
nsHTMLEditor::IsInLink(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *outLink)
{
  NS_ENSURE_TRUE(aNode, false);
  if (outLink)
    *outLink = nullptr;
  nsCOMPtr<nsIDOMNode> tmp, node = aNode;
  while (node)
  {
    if (nsHTMLEditUtils::IsLink(node)) 
    {
      if (outLink)
        *outLink = node;
      return true;
    }
    tmp = node;
    tmp->GetParentNode(getter_AddRefs(node));
  }
  return false;
}


nsresult
nsHTMLEditor::StripFormattingNodes(nsIDOMNode *aNode, bool aListOnly)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (content->TextIsOnlyWhitespace())
  {
    nsCOMPtr<nsIDOMNode> parent, ignored;
    aNode->GetParentNode(getter_AddRefs(parent));
    if (parent)
    {
      if (!aListOnly || nsHTMLEditUtils::IsList(parent)) {
        return parent->RemoveChild(aNode, getter_AddRefs(ignored));
      }
      return NS_OK;
    }
  }

  if (!nsHTMLEditUtils::IsPre(aNode))
  {
    nsCOMPtr<nsIDOMNode> child;
    aNode->GetLastChild(getter_AddRefs(child));

    while (child)
    {
      nsCOMPtr<nsIDOMNode> tmp;
      child->GetPreviousSibling(getter_AddRefs(tmp));
      nsresult rv = StripFormattingNodes(child, aListOnly);
      NS_ENSURE_SUCCESS(rv, rv);
      child = tmp;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::PrepareTransferable(nsITransferable **transferable)
{
  return NS_OK;
}

nsresult
nsHTMLEditor::PrepareHTMLTransferable(nsITransferable **aTransferable,
                                      bool aHavePrivFlavor)
{
  
  nsresult rv = CallCreateInstance("@mozilla.org/widget/transferable;1", aTransferable);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aTransferable)
  {
    nsCOMPtr<nsIDocument> destdoc = GetDocument();
    nsILoadContext* loadContext = destdoc ? destdoc->GetLoadContext() : nullptr;
    (*aTransferable)->Init(loadContext);

    
    
    
    if (!IsPlaintextEditor())
    {
      if (!aHavePrivFlavor) 
      {
        (*aTransferable)->AddDataFlavor(kNativeHTMLMime);
      }
      (*aTransferable)->AddDataFlavor(kHTMLMime);
      (*aTransferable)->AddDataFlavor(kFileMime);

      switch (Preferences::GetInt("clipboard.paste_image_type", 1))
      {
        case 0:  
          (*aTransferable)->AddDataFlavor(kJPEGImageMime);
          (*aTransferable)->AddDataFlavor(kJPGImageMime);
          (*aTransferable)->AddDataFlavor(kPNGImageMime);
          (*aTransferable)->AddDataFlavor(kGIFImageMime);
          break;
        case 1:  
        default:
          (*aTransferable)->AddDataFlavor(kPNGImageMime);
          (*aTransferable)->AddDataFlavor(kJPEGImageMime);
          (*aTransferable)->AddDataFlavor(kJPGImageMime);
          (*aTransferable)->AddDataFlavor(kGIFImageMime);
          break;
        case 2:  
          (*aTransferable)->AddDataFlavor(kGIFImageMime);
          (*aTransferable)->AddDataFlavor(kJPEGImageMime);
          (*aTransferable)->AddDataFlavor(kJPGImageMime);
          (*aTransferable)->AddDataFlavor(kPNGImageMime);
          break;
      }
    }
    (*aTransferable)->AddDataFlavor(kUnicodeMime);
    (*aTransferable)->AddDataFlavor(kMozTextInternal);
  }
  
  return NS_OK;
}

bool
FindIntegerAfterString(const char *aLeadingString, 
                       nsCString &aCStr, int32_t &foundNumber)
{
  
  int32_t numFront = aCStr.Find(aLeadingString);
  if (numFront == -1)
    return false;
  numFront += strlen(aLeadingString);

  int32_t numBack = aCStr.FindCharInSet(CRLF, numFront);
  if (numBack == -1)
    return false;

  nsAutoCString numStr(Substring(aCStr, numFront, numBack-numFront));
  nsresult errorCode;
  foundNumber = numStr.ToInteger(&errorCode);
  return true;
}

nsresult
RemoveFragComments(nsCString & aStr)
{
  
  int32_t startCommentIndx = aStr.Find("<!--StartFragment");
  if (startCommentIndx >= 0)
  {
    int32_t startCommentEnd = aStr.Find("-->", false, startCommentIndx);
    if (startCommentEnd > startCommentIndx)
      aStr.Cut(startCommentIndx, (startCommentEnd+3)-startCommentIndx);
  }
  int32_t endCommentIndx = aStr.Find("<!--EndFragment");
  if (endCommentIndx >= 0)
  {
    int32_t endCommentEnd = aStr.Find("-->", false, endCommentIndx);
    if (endCommentEnd > endCommentIndx)
      aStr.Cut(endCommentIndx, (endCommentEnd+3)-endCommentIndx);
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::ParseCFHTML(nsCString & aCfhtml, char16_t **aStuffToPaste, char16_t **aCfcontext)
{
  
  int32_t startHTML, endHTML, startFragment, endFragment;
  if (!FindIntegerAfterString("StartHTML:", aCfhtml, startHTML) || 
      startHTML < -1)
    return NS_ERROR_FAILURE;
  if (!FindIntegerAfterString("EndHTML:", aCfhtml, endHTML) || 
      endHTML < -1)
    return NS_ERROR_FAILURE;
  if (!FindIntegerAfterString("StartFragment:", aCfhtml, startFragment) || 
      startFragment < 0)
    return NS_ERROR_FAILURE;
  if (!FindIntegerAfterString("EndFragment:", aCfhtml, endFragment) || 
      startFragment < 0)
    return NS_ERROR_FAILURE;

  
  
  
  if (startHTML == -1) {
    startHTML = aCfhtml.Find("<!--StartFragment-->");
    if (startHTML == -1)
      return NS_OK;
  }
  if (endHTML == -1) {
    const char endFragmentMarker[] = "<!--EndFragment-->";
    endHTML = aCfhtml.Find(endFragmentMarker);
    if (endHTML == -1)
      return NS_OK;
    endHTML += ArrayLength(endFragmentMarker) - 1;
  }

  
  nsAutoCString contextUTF8(Substring(aCfhtml, startHTML, startFragment - startHTML) +
                            NS_LITERAL_CSTRING("<!--" kInsertCookie "-->") +
                            Substring(aCfhtml, endFragment, endHTML - endFragment));

  
  
  
  int32_t curPos = startFragment;
  while (curPos > startHTML)
  {
      if (aCfhtml[curPos] == '>')
      {
          
          
          break;
      }
      else if (aCfhtml[curPos] == '<') 
      {
          
          if (curPos != startFragment) 
          {
              
              
              NS_ERROR("StartFragment byte count in the clipboard looks bad, see bug #228879");
              startFragment = curPos - 1;
          }
          break;
      }
      else 
      {
          curPos--;
      }
  }

  
  nsAutoCString fragmentUTF8(Substring(aCfhtml, startFragment, endFragment-startFragment));

  
  RemoveFragComments(fragmentUTF8);

  
  RemoveFragComments(contextUTF8);

  
  const nsAFlatString& fragUcs2Str = NS_ConvertUTF8toUTF16(fragmentUTF8);
  const nsAFlatString& cntxtUcs2Str = NS_ConvertUTF8toUTF16(contextUTF8);

  
  int32_t oldLengthInChars = fragUcs2Str.Length() + 1;  
  int32_t newLengthInChars = 0;
  *aStuffToPaste = nsLinebreakConverter::ConvertUnicharLineBreaks(fragUcs2Str.get(),
                                                           nsLinebreakConverter::eLinebreakAny, 
                                                           nsLinebreakConverter::eLinebreakContent, 
                                                           oldLengthInChars, &newLengthInChars);
  NS_ENSURE_TRUE(*aStuffToPaste, NS_ERROR_FAILURE);

  
  oldLengthInChars = cntxtUcs2Str.Length() + 1;  
  newLengthInChars = 0;
  *aCfcontext = nsLinebreakConverter::ConvertUnicharLineBreaks(cntxtUcs2Str.get(),
                                                           nsLinebreakConverter::eLinebreakAny, 
                                                           nsLinebreakConverter::eLinebreakContent, 
                                                           oldLengthInChars, &newLengthInChars);
  

  
  return NS_OK;
}

nsresult nsHTMLEditor::InsertObject(const char* aType, nsISupports* aObject, bool aIsSafe,
                                    nsIDOMDocument *aSourceDoc,
                                    nsIDOMNode *aDestinationNode,
                                    int32_t aDestOffset,
                                    bool aDoDeleteSelection)
{
  nsresult rv;

  const char* type = aType;

  
  bool insertAsImage = false;
  nsCOMPtr<nsIURI> fileURI;
  if (0 == nsCRT::strcmp(type, kFileMime))
  {
    nsCOMPtr<nsIFile> fileObj = do_QueryInterface(aObject);
    if (fileObj)
    {
      rv = NS_NewFileURI(getter_AddRefs(fileURI), fileObj);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIMIMEService> mime = do_GetService("@mozilla.org/mime;1");
      NS_ENSURE_TRUE(mime, NS_ERROR_FAILURE);
      nsAutoCString contentType;
      rv = mime->GetTypeFromFile(fileObj, contentType);
      NS_ENSURE_SUCCESS(rv, rv);

      
      if (StringBeginsWith(contentType, NS_LITERAL_CSTRING("image/"))) {
        insertAsImage = true;
        type = contentType.get();
      }
    }
  }

  if (0 == nsCRT::strcmp(type, kJPEGImageMime) ||
      0 == nsCRT::strcmp(type, kJPGImageMime) ||
      0 == nsCRT::strcmp(type, kPNGImageMime) ||
      0 == nsCRT::strcmp(type, kGIFImageMime) ||
      insertAsImage)
  {
    nsCOMPtr<nsIInputStream> imageStream;
    if (insertAsImage) {
      NS_ASSERTION(fileURI, "The file URI should be retrieved earlier");

      nsCOMPtr<nsIChannel> channel;
      rv = NS_NewChannel(getter_AddRefs(channel),
                         fileURI,
                         nsContentUtils::GetSystemPrincipal(),
                         nsILoadInfo::SEC_NORMAL,
                         nsIContentPolicy::TYPE_OTHER);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = channel->Open(getter_AddRefs(imageStream));
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      imageStream = do_QueryInterface(aObject);
      NS_ENSURE_TRUE(imageStream, NS_ERROR_FAILURE);
    }

    nsCString imageData;
    rv = NS_ConsumeStream(imageStream, UINT32_MAX, imageData);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = imageStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString data64;
    rv = Base64Encode(imageData, data64);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString stuffToPaste;
    stuffToPaste.AssignLiteral("<IMG src=\"data:");
    AppendUTF8toUTF16(type, stuffToPaste);
    stuffToPaste.AppendLiteral(";base64,");
    AppendUTF8toUTF16(data64, stuffToPaste);
    stuffToPaste.AppendLiteral("\" alt=\"\" >");
    nsAutoEditBatch beginBatching(this);
    rv = DoInsertHTMLWithContext(stuffToPaste, EmptyString(), EmptyString(), 
                                 NS_LITERAL_STRING(kFileMime),
                                 aSourceDoc,
                                 aDestinationNode, aDestOffset,
                                 aDoDeleteSelection,
                                 aIsSafe);
  }

  return NS_OK;
}

nsresult
nsHTMLEditor::InsertFromTransferable(nsITransferable *transferable,
                                     nsIDOMDocument *aSourceDoc,
                                     const nsAString & aContextStr,
                                     const nsAString & aInfoStr,
                                     nsIDOMNode *aDestinationNode,
                                     int32_t aDestOffset,
                                     bool aDoDeleteSelection)
{
  nsresult rv = NS_OK;
  nsXPIDLCString bestFlavor;
  nsCOMPtr<nsISupports> genericDataObj;
  uint32_t len = 0;
  if (NS_SUCCEEDED(transferable->GetAnyTransferData(getter_Copies(bestFlavor), getter_AddRefs(genericDataObj), &len)))
  {
    nsAutoTxnsConserveSelection dontSpazMySelection(this);
    nsAutoString flavor;
    flavor.AssignWithConversion(bestFlavor);
    nsAutoString stuffToPaste;
    bool isSafe = IsSafeToInsertData(aSourceDoc);

    if (0 == nsCRT::strcmp(bestFlavor, kFileMime) ||
        0 == nsCRT::strcmp(bestFlavor, kJPEGImageMime) ||
        0 == nsCRT::strcmp(bestFlavor, kJPGImageMime) ||
        0 == nsCRT::strcmp(bestFlavor, kPNGImageMime) ||
        0 == nsCRT::strcmp(bestFlavor, kGIFImageMime)) {
      rv = InsertObject(bestFlavor, genericDataObj, isSafe,
                        aSourceDoc, aDestinationNode, aDestOffset, aDoDeleteSelection);
    }
    else if (0 == nsCRT::strcmp(bestFlavor, kNativeHTMLMime))
    {
      
      nsCOMPtr<nsISupportsCString> textDataObj = do_QueryInterface(genericDataObj);
      if (textDataObj && len > 0)
      {
        nsAutoCString cfhtml;
        textDataObj->GetData(cfhtml);
        NS_ASSERTION(cfhtml.Length() <= (len), "Invalid length!");
        nsXPIDLString cfcontext, cffragment, cfselection; 

        rv = ParseCFHTML(cfhtml, getter_Copies(cffragment), getter_Copies(cfcontext));
        if (NS_SUCCEEDED(rv) && !cffragment.IsEmpty())
        {
          nsAutoEditBatch beginBatching(this);
          rv = DoInsertHTMLWithContext(cffragment,
                                       cfcontext, cfselection, flavor,
                                       aSourceDoc,
                                       aDestinationNode, aDestOffset,
                                       aDoDeleteSelection,
                                       isSafe);
        } else {
          
          
          
          
          
          bestFlavor.AssignLiteral(kHTMLMime);
          
        }
      }
    }
    if (0 == nsCRT::strcmp(bestFlavor, kHTMLMime) ||
        0 == nsCRT::strcmp(bestFlavor, kUnicodeMime) ||
        0 == nsCRT::strcmp(bestFlavor, kMozTextInternal)) {
      nsCOMPtr<nsISupportsString> textDataObj = do_QueryInterface(genericDataObj);
      if (textDataObj && len > 0) {
        nsAutoString text;
        textDataObj->GetData(text);
        NS_ASSERTION(text.Length() <= (len/2), "Invalid length!");
        stuffToPaste.Assign(text.get(), len / 2);
      } else {
        nsCOMPtr<nsISupportsCString> textDataObj(do_QueryInterface(genericDataObj));
        if (textDataObj && len > 0) {
          nsAutoCString text;
          textDataObj->GetData(text);
          NS_ASSERTION(text.Length() <= len, "Invalid length!");
          stuffToPaste.Assign(NS_ConvertUTF8toUTF16(Substring(text, 0, len)));
        }
      }

      if (!stuffToPaste.IsEmpty()) {
        nsAutoEditBatch beginBatching(this);
        if (0 == nsCRT::strcmp(bestFlavor, kHTMLMime)) {
          rv = DoInsertHTMLWithContext(stuffToPaste,
                                       aContextStr, aInfoStr, flavor,
                                       aSourceDoc,
                                       aDestinationNode, aDestOffset,
                                       aDoDeleteSelection,
                                       isSafe);
        } else {
          rv = InsertTextAt(stuffToPaste, aDestinationNode, aDestOffset, aDoDeleteSelection);
        }
      }
    }
  }

  
  if (NS_SUCCEEDED(rv))
    ScrollSelectionIntoView(false);

  return rv;
}

static void
GetStringFromDataTransfer(nsIDOMDataTransfer *aDataTransfer, const nsAString& aType,
                          int32_t aIndex, nsAString& aOutputString)
{
  nsCOMPtr<nsIVariant> variant;
  aDataTransfer->MozGetDataAt(aType, aIndex, getter_AddRefs(variant));
  if (variant)
    variant->GetAsAString(aOutputString);
}

nsresult nsHTMLEditor::InsertFromDataTransfer(DataTransfer *aDataTransfer,
                                              int32_t aIndex,
                                              nsIDOMDocument *aSourceDoc,
                                              nsIDOMNode *aDestinationNode,
                                              int32_t aDestOffset,
                                              bool aDoDeleteSelection)
{
  ErrorResult rv;
  nsRefPtr<DOMStringList> types = aDataTransfer->MozTypesAt(aIndex, rv);
  if (rv.Failed()) {
    return rv.ErrorCode();
  }

  bool hasPrivateHTMLFlavor = types->Contains(NS_LITERAL_STRING(kHTMLContext));

  bool isText = IsPlaintextEditor();
  bool isSafe = IsSafeToInsertData(aSourceDoc);

  uint32_t length = types->Length();
  for (uint32_t t = 0; t < length; t++) {
    nsAutoString type;
    types->Item(t, type);

    if (!isText) {
      if (type.EqualsLiteral(kFileMime) ||
          type.EqualsLiteral(kJPEGImageMime) ||
          type.EqualsLiteral(kJPGImageMime) ||
          type.EqualsLiteral(kPNGImageMime) ||
          type.EqualsLiteral(kGIFImageMime)) {
        nsCOMPtr<nsIVariant> variant;
        aDataTransfer->MozGetDataAt(type, aIndex, getter_AddRefs(variant));
        if (variant) {
          nsCOMPtr<nsISupports> object;
          variant->GetAsISupports(getter_AddRefs(object));
          return InsertObject(NS_ConvertUTF16toUTF8(type).get(), object, isSafe,
                              aSourceDoc, aDestinationNode, aDestOffset, aDoDeleteSelection);
        }
      }
      else if (!hasPrivateHTMLFlavor && type.EqualsLiteral(kNativeHTMLMime)) {
        nsAutoString text;
        GetStringFromDataTransfer(aDataTransfer, NS_LITERAL_STRING(kNativeHTMLMime), aIndex, text);
        NS_ConvertUTF16toUTF8 cfhtml(text);

        nsXPIDLString cfcontext, cffragment, cfselection; 

        nsresult rv = ParseCFHTML(cfhtml, getter_Copies(cffragment), getter_Copies(cfcontext));
        if (NS_SUCCEEDED(rv) && !cffragment.IsEmpty())
        {
          nsAutoEditBatch beginBatching(this);
          return DoInsertHTMLWithContext(cffragment,
                                         cfcontext, cfselection, type,
                                         aSourceDoc,
                                         aDestinationNode, aDestOffset,
                                         aDoDeleteSelection,
                                         isSafe);
        }
      }
      else if (type.EqualsLiteral(kHTMLMime)) {
        nsAutoString text, contextString, infoString;
        GetStringFromDataTransfer(aDataTransfer, type, aIndex, text);
        GetStringFromDataTransfer(aDataTransfer, NS_LITERAL_STRING(kHTMLContext), aIndex, contextString);
        GetStringFromDataTransfer(aDataTransfer, NS_LITERAL_STRING(kHTMLInfo), aIndex, infoString);

        nsAutoEditBatch beginBatching(this);
        if (type.EqualsLiteral(kHTMLMime)) {
          return DoInsertHTMLWithContext(text,
                                         contextString, infoString, type,
                                         aSourceDoc,
                                         aDestinationNode, aDestOffset,
                                         aDoDeleteSelection,
                                         isSafe);
        }
      }
    }

    if (type.EqualsLiteral(kTextMime) ||
        type.EqualsLiteral(kMozTextInternal)) {
      nsAutoString text;
      GetStringFromDataTransfer(aDataTransfer, type, aIndex, text);

      nsAutoEditBatch beginBatching(this);
      return InsertTextAt(text, aDestinationNode, aDestOffset, aDoDeleteSelection);
    }
  }

  return NS_OK;
}

bool nsHTMLEditor::HavePrivateHTMLFlavor(nsIClipboard *aClipboard)
{
  
  

  NS_ENSURE_TRUE(aClipboard, false);
  bool bHavePrivateHTMLFlavor = false;

  const char* flavArray[] = { kHTMLContext };

  if (NS_SUCCEEDED(aClipboard->HasDataMatchingFlavors(flavArray,
    ArrayLength(flavArray), nsIClipboard::kGlobalClipboard,
    &bHavePrivateHTMLFlavor)))
    return bHavePrivateHTMLFlavor;

  return false;
}


NS_IMETHODIMP nsHTMLEditor::Paste(int32_t aSelectionType)
{
  if (!FireClipboardEvent(NS_PASTE, aSelectionType))
    return NS_OK;

  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  bool bHavePrivateHTMLFlavor = HavePrivateHTMLFlavor(clipboard);

  
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareHTMLTransferable(getter_AddRefs(trans), bHavePrivateHTMLFlavor);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(trans, NS_ERROR_FAILURE);
  
  rv = clipboard->GetData(trans, aSelectionType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!IsModifiable()) {
    return NS_OK;
  }

  
  nsAutoString contextStr, infoStr;

  
  if (bHavePrivateHTMLFlavor)
  {
    nsCOMPtr<nsISupports> contextDataObj, infoDataObj;
    uint32_t contextLen, infoLen;
    nsCOMPtr<nsISupportsString> textDataObj;

    nsCOMPtr<nsITransferable> contextTrans =
                  do_CreateInstance("@mozilla.org/widget/transferable;1");
    NS_ENSURE_TRUE(contextTrans, NS_ERROR_NULL_POINTER);
    contextTrans->Init(nullptr);
    contextTrans->AddDataFlavor(kHTMLContext);
    clipboard->GetData(contextTrans, aSelectionType);
    contextTrans->GetTransferData(kHTMLContext, getter_AddRefs(contextDataObj), &contextLen);

    nsCOMPtr<nsITransferable> infoTrans =
                  do_CreateInstance("@mozilla.org/widget/transferable;1");
    NS_ENSURE_TRUE(infoTrans, NS_ERROR_NULL_POINTER);
    infoTrans->Init(nullptr);
    infoTrans->AddDataFlavor(kHTMLInfo);
    clipboard->GetData(infoTrans, aSelectionType);
    infoTrans->GetTransferData(kHTMLInfo, getter_AddRefs(infoDataObj), &infoLen);

    if (contextDataObj)
    {
      nsAutoString text;
      textDataObj = do_QueryInterface(contextDataObj);
      textDataObj->GetData(text);
      NS_ASSERTION(text.Length() <= (contextLen/2), "Invalid length!");
      contextStr.Assign(text.get(), contextLen / 2);
    }

    if (infoDataObj)
    {
      nsAutoString text;
      textDataObj = do_QueryInterface(infoDataObj);
      textDataObj->GetData(text);
      NS_ASSERTION(text.Length() <= (infoLen/2), "Invalid length!");
      infoStr.Assign(text.get(), infoLen / 2);
    }
  }

  
  nsCOMPtr<nsIDOMDocument> domdoc;
  GetDocument(getter_AddRefs(domdoc));
  if (!nsEditorHookUtils::DoInsertionHook(domdoc, nullptr, trans))
    return NS_OK;

  return InsertFromTransferable(trans, nullptr, contextStr, infoStr,
                                nullptr, 0, true);
}

NS_IMETHODIMP nsHTMLEditor::PasteTransferable(nsITransferable *aTransferable)
{
  
  
  if (!FireClipboardEvent(NS_PASTE, nsIClipboard::kGlobalClipboard))
    return NS_OK;

  
  nsCOMPtr<nsIDOMDocument> domdoc = GetDOMDocument();
  if (!nsEditorHookUtils::DoInsertionHook(domdoc, nullptr, aTransferable))
    return NS_OK;

  nsAutoString contextStr, infoStr;
  return InsertFromTransferable(aTransferable, nullptr, contextStr, infoStr,
                                nullptr, 0, true);
}




NS_IMETHODIMP nsHTMLEditor::PasteNoFormatting(int32_t aSelectionType)
{
  if (!FireClipboardEvent(NS_PASTE, aSelectionType))
    return NS_OK;

  ForceCompositionEnd();

  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsITransferable> trans;
  rv = nsPlaintextEditor::PrepareTransferable(getter_AddRefs(trans));
  if (NS_SUCCEEDED(rv) && trans)
  {
    
    if (NS_SUCCEEDED(clipboard->GetData(trans, aSelectionType)) && IsModifiable())
    {
      const nsAFlatString& empty = EmptyString();
      rv = InsertFromTransferable(trans, nullptr, empty, empty, nullptr, 0,
                                  true);
    }
  }

  return rv;
}





static const char* textEditorFlavors[] = { kUnicodeMime };
static const char* textHtmlEditorFlavors[] = { kUnicodeMime, kHTMLMime,
                                               kJPEGImageMime, kJPGImageMime,
                                               kPNGImageMime, kGIFImageMime };

NS_IMETHODIMP nsHTMLEditor::CanPaste(int32_t aSelectionType, bool *aCanPaste)
{
  NS_ENSURE_ARG_POINTER(aCanPaste);
  *aCanPaste = false;

  
  if (!IsModifiable()) {
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  bool haveFlavors;

  
  if (IsPlaintextEditor())
    rv = clipboard->HasDataMatchingFlavors(textEditorFlavors,
                                           ArrayLength(textEditorFlavors),
                                           aSelectionType, &haveFlavors);
  else
    rv = clipboard->HasDataMatchingFlavors(textHtmlEditorFlavors,
                                           ArrayLength(textHtmlEditorFlavors),
                                           aSelectionType, &haveFlavors);

  NS_ENSURE_SUCCESS(rv, rv);

  *aCanPaste = haveFlavors;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::CanPasteTransferable(nsITransferable *aTransferable, bool *aCanPaste)
{
  NS_ENSURE_ARG_POINTER(aCanPaste);

  
  if (!IsModifiable()) {
    *aCanPaste = false;
    return NS_OK;
  }

  
  if (!aTransferable) {
    *aCanPaste = true;
    return NS_OK;
  }

  

  
  const char ** flavors;
  unsigned length;
  if (IsPlaintextEditor()) {
    flavors = textEditorFlavors;
    length = ArrayLength(textEditorFlavors);
  } else {
    flavors = textHtmlEditorFlavors;
    length = ArrayLength(textHtmlEditorFlavors);
  }

  for (unsigned int i = 0; i < length; i++, flavors++) {
    nsCOMPtr<nsISupports> data;
    uint32_t dataLen;
    nsresult rv = aTransferable->GetTransferData(*flavors,
                                                 getter_AddRefs(data),
                                                 &dataLen);
    if (NS_SUCCEEDED(rv) && data) {
      *aCanPaste = true;
      return NS_OK;
    }
  }

  *aCanPaste = false;
  return NS_OK;
}





NS_IMETHODIMP nsHTMLEditor::PasteAsQuotation(int32_t aSelectionType)
{
  if (IsPlaintextEditor())
    return PasteAsPlaintextQuotation(aSelectionType);

  nsAutoString citation;
  return PasteAsCitedQuotation(citation, aSelectionType);
}

NS_IMETHODIMP nsHTMLEditor::PasteAsCitedQuotation(const nsAString & aCitation,
                                                  int32_t aSelectionType)
{
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::insertQuotation, nsIEditor::eNext);

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  nsTextRulesInfo ruleInfo(EditAction::insertElement);
  bool cancel, handled;
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);
  nsresult rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel || handled) {
    return NS_OK; 
  }

  nsCOMPtr<Element> newNode =
    DeleteSelectionAndCreateElement(*nsGkAtoms::blockquote);
  NS_ENSURE_TRUE(newNode, NS_ERROR_NULL_POINTER);

  
  newNode->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                   NS_LITERAL_STRING("cite"), true);

  
  rv = selection->Collapse(newNode, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  return Paste(aSelectionType);
}




NS_IMETHODIMP nsHTMLEditor::PasteAsPlaintextQuotation(int32_t aSelectionType)
{
  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransferable> trans =
                 do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(trans, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocument> destdoc = GetDocument();
  nsILoadContext* loadContext = destdoc ? destdoc->GetLoadContext() : nullptr;
  trans->Init(loadContext);

  
  trans->AddDataFlavor(kUnicodeMime);

  
  clipboard->GetData(trans, aSelectionType);

  
  
  
  nsCOMPtr<nsISupports> genericDataObj;
  uint32_t len = 0;
  char* flav = 0;
  rv = trans->GetAnyTransferData(&flav, getter_AddRefs(genericDataObj), &len);
  NS_ENSURE_SUCCESS(rv, rv);

  if (flav && 0 == nsCRT::strcmp(flav, kUnicodeMime))
  {
    nsCOMPtr<nsISupportsString> textDataObj = do_QueryInterface(genericDataObj);
    if (textDataObj && len > 0)
    {
      nsAutoString stuffToPaste;
      textDataObj->GetData(stuffToPaste);
      NS_ASSERTION(stuffToPaste.Length() <= (len/2), "Invalid length!");
      nsAutoEditBatch beginBatching(this);
      rv = InsertAsPlaintextQuotation(stuffToPaste, true, 0);
    }
  }
  NS_Free(flav);

  return rv;
}

NS_IMETHODIMP
nsHTMLEditor::InsertTextWithQuotations(const nsAString &aStringToInsert)
{
  if (mWrapToWindow)
    return InsertText(aStringToInsert);

  
  BeginTransaction();

  
  
  
  

  static const char16_t cite('>');
  bool curHunkIsQuoted = (aStringToInsert.First() == cite);

  nsAString::const_iterator hunkStart, strEnd;
  aStringToInsert.BeginReading(hunkStart);
  aStringToInsert.EndReading(strEnd);

  
  
  
  
  
#ifdef DEBUG
  nsAString::const_iterator dbgStart (hunkStart);
  if (FindCharInReadable('\r', dbgStart, strEnd))
    NS_ASSERTION(false,
            "Return characters in DOM! InsertTextWithQuotations may be wrong");
#endif 

  
  nsresult rv = NS_OK;
  nsAString::const_iterator lineStart (hunkStart);
  while (1)   
  {
    
    bool found = FindCharInReadable('\n', lineStart, strEnd);
    bool quoted = false;
    if (found)
    {
      
      
      nsAString::const_iterator firstNewline (lineStart);
      while (*lineStart == '\n')
        ++lineStart;
      quoted = (*lineStart == cite);
      if (quoted == curHunkIsQuoted)
        continue;
      
      

      
      
      
      
      
      
      if (curHunkIsQuoted)
        lineStart = firstNewline;
    }

    
    
    const nsAString &curHunk = Substring(hunkStart, lineStart);
    nsCOMPtr<nsIDOMNode> dummyNode;
    if (curHunkIsQuoted)
      rv = InsertAsPlaintextQuotation(curHunk, false,
                                      getter_AddRefs(dummyNode));
    else
      rv = InsertText(curHunk);

    if (!found)
      break;

    curHunkIsQuoted = quoted;
    hunkStart = lineStart;
  }

  EndTransaction();

  return rv;
}

NS_IMETHODIMP nsHTMLEditor::InsertAsQuotation(const nsAString & aQuotedText,
                                              nsIDOMNode **aNodeInserted)
{
  if (IsPlaintextEditor())
    return InsertAsPlaintextQuotation(aQuotedText, true, aNodeInserted);

  nsAutoString citation;
  return InsertAsCitedQuotation(aQuotedText, citation, false,
                                aNodeInserted);
}





NS_IMETHODIMP
nsHTMLEditor::InsertAsPlaintextQuotation(const nsAString & aQuotedText,
                                         bool aAddCites,
                                         nsIDOMNode **aNodeInserted)
{
  if (mWrapToWindow)
    return nsPlaintextEditor::InsertAsQuotation(aQuotedText, aNodeInserted);

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::insertQuotation, nsIEditor::eNext);

  
  nsTextRulesInfo ruleInfo(EditAction::insertElement);
  bool cancel, handled;
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);
  nsresult rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel || handled) {
    return NS_OK; 
  }

  
  nsCOMPtr<Element> newNode =
    DeleteSelectionAndCreateElement(*nsGkAtoms::span);

  
  
  
  
  if (newNode) {
    
    
    newNode->SetAttr(kNameSpaceID_None, nsGkAtoms::mozquote,
                     NS_LITERAL_STRING("true"), true);
    
    newNode->SetAttr(kNameSpaceID_None, nsGkAtoms::style,
                     NS_LITERAL_STRING("white-space: pre;"), true);

    
    selection->Collapse(newNode, 0);
  }

  if (aAddCites)
    rv = nsPlaintextEditor::InsertAsQuotation(aQuotedText, aNodeInserted);
  else
    rv = nsPlaintextEditor::InsertText(aQuotedText);
  
  
  

  if (aNodeInserted && NS_SUCCEEDED(rv))
  {
    *aNodeInserted = GetAsDOMNode(newNode);
    NS_IF_ADDREF(*aNodeInserted);
  }

  
  if (NS_SUCCEEDED(rv) && newNode)
  {
    nsCOMPtr<nsINode> parent = newNode->GetParentNode();
    int32_t offset = parent ? parent->IndexOf(newNode) : -1;
    if (parent) {
      selection->Collapse(parent, offset + 1);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLEditor::StripCites()
{
  return nsPlaintextEditor::StripCites();
}

NS_IMETHODIMP
nsHTMLEditor::Rewrap(bool aRespectNewlines)
{
  return nsPlaintextEditor::Rewrap(aRespectNewlines);
}

NS_IMETHODIMP
nsHTMLEditor::InsertAsCitedQuotation(const nsAString & aQuotedText,
                                     const nsAString & aCitation,
                                     bool aInsertHTML,
                                     nsIDOMNode **aNodeInserted)
{
  
  if (IsPlaintextEditor())
  {
    NS_ASSERTION(!aInsertHTML, "InsertAsCitedQuotation: trying to insert html into plaintext editor");
    return InsertAsPlaintextQuotation(aQuotedText, true, aNodeInserted);
  }

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::insertQuotation, nsIEditor::eNext);

  
  nsTextRulesInfo ruleInfo(EditAction::insertElement);
  bool cancel, handled;
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);
  nsresult rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel || handled) {
    return NS_OK; 
  }

  nsCOMPtr<Element> newNode =
    DeleteSelectionAndCreateElement(*nsGkAtoms::blockquote);
  NS_ENSURE_TRUE(newNode, NS_ERROR_NULL_POINTER);

  
  newNode->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                   NS_LITERAL_STRING("cite"), true);

  if (!aCitation.IsEmpty()) {
    newNode->SetAttr(kNameSpaceID_None, nsGkAtoms::cite, aCitation, true);
  }

  
  selection->Collapse(newNode, 0);

  if (aInsertHTML)
    rv = LoadHTML(aQuotedText);
  else
    rv = InsertText(aQuotedText);  

  if (aNodeInserted && NS_SUCCEEDED(rv))
  {
    *aNodeInserted = GetAsDOMNode(newNode);
    NS_IF_ADDREF(*aNodeInserted);
  }

  
  if (NS_SUCCEEDED(rv) && newNode)
  {
    nsCOMPtr<nsINode> parent = newNode->GetParentNode();
    int32_t offset = parent ? parent->IndexOf(newNode) : -1;
    if (parent) {
      selection->Collapse(parent, offset + 1);
    }
  }
  return rv;
}


void RemoveBodyAndHead(nsIDOMNode *aNode)
{
  if (!aNode)
    return;

  nsCOMPtr<nsIDOMNode> tmp, child, body, head;
  
  
  aNode->GetFirstChild(getter_AddRefs(child));
  while (child)
  {
    if (nsTextEditUtils::IsBody(child))
    {
      body = child;
    } else if (nsEditor::NodeIsType(child, nsGkAtoms::head)) {
      head = child;
    }
    child->GetNextSibling(getter_AddRefs(tmp));
    child = tmp;
  }
  if (head)
  {
    aNode->RemoveChild(head, getter_AddRefs(tmp));
  }
  if (body)
  {
    body->GetFirstChild(getter_AddRefs(child));
    while (child)
    {
      aNode->InsertBefore(child, body, getter_AddRefs(tmp));
      body->GetFirstChild(getter_AddRefs(child));
    }
    aNode->RemoveChild(body, getter_AddRefs(tmp));
  }
}










nsresult FindTargetNode(nsIDOMNode *aStart, nsCOMPtr<nsIDOMNode> &aResult)
{
  NS_ENSURE_TRUE(aStart, NS_OK);

  nsCOMPtr<nsIDOMNode> child, tmp;

  nsresult rv = aStart->GetFirstChild(getter_AddRefs(child));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!child)
  {
    
    
    if (!aResult)
      aResult = aStart;

    return NS_OK;
  }

  do
  {
    
    nsCOMPtr<nsIDOMComment> comment = do_QueryInterface(child);
    if (comment)
    {
      nsAutoString data;
      rv = comment->GetData(data);
      NS_ENSURE_SUCCESS(rv, rv);

      if (data.EqualsLiteral(kInsertCookie))
      {
        
        
        aResult = aStart;

        
        aStart->RemoveChild(child, getter_AddRefs(tmp));

        return NS_FOUND_TARGET;
      }
    }

    
    
    
    rv = FindTargetNode(child, aResult);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = child->GetNextSibling(getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(rv, rv);

    child = tmp;
  } while (child);

  return NS_OK;
}

nsresult nsHTMLEditor::CreateDOMFragmentFromPaste(const nsAString &aInputString,
                                                  const nsAString & aContextStr,
                                                  const nsAString & aInfoStr,
                                                  nsCOMPtr<nsIDOMNode> *outFragNode,
                                                  nsCOMPtr<nsIDOMNode> *outStartNode,
                                                  nsCOMPtr<nsIDOMNode> *outEndNode,
                                                  int32_t *outStartOffset,
                                                  int32_t *outEndOffset,
                                                  bool aTrustedInput)
{
  NS_ENSURE_TRUE(outFragNode && outStartNode && outEndNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMDocumentFragment> docfrag;
  nsCOMPtr<nsIDOMNode> contextAsNode, tmp;
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDocument> doc = GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDOMDocumentFragment> contextfrag;
  nsCOMPtr<nsIDOMNode> contextLeaf, junk;
  if (!aContextStr.IsEmpty())
  {
    rv = ParseFragment(aContextStr, nullptr, doc, address_of(contextAsNode),
                       aTrustedInput);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(contextAsNode, NS_ERROR_FAILURE);

    rv = StripFormattingNodes(contextAsNode);
    NS_ENSURE_SUCCESS(rv, rv);

    RemoveBodyAndHead(contextAsNode);

    rv = FindTargetNode(contextAsNode, contextLeaf);
    if (rv == NS_FOUND_TARGET) {
      rv = NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIContent> contextLeafAsContent = do_QueryInterface(contextLeaf);

  
  nsIAtom* contextAtom;
  if (contextLeafAsContent) {
    contextAtom = contextLeafAsContent->NodeInfo()->NameAtom();
    if (contextLeafAsContent->IsHTMLElement(nsGkAtoms::html)) {
      contextAtom = nsGkAtoms::body;
    }
  } else {
    contextAtom = nsGkAtoms::body;
  }
  rv = ParseFragment(aInputString,
                     contextAtom,
                     doc,
                     outFragNode,
                     aTrustedInput);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(*outFragNode, NS_ERROR_FAILURE);

  RemoveBodyAndHead(*outFragNode);

  if (contextAsNode)
  {
    
    contextLeaf->AppendChild(*outFragNode, getter_AddRefs(junk));
    *outFragNode = contextAsNode;
  }

  rv = StripFormattingNodes(*outFragNode, true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (contextLeaf)
    *outEndNode = *outStartNode = contextLeaf;
  else
    *outEndNode = *outStartNode = *outFragNode;

  *outStartOffset = 0;

  
  nsAutoString numstr1, numstr2;
  if (!aInfoStr.IsEmpty())
  {
    int32_t sep, num;
    sep = aInfoStr.FindChar((char16_t)',');
    numstr1 = Substring(aInfoStr, 0, sep);
    numstr2 = Substring(aInfoStr, sep+1, aInfoStr.Length() - (sep+1));

    
    nsresult err;
    num = numstr1.ToInteger(&err);
    while (num--)
    {
      (*outStartNode)->GetFirstChild(getter_AddRefs(tmp));
      NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
      tmp.swap(*outStartNode);
    }

    num = numstr2.ToInteger(&err);
    while (num--)
    {
      (*outEndNode)->GetLastChild(getter_AddRefs(tmp));
      NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
      tmp.swap(*outEndNode);
    }
  }

  GetLengthOfDOMNode(*outEndNode, (uint32_t&)*outEndOffset);
  return NS_OK;
}


nsresult nsHTMLEditor::ParseFragment(const nsAString & aFragStr,
                                     nsIAtom* aContextLocalName,
                                     nsIDocument* aTargetDocument,
                                     nsCOMPtr<nsIDOMNode> *outNode,
                                     bool aTrustedInput)
{
  nsAutoScriptBlockerSuppressNodeRemoved autoBlocker;

  nsRefPtr<DocumentFragment> fragment =
    new DocumentFragment(aTargetDocument->NodeInfoManager());
  nsresult rv = nsContentUtils::ParseFragmentHTML(aFragStr,
                                                  fragment,
                                                  aContextLocalName ?
                                                    aContextLocalName : nsGkAtoms::body,
                                                    kNameSpaceID_XHTML,
                                                  false,
                                                  true);
  if (!aTrustedInput) {
    nsTreeSanitizer sanitizer(aContextLocalName ?
                              nsIParserUtils::SanitizerAllowStyle :
                              nsIParserUtils::SanitizerAllowComments);
    sanitizer.Sanitize(fragment);
  }
  *outNode = fragment.forget();
  return rv;
}

nsresult nsHTMLEditor::CreateListOfNodesToPaste(nsIDOMNode  *aFragmentAsNode,
                                                nsCOMArray<nsIDOMNode>& outNodeList,
                                                nsIDOMNode *aStartNode,
                                                int32_t aStartOffset,
                                                nsIDOMNode *aEndNode,
                                                int32_t aEndOffset)
{
  NS_ENSURE_TRUE(aFragmentAsNode, NS_ERROR_NULL_POINTER);

  nsresult rv;

  
  
  if (!aStartNode)
  {
    int32_t fragLen;
    rv = GetLengthOfDOMNode(aFragmentAsNode, (uint32_t&)fragLen);
    NS_ENSURE_SUCCESS(rv, rv);

    aStartNode = aFragmentAsNode;
    aStartOffset = 0;
    aEndNode = aFragmentAsNode;
    aEndOffset = fragLen;
  }

  nsRefPtr<nsRange> docFragRange;
  rv = nsRange::CreateRange(aStartNode, aStartOffset, aEndNode, aEndOffset, getter_AddRefs(docFragRange));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsTrivialFunctor functor;
  nsDOMSubtreeIterator iter;
  rv = iter.Init(docFragRange);
  NS_ENSURE_SUCCESS(rv, rv);

  return iter.AppendList(functor, outNodeList);
}

nsresult
nsHTMLEditor::GetListAndTableParents(bool aEnd,
                                     nsCOMArray<nsIDOMNode>& aListOfNodes,
                                     nsCOMArray<nsIDOMNode>& outArray)
{
  int32_t listCount = aListOfNodes.Count();
  NS_ENSURE_TRUE(listCount > 0, NS_ERROR_FAILURE);  

  
  
  int32_t idx = 0;
  if (aEnd) idx = listCount-1;

  nsCOMPtr<nsIDOMNode> pNode = aListOfNodes[idx];
  while (pNode)
  {
    if (nsHTMLEditUtils::IsList(pNode) || nsHTMLEditUtils::IsTable(pNode))
    {
      NS_ENSURE_TRUE(outArray.AppendObject(pNode), NS_ERROR_FAILURE);
    }
    nsCOMPtr<nsIDOMNode> parent;
    pNode->GetParentNode(getter_AddRefs(parent));
    pNode = parent;
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::DiscoverPartialListsAndTables(nsCOMArray<nsIDOMNode>& aPasteNodes,
                                            nsCOMArray<nsIDOMNode>& aListsAndTables,
                                            int32_t *outHighWaterMark)
{
  NS_ENSURE_TRUE(outHighWaterMark, NS_ERROR_NULL_POINTER);
  
  *outHighWaterMark = -1;
  int32_t listAndTableParents = aListsAndTables.Count();
  
  
  int32_t listCount = aPasteNodes.Count();
  int32_t j;  
  for (j=0; j<listCount; j++)
  {
    nsCOMPtr<nsIDOMNode> curNode = aPasteNodes[j];

    NS_ENSURE_TRUE(curNode, NS_ERROR_FAILURE);
    if (nsHTMLEditUtils::IsTableElement(curNode) && !nsHTMLEditUtils::IsTable(curNode))
    {
      nsCOMPtr<nsIDOMNode> theTable = GetTableParent(curNode);
      if (theTable)
      {
        int32_t indexT = aListsAndTables.IndexOf(theTable);
        if (indexT >= 0)
        {
          *outHighWaterMark = indexT;
          if (*outHighWaterMark == listAndTableParents-1) break;
        }
        else
        {
          break;
        }
      }
    }
    if (nsHTMLEditUtils::IsListItem(curNode))
    {
      nsCOMPtr<nsIDOMNode> theList = GetListParent(curNode);
      if (theList)
      {
        int32_t indexL = aListsAndTables.IndexOf(theList);
        if (indexL >= 0)
        {
          *outHighWaterMark = indexL;
          if (*outHighWaterMark == listAndTableParents-1) break;
        }
        else
        {
          break;
        }
      }
    }
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::ScanForListAndTableStructure( bool aEnd,
                                            nsCOMArray<nsIDOMNode>& aNodes,
                                            nsIDOMNode *aListOrTable,
                                            nsCOMPtr<nsIDOMNode> *outReplaceNode)
{
  NS_ENSURE_TRUE(aListOrTable, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(outReplaceNode, NS_ERROR_NULL_POINTER);

  *outReplaceNode = 0;
  
  
  int32_t listCount = aNodes.Count(), idx = 0;
  if (aEnd) idx = listCount-1;
  bool bList = nsHTMLEditUtils::IsList(aListOrTable);
  
  nsCOMPtr<nsIDOMNode>  pNode = aNodes[idx];
  nsCOMPtr<nsIDOMNode>  originalNode = pNode;
  while (pNode)
  {
    if ((bList && nsHTMLEditUtils::IsListItem(pNode)) ||
        (!bList && (nsHTMLEditUtils::IsTableElement(pNode) && !nsHTMLEditUtils::IsTable(pNode))))
    {
      nsCOMPtr<nsIDOMNode> structureNode;
      if (bList) structureNode = GetListParent(pNode);
      else structureNode = GetTableParent(pNode);
      if (structureNode == aListOrTable)
      {
        if (bList)
          *outReplaceNode = structureNode;
        else
          *outReplaceNode = pNode;
        break;
      }
    }
    nsCOMPtr<nsIDOMNode> parent;
    pNode->GetParentNode(getter_AddRefs(parent));
    pNode = parent;
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::ReplaceOrphanedStructure(bool aEnd,
                                       nsCOMArray<nsIDOMNode>& aNodeArray,
                                       nsCOMArray<nsIDOMNode>& aListAndTableArray,
                                       int32_t aHighWaterMark)
{
  nsCOMPtr<nsIDOMNode> curNode = aListAndTableArray[aHighWaterMark];
  NS_ENSURE_TRUE(curNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> replaceNode, originalNode;

  
  nsresult rv = ScanForListAndTableStructure(aEnd, aNodeArray,
                                 curNode, address_of(replaceNode));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (replaceNode)
  {
    
    
    nsCOMPtr<nsIDOMNode> endpoint;
    do
    {
      endpoint = GetArrayEndpoint(aEnd, aNodeArray);
      if (!endpoint) break;
      if (nsEditorUtils::IsDescendantOf(endpoint, replaceNode))
        aNodeArray.RemoveObject(endpoint);
      else
        break;
    } while(endpoint);

    
    if (aEnd) aNodeArray.AppendObject(replaceNode);
    else aNodeArray.InsertObjectAt(replaceNode, 0);
  }
  return NS_OK;
}

nsIDOMNode* nsHTMLEditor::GetArrayEndpoint(bool aEnd,
                                           nsCOMArray<nsIDOMNode>& aNodeArray)
{
  int32_t listCount = aNodeArray.Count();
  if (listCount <= 0) {
    return nullptr;
  }

  if (aEnd) {
    return aNodeArray[listCount-1];
  }

  return aNodeArray[0];
}
