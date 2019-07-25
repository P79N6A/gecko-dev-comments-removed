





































#include "mozilla/Util.h"

#include "nsHTMLEditor.h"
#include "nsHTMLEditRules.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsWSRunObject.h"

#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMComment.h"
#include "nsIDOMNodeList.h"
#include "nsIDocument.h"
#include "nsIDOMMouseEvent.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsISelectionController.h"
#include "nsIFileChannel.h"
#include "nsIPrivateDOMEvent.h"

#include "nsIDocumentObserver.h"
#include "nsIDocumentStateListener.h"

#include "nsIEnumerator.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMRange.h"
#include "nsIDOMDOMStringList.h"
#include "nsIDOMDragEvent.h"
#include "nsCOMArray.h"
#include "nsIFile.h"
#include "nsIURL.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIDocumentEncoder.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsLinebreakConverter.h"
#include "nsHtml5Module.h"
#include "nsTreeSanitizer.h"


#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIMIMEService.h"


#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIDragService.h"
#include "nsIDOMUIEvent.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsDirectoryServiceDefs.h"


#include "nsUnicharUtils.h"
#include "nsIDOMTreeWalker.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMHTMLTableRowElement.h"
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLBodyElement.h"


#include "nsEditorUtils.h"
#include "nsIContentFilter.h"
#include "nsEventDispatcher.h"
#include "plbase64.h"
#include "prmem.h"
#include "nsStreamUtils.h"
#include "nsIPrincipal.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"
#include "nsIParserUtils.h"

using namespace mozilla;

const PRUnichar nbsp = 160;

#define kInsertCookie  "_moz_Insert Here_moz_"

#define NS_FOUND_TARGET NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_EDITOR, 3)


static bool FindIntegerAfterString(const char *aLeadingString, 
                                     nsCString &aCStr, PRInt32 &foundNumber);
static nsresult RemoveFragComments(nsCString &theStr);
static void RemoveBodyAndHead(nsIDOMNode *aNode);
static nsresult FindTargetNode(nsIDOMNode *aStart, nsCOMPtr<nsIDOMNode> &aResult);

static nsCOMPtr<nsIDOMNode> GetListParent(nsIDOMNode* aNode)
{
  NS_ENSURE_TRUE(aNode, nsnull);
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
  return nsnull;
}

static nsCOMPtr<nsIDOMNode> GetTableParent(nsIDOMNode* aNode)
{
  NS_ENSURE_TRUE(aNode, nsnull);
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
  return nsnull;
}


NS_IMETHODIMP nsHTMLEditor::LoadHTML(const nsAString & aInputString)
{
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);

  
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpLoadHTML, nsIEditor::eNext);

  
  nsCOMPtr<nsISelection>selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  nsTextRulesInfo ruleInfo(kOpLoadHTML);
  bool cancel, handled;
  rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel) {
    return NS_OK; 
  }

  if (!handled)
  {
    bool isCollapsed;
    rv = selection->GetIsCollapsed(&isCollapsed);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!isCollapsed) 
    {
      rv = DeleteSelection(eNone, eStrip);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    nsCOMPtr<nsIDOMRange> range;
    rv = selection->GetRangeAt(0, getter_AddRefs(range));
    NS_ENSURE_SUCCESS(rv, rv);
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
    PRInt32 childOffset;
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
                               nsnull,  nsnull, 0, true);
}


nsresult
nsHTMLEditor::InsertHTMLWithContext(const nsAString & aInputString,
                                    const nsAString & aContextStr,
                                    const nsAString & aInfoStr,
                                    const nsAString & aFlavor,
                                    nsIDOMDocument *aSourceDoc,
                                    nsIDOMNode *aDestNode,
                                    PRInt32 aDestOffset,
                                    bool aDeleteSelection)
{
  return DoInsertHTMLWithContext(aInputString, aContextStr, aInfoStr,
      aFlavor, aSourceDoc, aDestNode, aDestOffset, aDeleteSelection,
      true);
}

nsresult
nsHTMLEditor::DoInsertHTMLWithContext(const nsAString & aInputString,
                                      const nsAString & aContextStr,
                                      const nsAString & aInfoStr,
                                      const nsAString & aFlavor,
                                      nsIDOMDocument *aSourceDoc,
                                      nsIDOMNode *aDestNode,
                                      PRInt32 aDestOffset,
                                      bool aDeleteSelection,
                                      bool aTrustedInput)
{
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);

  
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpHTMLPaste, nsIEditor::eNext);

  
  nsCOMPtr<nsISelection>selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMNode> fragmentAsNode, streamStartParent, streamEndParent;
  PRInt32 streamStartOffset = 0, streamEndOffset = 0;

  rv = CreateDOMFragmentFromPaste(aInputString, aContextStr, aInfoStr, 
                                  address_of(fragmentAsNode),
                                  address_of(streamStartParent),
                                  address_of(streamEndParent),
                                  &streamStartOffset,
                                  &streamEndOffset,
                                  aTrustedInput);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> targetNode, tempNode;
  PRInt32 targetOffset=0;

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
  PRInt32 offsetOfNewNode;

  
  bool cellSelectionMode = false;
  nsCOMPtr<nsIDOMElement> cell;
  rv = GetFirstSelectedCell(nsnull, getter_AddRefs(cell));
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
    rv = DeleteSelectionAndPrepareToCreateNode(parentNode, offsetOfNewNode);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = RemoveAllInlineProperties();
    NS_ENSURE_SUCCESS(rv, rv);
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

  
  nsTextRulesInfo ruleInfo(kOpInsertElement);
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
    if (nsTextEditUtils::IsBreak(wsObj.mEndReasonNode) && 
        !IsVisBreak(wsObj.mEndReasonNode) )
    {
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

    
    PRInt32 highWaterMark = -1;
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
    PRInt32 listCount = nodeList.Count();
    PRInt32 j;
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
                nsCOMPtr<nsIDOMNode> listNode;
                PRInt32 newOffset;
                GetNodeLocation(parentNode, address_of(listNode), &newOffset);
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

      }
      
      else if (nsHTMLEditUtils::IsPre(parentBlock) && nsHTMLEditUtils::IsPre(curNode))
      {
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
        rv = GetNodeLocation(lastInsertNode, address_of(parentNode), &offsetOfNewNode);
        NS_ENSURE_SUCCESS(rv, rv);
        offsetOfNewNode++;
      }
    }

    
    if (lastInsertNode) 
    {
      
      nsCOMPtr<nsIDOMNode> selNode, tmp, visNode, highTable;
      PRInt32 selOffset;

      
      if (!nsHTMLEditUtils::IsTable(lastInsertNode))
      {
        rv = GetLastEditableLeaf(lastInsertNode, address_of(selNode));
        NS_ENSURE_SUCCESS(rv, rv);
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
        rv = GetLengthOfDOMNode(selNode, (PRUint32&)selOffset);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else 
      {
        tmp = selNode;
        rv = GetNodeLocation(tmp, address_of(selNode), &selOffset);
        ++selOffset;  
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      nsWSRunObject wsRunObj(this, selNode, selOffset);
      PRInt32 outVisOffset=0;
      PRInt16 visType=0;
      rv = wsRunObj.PriorVisibleNode(selNode, selOffset, address_of(visNode), &outVisOffset, &visType);
      NS_ENSURE_SUCCESS(rv, rv);
      if (visType == nsWSRunObject::eBreak)
      {
        
        
        
        
        if (!IsVisBreak(wsRunObj.mStartReasonNode))
        {
          
          
          rv = GetNodeLocation(wsRunObj.mStartReasonNode, address_of(selNode), &selOffset);
          
          nsWSRunObject wsRunObj(this, selNode, selOffset);
          rv = wsRunObj.PriorVisibleNode(selNode, selOffset, address_of(visNode), &outVisOffset, &visType);
          NS_ENSURE_SUCCESS(rv, rv);
          if (visType == nsWSRunObject::eText ||
              visType == nsWSRunObject::eNormalWS)
          {
            selNode = visNode;
            selOffset = outVisOffset;  
          }
          else if (visType == nsWSRunObject::eSpecial)
          {
            
            
            rv = GetNodeLocation(wsRunObj.mStartReasonNode, address_of(selNode), &selOffset);
            ++selOffset;
          }
        }
      }
      selection->Collapse(selNode, selOffset);

      
      nsCOMPtr<nsIDOMNode> link;
      if (!bStartedInLink && IsInLink(selNode, address_of(link)))
      {
        
        
        
        
        nsCOMPtr<nsIDOMNode> leftLink;
        PRInt32 linkOffset;
        rv = SplitNodeDeep(link, selNode, selOffset, &linkOffset, true, address_of(leftLink));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = GetNodeLocation(leftLink, address_of(selNode), &selOffset);
        NS_ENSURE_SUCCESS(rv, rv);
        selection->Collapse(selNode, selOffset+1);
      }
    }
  }

  return mRules->DidDoAction(selection, &ruleInfo, rv);
}


nsresult
nsHTMLEditor::GetAttributeToModifyOnNode(nsIDOMNode *aNode, nsAString &aAttr)
{
  aAttr.Truncate();

  NS_NAMED_LITERAL_STRING(srcStr, "src");
  nsCOMPtr<nsIDOMHTMLImageElement> nodeAsImage = do_QueryInterface(aNode);
  if (nodeAsImage)
  {
    aAttr = srcStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLAnchorElement> nodeAsAnchor = do_QueryInterface(aNode);
  if (nodeAsAnchor)
  {
    aAttr.AssignLiteral("href");
    return NS_OK;
  }

  NS_NAMED_LITERAL_STRING(bgStr, "background");
  nsCOMPtr<nsIDOMHTMLBodyElement> nodeAsBody = do_QueryInterface(aNode);
  if (nodeAsBody)
  {
    aAttr = bgStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLTableElement> nodeAsTable = do_QueryInterface(aNode);
  if (nodeAsTable)
  {
    aAttr = bgStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLTableRowElement> nodeAsTableRow = do_QueryInterface(aNode);
  if (nodeAsTableRow)
  {
    aAttr = bgStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLTableCellElement> nodeAsTableCell = do_QueryInterface(aNode);
  if (nodeAsTableCell)
  {
    aAttr = bgStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLScriptElement> nodeAsScript = do_QueryInterface(aNode);
  if (nodeAsScript)
  {
    aAttr = srcStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLEmbedElement> nodeAsEmbed = do_QueryInterface(aNode);
  if (nodeAsEmbed)
  {
    aAttr = srcStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLObjectElement> nodeAsObject = do_QueryInterface(aNode);
  if (nodeAsObject)
  {
    aAttr.AssignLiteral("data");
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLLinkElement> nodeAsLink = do_QueryInterface(aNode);
  if (nodeAsLink)
  {
    
    nsAutoString linkRel;
    if (NS_SUCCEEDED(nodeAsLink->GetRel(linkRel)) && !linkRel.IsEmpty())
    {
      nsReadingIterator<PRUnichar> start;
      nsReadingIterator<PRUnichar> end;
      nsReadingIterator<PRUnichar> current;

      linkRel.BeginReading(start);
      linkRel.EndReading(end);

      
      for (current = start; current != end; ++current)
      {
        
        if (nsCRT::IsAsciiSpace(*current))
          continue;

        
        nsReadingIterator<PRUnichar> startWord = current;
        do {
          ++current;
        } while (current != end && !nsCRT::IsAsciiSpace(*current));

        
        if (Substring(startWord, current).LowerCaseEqualsLiteral("stylesheet"))
        {
          aAttr.AssignLiteral("href");
          return NS_OK;
        }
        if (current == end)
          break;
      }
    }
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLFrameElement> nodeAsFrame = do_QueryInterface(aNode);
  if (nodeAsFrame)
  {
    aAttr = srcStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLIFrameElement> nodeAsIFrame = do_QueryInterface(aNode);
  if (nodeAsIFrame)
  {
    aAttr = srcStr;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLInputElement> nodeAsInput = do_QueryInterface(aNode);
  if (nodeAsInput)
  {
    aAttr = srcStr;
    return NS_OK;
  }

  return NS_OK;
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
                                      PRInt32 *aFragStartOffset,
                                      nsIDOMNode **aFragEndNode, 
                                      PRInt32 *aFragEndOffset,
                                      nsIDOMNode **aTargetNode,
                                      PRInt32 *aTargetOffset,
                                      bool *aDoContinue)
{
  *aDoContinue = true;

  PRInt32 i;
  nsIContentFilter *listener;
  for (i=0; i < mContentFilters.Count() && *aDoContinue; i++)
  {
    listener = (nsIContentFilter *)mContentFilters[i];
    if (listener)
      listener->NotifyOfInsertion(aFlavor, nsnull, sourceDoc,
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
    *outLink = nsnull;
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

NS_IMETHODIMP nsHTMLEditor::PrepareHTMLTransferable(nsITransferable **aTransferable, 
                                                    bool aHavePrivFlavor)
{
  
  nsresult rv = CallCreateInstance("@mozilla.org/widget/transferable;1", aTransferable);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aTransferable)
  {
    
    
    
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
          (*aTransferable)->AddDataFlavor(kPNGImageMime);
          (*aTransferable)->AddDataFlavor(kGIFImageMime);
          break;
        case 1:  
        default:
          (*aTransferable)->AddDataFlavor(kPNGImageMime);
          (*aTransferable)->AddDataFlavor(kJPEGImageMime);
          (*aTransferable)->AddDataFlavor(kGIFImageMime);
          break;
        case 2:  
          (*aTransferable)->AddDataFlavor(kGIFImageMime);
          (*aTransferable)->AddDataFlavor(kJPEGImageMime);
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
                       nsCString &aCStr, PRInt32 &foundNumber)
{
  
  PRInt32 numFront = aCStr.Find(aLeadingString);
  if (numFront == -1)
    return false;
  numFront += strlen(aLeadingString);

  PRInt32 numBack = aCStr.FindCharInSet(CRLF, numFront);
  if (numBack == -1)
    return false;

  nsCAutoString numStr(Substring(aCStr, numFront, numBack-numFront));
  PRInt32 errorCode;
  foundNumber = numStr.ToInteger(&errorCode);
  return true;
}

nsresult
RemoveFragComments(nsCString & aStr)
{
  
  PRInt32 startCommentIndx = aStr.Find("<!--StartFragment");
  if (startCommentIndx >= 0)
  {
    PRInt32 startCommentEnd = aStr.Find("-->", false, startCommentIndx);
    if (startCommentEnd > startCommentIndx)
      aStr.Cut(startCommentIndx, (startCommentEnd+3)-startCommentIndx);
  }
  PRInt32 endCommentIndx = aStr.Find("<!--EndFragment");
  if (endCommentIndx >= 0)
  {
    PRInt32 endCommentEnd = aStr.Find("-->", false, endCommentIndx);
    if (endCommentEnd > endCommentIndx)
      aStr.Cut(endCommentIndx, (endCommentEnd+3)-endCommentIndx);
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::ParseCFHTML(nsCString & aCfhtml, PRUnichar **aStuffToPaste, PRUnichar **aCfcontext)
{
  
  PRInt32 startHTML, endHTML, startFragment, endFragment;
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

  
  nsCAutoString contextUTF8(Substring(aCfhtml, startHTML, startFragment - startHTML) +
                            NS_LITERAL_CSTRING("<!--" kInsertCookie "-->") +
                            Substring(aCfhtml, endFragment, endHTML - endFragment));

  
  
  
  PRInt32 curPos = startFragment;
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

  
  nsCAutoString fragmentUTF8(Substring(aCfhtml, startFragment, endFragment-startFragment));

  
  RemoveFragComments(fragmentUTF8);

  
  RemoveFragComments(contextUTF8);

  
  const nsAFlatString& fragUcs2Str = NS_ConvertUTF8toUTF16(fragmentUTF8);
  const nsAFlatString& cntxtUcs2Str = NS_ConvertUTF8toUTF16(contextUTF8);

  
  PRInt32 oldLengthInChars = fragUcs2Str.Length() + 1;  
  PRInt32 newLengthInChars = 0;
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

bool nsHTMLEditor::IsSafeToInsertData(nsIDOMDocument* aSourceDoc)
{
  
  bool isSafe = false;

  nsCOMPtr<nsIDocument> destdoc = GetDocument();
  NS_ASSERTION(destdoc, "Where is our destination doc?");
  nsCOMPtr<nsISupports> container = destdoc->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(container);
  nsCOMPtr<nsIDocShellTreeItem> root;
  if (dsti)
    dsti->GetRootTreeItem(getter_AddRefs(root));
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(root);
  PRUint32 appType;
  if (docShell && NS_SUCCEEDED(docShell->GetAppType(&appType)))
    isSafe = appType == nsIDocShell::APP_TYPE_EDITOR;
  if (!isSafe && aSourceDoc) {
    nsCOMPtr<nsIDocument> srcdoc = do_QueryInterface(aSourceDoc);
    NS_ASSERTION(srcdoc, "Where is our source doc?");

    nsIPrincipal* srcPrincipal = srcdoc->NodePrincipal();
    nsIPrincipal* destPrincipal = destdoc->NodePrincipal();
    NS_ASSERTION(srcPrincipal && destPrincipal, "How come we don't have a principal?");
    srcPrincipal->Subsumes(destPrincipal, &isSafe);
  }

  return isSafe;
}

nsresult nsHTMLEditor::InsertObject(const char* aType, nsISupports* aObject, bool aIsSafe,
                                    nsIDOMDocument *aSourceDoc,
                                    nsIDOMNode *aDestinationNode,
                                    PRInt32 aDestOffset,
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
      nsCAutoString contentType;
      rv = mime->GetTypeFromFile(fileObj, contentType);
      NS_ENSURE_SUCCESS(rv, rv);

      
      if (StringBeginsWith(contentType, NS_LITERAL_CSTRING("image/"))) {
        insertAsImage = true;
        type = contentType.get();
      }
    }
  }

  if (0 == nsCRT::strcmp(type, kJPEGImageMime) ||
      0 == nsCRT::strcmp(type, kPNGImageMime) ||
      0 == nsCRT::strcmp(type, kGIFImageMime) ||
      insertAsImage)
  {
    nsCOMPtr<nsIInputStream> imageStream;
    if (insertAsImage) {
      NS_ASSERTION(fileURI, "The file URI should be retrieved earlier");
      rv = NS_OpenURI(getter_AddRefs(imageStream), fileURI);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      imageStream = do_QueryInterface(aObject);
      NS_ENSURE_TRUE(imageStream, NS_ERROR_FAILURE);
    }

    nsCString imageData;
    rv = NS_ConsumeStream(imageStream, PR_UINT32_MAX, imageData);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = imageStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    char * base64 = PL_Base64Encode(imageData.get(), imageData.Length(), nsnull);
    NS_ENSURE_TRUE(base64, NS_ERROR_OUT_OF_MEMORY);

    nsAutoString stuffToPaste;
    stuffToPaste.AssignLiteral("<IMG src=\"data:");
    AppendUTF8toUTF16(type, stuffToPaste);
    stuffToPaste.AppendLiteral(";base64,");
    AppendUTF8toUTF16(base64, stuffToPaste);
    stuffToPaste.AppendLiteral("\" alt=\"\" >");
    nsAutoEditBatch beginBatching(this);
    rv = DoInsertHTMLWithContext(stuffToPaste, EmptyString(), EmptyString(), 
                                 NS_LITERAL_STRING(kFileMime),
                                 aSourceDoc,
                                 aDestinationNode, aDestOffset,
                                 aDoDeleteSelection,
                                 aIsSafe);
    PR_Free(base64);
  }

  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::InsertFromTransferable(nsITransferable *transferable, 
                                                   nsIDOMDocument *aSourceDoc,
                                                   const nsAString & aContextStr,
                                                   const nsAString & aInfoStr,
                                                   nsIDOMNode *aDestinationNode,
                                                   PRInt32 aDestOffset,
                                                   bool aDoDeleteSelection)
{
  nsresult rv = NS_OK;
  nsXPIDLCString bestFlavor;
  nsCOMPtr<nsISupports> genericDataObj;
  PRUint32 len = 0;
  if (NS_SUCCEEDED(transferable->GetAnyTransferData(getter_Copies(bestFlavor), getter_AddRefs(genericDataObj), &len)))
  {
    nsAutoTxnsConserveSelection dontSpazMySelection(this);
    nsAutoString flavor;
    flavor.AssignWithConversion(bestFlavor);
    nsAutoString stuffToPaste;
#ifdef DEBUG_clipboard
    printf("Got flavor [%s]\n", bestFlavor.get());
#endif

    bool isSafe = IsSafeToInsertData(aSourceDoc);

    if (0 == nsCRT::strcmp(bestFlavor, kFileMime) ||
        0 == nsCRT::strcmp(bestFlavor, kJPEGImageMime) ||
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
        nsCAutoString cfhtml;
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
        }
      }
    }
    else if (0 == nsCRT::strcmp(bestFlavor, kHTMLMime) ||
             0 == nsCRT::strcmp(bestFlavor, kUnicodeMime) ||
             0 == nsCRT::strcmp(bestFlavor, kMozTextInternal)) {
      nsCOMPtr<nsISupportsString> textDataObj = do_QueryInterface(genericDataObj);
      if (textDataObj && len > 0)
      {
        nsAutoString text;
        textDataObj->GetData(text);
        NS_ASSERTION(text.Length() <= (len/2), "Invalid length!");
        stuffToPaste.Assign(text.get(), len / 2);

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
                          PRInt32 aIndex, nsAString& aOutputString)
{
  nsCOMPtr<nsIVariant> variant;
  aDataTransfer->MozGetDataAt(aType, aIndex, getter_AddRefs(variant));
  if (variant)
    variant->GetAsAString(aOutputString);
}

nsresult nsHTMLEditor::InsertFromDataTransfer(nsIDOMDataTransfer *aDataTransfer,
                                              PRInt32 aIndex,
                                              nsIDOMDocument *aSourceDoc,
                                              nsIDOMNode *aDestinationNode,
                                              PRInt32 aDestOffset,
                                              bool aDoDeleteSelection)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDOMDOMStringList> types;
  aDataTransfer->MozTypesAt(aIndex, getter_AddRefs(types));

  bool hasPrivateHTMLFlavor;
  types->Contains(NS_LITERAL_STRING(kHTMLContext), &hasPrivateHTMLFlavor);

  bool isText = IsPlaintextEditor();
  bool isSafe = IsSafeToInsertData(aSourceDoc);
  
  PRUint32 length;
  types->GetLength(&length);
  for (PRUint32 t = 0; t < length; t++) {
    nsAutoString type;
    types->Item(t, type);

    if (!isText) {
      if (type.EqualsLiteral(kFileMime) ||
          type.EqualsLiteral(kJPEGImageMime) ||
          type.EqualsLiteral(kPNGImageMime) ||
          type.EqualsLiteral(kGIFImageMime)) {
        nsCOMPtr<nsIVariant> variant;
        aDataTransfer->MozGetDataAt(type, aIndex, getter_AddRefs(variant));
        if (variant) {
          nsCOMPtr<nsISupports> object;
          variant->GetAsISupports(getter_AddRefs(object));
          rv = InsertObject(NS_ConvertUTF16toUTF8(type).get(), object, isSafe,
                            aSourceDoc, aDestinationNode, aDestOffset, aDoDeleteSelection);
          if (NS_SUCCEEDED(rv))
            return NS_OK;
        }
      }
      else if (!hasPrivateHTMLFlavor && type.EqualsLiteral(kNativeHTMLMime)) {
        nsAutoString text;
        GetStringFromDataTransfer(aDataTransfer, NS_LITERAL_STRING(kNativeHTMLMime), aIndex, text);
        NS_ConvertUTF16toUTF8 cfhtml(text);

        nsXPIDLString cfcontext, cffragment, cfselection; 

        rv = ParseCFHTML(cfhtml, getter_Copies(cffragment), getter_Copies(cfcontext));
        if (NS_SUCCEEDED(rv) && !cffragment.IsEmpty())
        {
          nsAutoEditBatch beginBatching(this);
          rv = DoInsertHTMLWithContext(cffragment,
                                       cfcontext, cfselection, type,
                                       aSourceDoc,
                                       aDestinationNode, aDestOffset,
                                       aDoDeleteSelection,
                                       isSafe);
          if (NS_SUCCEEDED(rv))
            return NS_OK;
        }
      }
      else if (type.EqualsLiteral(kHTMLMime)) {
        nsAutoString text, contextString, infoString;
        GetStringFromDataTransfer(aDataTransfer, type, aIndex, text);
        GetStringFromDataTransfer(aDataTransfer, NS_LITERAL_STRING(kHTMLContext), aIndex, contextString);
        GetStringFromDataTransfer(aDataTransfer, NS_LITERAL_STRING(kHTMLInfo), aIndex, infoString);

        nsAutoEditBatch beginBatching(this);
        if (type.EqualsLiteral(kHTMLMime)) {
          rv = DoInsertHTMLWithContext(text,
                                       contextString, infoString, type,
                                       aSourceDoc,
                                       aDestinationNode, aDestOffset,
                                       aDoDeleteSelection,
                                       isSafe);
          if (NS_SUCCEEDED(rv))
            return NS_OK;
        }
      }
    }

    if (type.EqualsLiteral(kTextMime) ||
        type.EqualsLiteral(kMozTextInternal)) {
      nsAutoString text;
      GetStringFromDataTransfer(aDataTransfer, type, aIndex, text);

      nsAutoEditBatch beginBatching(this);
      rv = InsertTextAt(text, aDestinationNode, aDestOffset, aDoDeleteSelection);
      if (NS_SUCCEEDED(rv))
        return NS_OK;
    }
  }

  return rv;
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


NS_IMETHODIMP nsHTMLEditor::Paste(PRInt32 aSelectionType)
{
  if (!FireClipboardEvent(NS_PASTE))
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
    PRUint32 contextLen, infoLen;
    nsCOMPtr<nsISupportsString> textDataObj;

    nsCOMPtr<nsITransferable> contextTrans =
                  do_CreateInstance("@mozilla.org/widget/transferable;1");
    NS_ENSURE_TRUE(contextTrans, NS_ERROR_NULL_POINTER);
    contextTrans->AddDataFlavor(kHTMLContext);
    clipboard->GetData(contextTrans, aSelectionType);
    contextTrans->GetTransferData(kHTMLContext, getter_AddRefs(contextDataObj), &contextLen);

    nsCOMPtr<nsITransferable> infoTrans =
                  do_CreateInstance("@mozilla.org/widget/transferable;1");
    NS_ENSURE_TRUE(infoTrans, NS_ERROR_NULL_POINTER);
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
  if (!nsEditorHookUtils::DoInsertionHook(domdoc, nsnull, trans))
    return NS_OK;

  return InsertFromTransferable(trans, nsnull, contextStr, infoStr,
                                nsnull, 0, true);
}

NS_IMETHODIMP nsHTMLEditor::PasteTransferable(nsITransferable *aTransferable)
{
  if (!FireClipboardEvent(NS_PASTE))
    return NS_OK;

  
  nsCOMPtr<nsIDOMDocument> domdoc = GetDOMDocument();
  if (!nsEditorHookUtils::DoInsertionHook(domdoc, nsnull, aTransferable))
    return NS_OK;

  nsAutoString contextStr, infoStr;
  return InsertFromTransferable(aTransferable, nsnull, contextStr, infoStr,
                                nsnull, 0, true);
}




NS_IMETHODIMP nsHTMLEditor::PasteNoFormatting(PRInt32 aSelectionType)
{
  if (!FireClipboardEvent(NS_PASTE))
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
      rv = InsertFromTransferable(trans, nsnull, empty, empty, nsnull, 0,
                                  true);
    }
  }

  return rv;
}





static const char* textEditorFlavors[] = { kUnicodeMime };
static const char* textHtmlEditorFlavors[] = { kUnicodeMime, kHTMLMime,
                                               kJPEGImageMime, kPNGImageMime,
                                               kGIFImageMime };

NS_IMETHODIMP nsHTMLEditor::CanPaste(PRInt32 aSelectionType, bool *aCanPaste)
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
    PRUint32 dataLen;
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





NS_IMETHODIMP nsHTMLEditor::PasteAsQuotation(PRInt32 aSelectionType)
{
  if (IsPlaintextEditor())
    return PasteAsPlaintextQuotation(aSelectionType);

  nsAutoString citation;
  return PasteAsCitedQuotation(citation, aSelectionType);
}

NS_IMETHODIMP nsHTMLEditor::PasteAsCitedQuotation(const nsAString & aCitation,
                                                  PRInt32 aSelectionType)
{
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertQuotation, nsIEditor::eNext);

  
  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  nsTextRulesInfo ruleInfo(kOpInsertElement);
  bool cancel, handled;
  rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel || handled) {
    return NS_OK; 
  }

  nsCOMPtr<nsIDOMNode> newNode;
  rv = DeleteSelectionAndCreateNode(NS_LITERAL_STRING("blockquote"), getter_AddRefs(newNode));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(newNode, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsIDOMElement> newElement = do_QueryInterface(newNode);
  if (newElement) {
    newElement->SetAttribute(NS_LITERAL_STRING("type"), NS_LITERAL_STRING("cite"));
  }

  
  rv = selection->Collapse(newNode, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  return Paste(aSelectionType);
}




NS_IMETHODIMP nsHTMLEditor::PasteAsPlaintextQuotation(PRInt32 aSelectionType)
{
  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransferable> trans =
                 do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(trans, NS_ERROR_FAILURE);

  
  trans->AddDataFlavor(kUnicodeMime);

  
  clipboard->GetData(trans, aSelectionType);

  
  
  
  nsCOMPtr<nsISupports> genericDataObj;
  PRUint32 len = 0;
  char* flav = 0;
  rv = trans->GetAnyTransferData(&flav, getter_AddRefs(genericDataObj), &len);
  NS_ENSURE_SUCCESS(rv, rv);

  if (flav && 0 == nsCRT::strcmp(flav, kUnicodeMime))
  {
#ifdef DEBUG_clipboard
    printf("Got flavor [%s]\n", flav);
#endif
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

  
  
  
  

  static const PRUnichar cite('>');
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
#ifdef DEBUG_akkana_verbose
    printf("==== Inserting text as %squoted: ---\n%s---\n",
           curHunkIsQuoted ? "" : "non-",
           NS_LossyConvertUTF16toASCII(curHunk).get());
#endif
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

  nsCOMPtr<nsIDOMNode> newNode;
  
  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertQuotation, nsIEditor::eNext);

  
  nsTextRulesInfo ruleInfo(kOpInsertElement);
  bool cancel, handled;
  rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel || handled) {
    return NS_OK; 
  }

  
  rv = DeleteSelectionAndCreateNode(NS_LITERAL_STRING("span"), getter_AddRefs(newNode));

  
  
  
  
  if (NS_SUCCEEDED(rv) && newNode)
  {
    
    
    nsCOMPtr<nsIDOMElement> preElement = do_QueryInterface(newNode);
    if (preElement)
    {
      preElement->SetAttribute(NS_LITERAL_STRING("_moz_quote"),
                               NS_LITERAL_STRING("true"));
      
      preElement->SetAttribute(NS_LITERAL_STRING("style"),
                               NS_LITERAL_STRING("white-space: pre;"));
    }
    
    selection->Collapse(newNode, 0);
  }

  if (aAddCites)
    rv = nsPlaintextEditor::InsertAsQuotation(aQuotedText, aNodeInserted);
  else
    rv = nsPlaintextEditor::InsertText(aQuotedText);
  
  
  

  if (aNodeInserted && NS_SUCCEEDED(rv))
  {
    *aNodeInserted = newNode;
    NS_IF_ADDREF(*aNodeInserted);
  }

  
  if (NS_SUCCEEDED(rv) && newNode)
  {
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    if (NS_SUCCEEDED(GetNodeLocation(newNode, address_of(parent), &offset)) && parent)
      selection->Collapse(parent, offset+1);
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

  nsCOMPtr<nsIDOMNode> newNode;

  
  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertQuotation, nsIEditor::eNext);

  
  nsTextRulesInfo ruleInfo(kOpInsertElement);
  bool cancel, handled;
  rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel || handled) {
    return NS_OK; 
  }

  rv = DeleteSelectionAndCreateNode(NS_LITERAL_STRING("blockquote"), getter_AddRefs(newNode));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(newNode, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsIDOMElement> newElement = do_QueryInterface(newNode);
  if (newElement)
  {
    NS_NAMED_LITERAL_STRING(citeStr, "cite");
    newElement->SetAttribute(NS_LITERAL_STRING("type"), citeStr);

    if (!aCitation.IsEmpty())
      newElement->SetAttribute(citeStr, aCitation);

    
    selection->Collapse(newNode, 0);
  }

  if (aInsertHTML)
    rv = LoadHTML(aQuotedText);
  else
    rv = InsertText(aQuotedText);  

  if (aNodeInserted && NS_SUCCEEDED(rv))
  {
    *aNodeInserted = newNode;
    NS_IF_ADDREF(*aNodeInserted);
  }

  
  if (NS_SUCCEEDED(rv) && newNode)
  {
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    if (NS_SUCCEEDED(GetNodeLocation(newNode, address_of(parent), &offset)) && parent)
      selection->Collapse(parent, offset+1);
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
    }
    else if (nsEditor::NodeIsType(child, nsEditProperty::head))
    {
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
                                                  PRInt32 *outStartOffset,
                                                  PRInt32 *outEndOffset,
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
    rv = ParseFragment(aContextStr, nsnull, doc, address_of(contextAsNode),
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
    contextAtom = contextLeafAsContent->Tag();
    if (contextAtom == nsGkAtoms::html) {
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
    PRInt32 err, sep, num;
    sep = aInfoStr.FindChar((PRUnichar)',');
    numstr1 = Substring(aInfoStr, 0, sep);
    numstr2 = Substring(aInfoStr, sep+1, aInfoStr.Length() - (sep+1));

    
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

  GetLengthOfDOMNode(*outEndNode, (PRUint32&)*outEndOffset);
  return NS_OK;
}


nsresult nsHTMLEditor::ParseFragment(const nsAString & aFragStr,
                                     nsIAtom* aContextLocalName,
                                     nsIDocument* aTargetDocument,
                                     nsCOMPtr<nsIDOMNode> *outNode,
                                     bool aTrustedInput)
{
  nsCOMPtr<nsIDOMDocumentFragment> frag;
  NS_NewDocumentFragment(getter_AddRefs(frag),
                         aTargetDocument->NodeInfoManager());
  nsCOMPtr<nsIContent> fragment = do_QueryInterface(frag);
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
  *outNode = do_QueryInterface(frag);
  return rv;
}

nsresult nsHTMLEditor::CreateListOfNodesToPaste(nsIDOMNode  *aFragmentAsNode,
                                                nsCOMArray<nsIDOMNode>& outNodeList,
                                                nsIDOMNode *aStartNode,
                                                PRInt32 aStartOffset,
                                                nsIDOMNode *aEndNode,
                                                PRInt32 aEndOffset)
{
  NS_ENSURE_TRUE(aFragmentAsNode, NS_ERROR_NULL_POINTER);

  nsresult rv;

  
  
  if (!aStartNode)
  {
    PRInt32 fragLen;
    rv = GetLengthOfDOMNode(aFragmentAsNode, (PRUint32&)fragLen);
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
  PRInt32 listCount = aListOfNodes.Count();
  NS_ENSURE_TRUE(listCount > 0, NS_ERROR_FAILURE);  

  
  
  PRInt32 idx = 0;
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
                                            PRInt32 *outHighWaterMark)
{
  NS_ENSURE_TRUE(outHighWaterMark, NS_ERROR_NULL_POINTER);
  
  *outHighWaterMark = -1;
  PRInt32 listAndTableParents = aListsAndTables.Count();
  
  
  PRInt32 listCount = aPasteNodes.Count();
  PRInt32 j;  
  for (j=0; j<listCount; j++)
  {
    nsCOMPtr<nsIDOMNode> curNode = aPasteNodes[j];

    NS_ENSURE_TRUE(curNode, NS_ERROR_FAILURE);
    if (nsHTMLEditUtils::IsTableElement(curNode) && !nsHTMLEditUtils::IsTable(curNode))
    {
      nsCOMPtr<nsIDOMNode> theTable = GetTableParent(curNode);
      if (theTable)
      {
        PRInt32 indexT = aListsAndTables.IndexOf(theTable);
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
        PRInt32 indexL = aListsAndTables.IndexOf(theList);
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
  
  
  PRInt32 listCount = aNodes.Count(), idx = 0;
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
                                       PRInt32 aHighWaterMark)
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
  PRInt32 listCount = aNodeArray.Count();
  if (listCount <= 0) {
    return nsnull;
  }

  if (aEnd) {
    return aNodeArray[listCount-1];
  }

  return aNodeArray[0];
}
