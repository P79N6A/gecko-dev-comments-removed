






































#include "nsHTMLEditor.h"
#include "nsHTMLEditRules.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsWSRunObject.h"

#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDocument.h"
#include "nsIDOMEventTarget.h" 
#include "nsIDOMNSEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMKeyListener.h" 
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMComment.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsISelectionController.h"
#include "nsIFileChannel.h"

#include "nsIDocumentObserver.h"
#include "nsIDocumentStateListener.h"

#include "nsIEnumerator.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsCOMArray.h"
#include "nsIFile.h"
#include "nsIURL.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIDocumentEncoder.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsLinebreakConverter.h"
#include "nsIFragmentContentSink.h"
#include "nsIContentSink.h"


#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIMIMEService.h"


#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIDragService.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsDirectoryServiceDefs.h"


#include "nsUnicharUtils.h"
#include "nsIDOMDocumentTraversal.h"
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


#include "TextEditorTest.h"
#include "nsEditorUtils.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIContentFilter.h"
#include "nsEventDispatcher.h"
#include "plbase64.h"
#include "prmem.h"
#include "nsStreamUtils.h"
#include "nsIPrincipal.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"

const PRUnichar nbsp = 160;

static NS_DEFINE_CID(kCParserCID,     NS_PARSER_CID);


#define kHTMLContext   "text/_moz_htmlcontext"
#define kHTMLInfo      "text/_moz_htmlinfo"
#define kInsertCookie  "_moz_Insert Here_moz_"

#define NS_FOUND_TARGET NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_EDITOR, 3)


static PRInt32 FindPositiveIntegerAfterString(const char *aLeadingString, nsCString &aCStr);
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
    if (nsHTMLEditUtils::IsList(parent)) return parent;
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
    if (nsHTMLEditUtils::IsTable(parent)) return parent;
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
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  
  nsTextRulesInfo ruleInfo(nsTextEditRules::kLoadHTML);
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (cancel) return NS_OK; 
  if (!handled)
  {
    PRBool isCollapsed;
    res = selection->GetIsCollapsed(&isCollapsed);
    NS_ENSURE_SUCCESS(res, res);

    
    if (!isCollapsed) 
    {
      res = DeleteSelection(eNone);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    nsCOMPtr<nsIDOMRange> range;
    res = selection->GetRangeAt(0, getter_AddRefs(range));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(range, NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIDOMNSRange> nsrange (do_QueryInterface(range));
    NS_ENSURE_TRUE(nsrange, NS_ERROR_NO_INTERFACE);

    
    nsCOMPtr<nsIDOMDocumentFragment> docfrag;
    {
      res = nsrange->CreateContextualFragment(aInputString, getter_AddRefs(docfrag));
      NS_ENSURE_SUCCESS(res, res);
    }
    
    nsCOMPtr<nsIDOMNode> parent, junk;
    res = range->GetStartContainer(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);
    PRInt32 childOffset;
    res = range->GetStartOffset(&childOffset);
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMNode> nodeToInsert;
    docfrag->GetFirstChild(getter_AddRefs(nodeToInsert));
    while (nodeToInsert)
    {
      res = InsertNode(nodeToInsert, parent, childOffset++);
      NS_ENSURE_SUCCESS(res, res);
      docfrag->GetFirstChild(getter_AddRefs(nodeToInsert));
    }
  }

  return mRules->DidDoAction(selection, &ruleInfo, res);
}


NS_IMETHODIMP nsHTMLEditor::InsertHTML(const nsAString & aInString)
{
  const nsAFlatString& empty = EmptyString();

  return InsertHTMLWithContext(aInString, empty, empty, empty,
                               nsnull,  nsnull, 0, PR_TRUE);
}


nsresult
nsHTMLEditor::InsertHTMLWithContext(const nsAString & aInputString,
                                    const nsAString & aContextStr,
                                    const nsAString & aInfoStr,
                                    const nsAString & aFlavor,
                                    nsIDOMDocument *aSourceDoc,
                                    nsIDOMNode *aDestNode,
                                    PRInt32 aDestOffset,
                                    PRBool aDeleteSelection)
{
  return DoInsertHTMLWithContext(aInputString, aContextStr, aInfoStr,
      aFlavor, aSourceDoc, aDestNode, aDestOffset, aDeleteSelection,
      PR_TRUE);
}

nsresult
nsHTMLEditor::DoInsertHTMLWithContext(const nsAString & aInputString,
                                      const nsAString & aContextStr,
                                      const nsAString & aInfoStr,
                                      const nsAString & aFlavor,
                                      nsIDOMDocument *aSourceDoc,
                                      nsIDOMNode *aDestNode,
                                      PRInt32 aDestOffset,
                                      PRBool aDeleteSelection,
                                      PRBool aTrustedInput)
{
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);

  
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpHTMLPaste, nsIEditor::eNext);
  
  
  nsresult res;
  nsCOMPtr<nsISelection>selection;
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  
  
  nsCOMPtr<nsIDOMNode> fragmentAsNode, streamStartParent, streamEndParent;
  PRInt32 streamStartOffset = 0, streamEndOffset = 0;

  res = CreateDOMFragmentFromPaste(aInputString, aContextStr, aInfoStr, 
                                   address_of(fragmentAsNode),
                                   address_of(streamStartParent),
                                   address_of(streamEndParent),
                                   &streamStartOffset,
                                   &streamEndOffset,
                                   aTrustedInput);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> targetNode, tempNode;
  PRInt32 targetOffset=0;

  if (!aDestNode)
  {
    
    
    res = GetStartNodeAndOffset(selection, getter_AddRefs(targetNode), &targetOffset);
    if (!targetNode) res = NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(res, res);
  }
  else
  {
    targetNode = aDestNode;
    targetOffset = aDestOffset;
  }

  PRBool doContinue = PR_TRUE;

  res = DoContentFilterCallback(aFlavor, aSourceDoc, aDeleteSelection,
                                (nsIDOMNode **)address_of(fragmentAsNode), 
                                (nsIDOMNode **)address_of(streamStartParent), 
                                &streamStartOffset,
                                (nsIDOMNode **)address_of(streamEndParent),
                                &streamEndOffset, 
                                (nsIDOMNode **)address_of(targetNode), 
                                &targetOffset, &doContinue);

  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(doContinue, NS_OK);

  
  
  
  
  
  
  if (aDestNode)
  {
    if (aDeleteSelection)
    {
      
      
      nsAutoTrackDOMPoint tracker(mRangeUpdater, &targetNode, &targetOffset);
      res = DeleteSelection(eNone);
      NS_ENSURE_SUCCESS(res, res);
    }

    res = selection->Collapse(targetNode, targetOffset);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  

  
  nsCOMArray<nsIDOMNode> nodeList;
  res = CreateListOfNodesToPaste(fragmentAsNode, nodeList,
                                 streamStartParent, streamStartOffset,
                                 streamEndParent, streamEndOffset);
  NS_ENSURE_SUCCESS(res, res);

  if (nodeList.Count() == 0)
    return NS_OK;

  
  res = RelativizeURIInFragmentList(nodeList, aFlavor, aSourceDoc, targetNode);
  
 
  
  
  nsCOMPtr<nsIDOMNode> parentNode;
  PRInt32 offsetOfNewNode;
  
  
  PRBool cellSelectionMode = PR_FALSE;
  nsCOMPtr<nsIDOMElement> cell;
  res = GetFirstSelectedCell(nsnull, getter_AddRefs(cell));
  if (NS_SUCCEEDED(res) && cell)
  {
    cellSelectionMode = PR_TRUE;
  }
  
  if (cellSelectionMode)
  {
    
    
    
    
    
    nsIDOMNode* firstNode = nodeList[0];
    if (!nsHTMLEditUtils::IsTableElement(firstNode))
      cellSelectionMode = PR_FALSE;
  }

  if (!cellSelectionMode)
  {
    res = DeleteSelectionAndPrepareToCreateNode(parentNode, offsetOfNewNode);
    NS_ENSURE_SUCCESS(res, res);

    
    res = RemoveAllInlineProperties();
    NS_ENSURE_SUCCESS(res, res);
  }
  else
  {
    
    if (1)
    {
      
      nsAutoSelectionReset selectionResetter(selection, this);
      res = DeleteTableCell(1);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    selection->CollapseToStart();
  }
  
  
  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (cancel) return NS_OK; 
  if (!handled)
  {
    
    
    res = GetStartNodeAndOffset(selection, getter_AddRefs(parentNode), &offsetOfNewNode);
    if (!parentNode) res = NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(res, res);

    
    NormalizeEOLInsertPosition(nodeList[0], address_of(parentNode), &offsetOfNewNode);

    
    
    
    nsWSRunObject wsObj(this, parentNode, offsetOfNewNode);
    if (nsTextEditUtils::IsBreak(wsObj.mEndReasonNode) && 
        !IsVisBreak(wsObj.mEndReasonNode) )
    {
      res = DeleteNode(wsObj.mEndReasonNode);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    
    PRBool bStartedInLink = IsInLink(parentNode);
  
    
    if (IsTextNode(parentNode))
    {
      nsCOMPtr<nsIDOMNode> temp;
      res = SplitNodeDeep(parentNode, parentNode, offsetOfNewNode, &offsetOfNewNode);
      NS_ENSURE_SUCCESS(res, res);
      res = parentNode->GetParentNode(getter_AddRefs(temp));
      NS_ENSURE_SUCCESS(res, res);
      parentNode = temp;
    }

    
    
    nsCOMArray<nsIDOMNode> startListAndTableArray;
    res = GetListAndTableParents(PR_FALSE, nodeList, startListAndTableArray);
    NS_ENSURE_SUCCESS(res, res);
    
    
    PRInt32 highWaterMark = -1;
    if (startListAndTableArray.Count() > 0)
    {
      res = DiscoverPartialListsAndTables(nodeList, startListAndTableArray, &highWaterMark);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    
    
    if (highWaterMark >= 0)
    {
      res = ReplaceOrphanedStructure(PR_FALSE, nodeList, startListAndTableArray, highWaterMark);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    
    nsCOMArray<nsIDOMNode> endListAndTableArray;
    res = GetListAndTableParents(PR_TRUE, nodeList, endListAndTableArray);
    NS_ENSURE_SUCCESS(res, res);
    highWaterMark = -1;
   
    
    if (endListAndTableArray.Count() > 0)
    {
      res = DiscoverPartialListsAndTables(nodeList, endListAndTableArray, &highWaterMark);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    
    if (highWaterMark >= 0)
    {
      res = ReplaceOrphanedStructure(PR_TRUE, nodeList, endListAndTableArray, highWaterMark);
      NS_ENSURE_SUCCESS(res, res);
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
      PRBool bDidInsert = PR_FALSE;
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
          res = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
          if (NS_FAILED(res))
            break;

          bDidInsert = PR_TRUE;
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
              PRBool isEmpty;
              res = IsEmptyNode(parentNode, &isEmpty, PR_TRUE);
              if ((NS_SUCCEEDED(res)) && isEmpty)
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
            res = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
            if (NS_FAILED(res))
              break;

            bDidInsert = PR_TRUE;
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
          res = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
          if (NS_FAILED(res))
            break;

          bDidInsert = PR_TRUE;
          lastInsertNode = child;
          offsetOfNewNode++;

          curNode->GetFirstChild(getter_AddRefs(child));
        }
      }

      if (!bDidInsert || NS_FAILED(res))
      {
        
        res = InsertNodeAtPoint(curNode, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
        if (NS_SUCCEEDED(res)) 
        {
          bDidInsert = PR_TRUE;
          lastInsertNode = curNode;
        }

        
        
        nsCOMPtr<nsIDOMNode> parent;
        while (NS_FAILED(res) && curNode)
        {
          curNode->GetParentNode(getter_AddRefs(parent));
          if (parent && !nsTextEditUtils::IsBody(parent))
          {
            res = InsertNodeAtPoint(parent, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
            if (NS_SUCCEEDED(res)) 
            {
              bDidInsert = PR_TRUE;
              insertedContextParent = parent;
              lastInsertNode = parent;
            }
          }
          curNode = parent;
        }
      }
      if (lastInsertNode)
      {
        res = GetNodeLocation(lastInsertNode, address_of(parentNode), &offsetOfNewNode);
        NS_ENSURE_SUCCESS(res, res);
        offsetOfNewNode++;
      }
    }

    
    if (lastInsertNode) 
    {
      
      nsCOMPtr<nsIDOMNode> selNode, tmp, visNode, highTable;
      PRInt32 selOffset;
      
      
      if (!nsHTMLEditUtils::IsTable(lastInsertNode))
      {
        res = GetLastEditableLeaf(lastInsertNode, address_of(selNode));
        NS_ENSURE_SUCCESS(res, res);
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
        res = GetLengthOfDOMNode(selNode, (PRUint32&)selOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
      else 
      {
        tmp = selNode;
        res = GetNodeLocation(tmp, address_of(selNode), &selOffset);
        ++selOffset;  
        NS_ENSURE_SUCCESS(res, res);
      }
      
      
      nsWSRunObject wsRunObj(this, selNode, selOffset);
      PRInt32 outVisOffset=0;
      PRInt16 visType=0;
      res = wsRunObj.PriorVisibleNode(selNode, selOffset, address_of(visNode), &outVisOffset, &visType);
      NS_ENSURE_SUCCESS(res, res);
      if (visType == nsWSRunObject::eBreak)
      {
        
        
        
        
        if (!IsVisBreak(wsRunObj.mStartReasonNode))
        {
          
          
          res = GetNodeLocation(wsRunObj.mStartReasonNode, address_of(selNode), &selOffset);
          
          nsWSRunObject wsRunObj(this, selNode, selOffset);
          res = wsRunObj.PriorVisibleNode(selNode, selOffset, address_of(visNode), &outVisOffset, &visType);
          NS_ENSURE_SUCCESS(res, res);
          if (visType == nsWSRunObject::eText ||
              visType == nsWSRunObject::eNormalWS)
          {
            selNode = visNode;
            selOffset = outVisOffset;  
          }
          else if (visType == nsWSRunObject::eSpecial)
          {
            
            
            res = GetNodeLocation(wsRunObj.mStartReasonNode, address_of(selNode), &selOffset);
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
        res = SplitNodeDeep(link, selNode, selOffset, &linkOffset, PR_TRUE, address_of(leftLink));
        NS_ENSURE_SUCCESS(res, res);
        res = GetNodeLocation(leftLink, address_of(selNode), &selOffset);
        NS_ENSURE_SUCCESS(res, res);
        selection->Collapse(selNode, selOffset+1);
      }
    }
  }
  
  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
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
nsHTMLEditor::RelativizeURIForNode(nsIDOMNode *aNode, nsIURL *aDestURL)
{
  nsAutoString attributeToModify;
  GetAttributeToModifyOnNode(aNode, attributeToModify);
  if (attributeToModify.IsEmpty())
    return NS_OK;

  nsCOMPtr<nsIDOMNamedNodeMap> attrMap;
  nsresult rv = aNode->GetAttributes(getter_AddRefs(attrMap));
  NS_ENSURE_SUCCESS(rv, NS_OK);
  NS_ENSURE_TRUE(attrMap, NS_OK); 

  nsCOMPtr<nsIDOMNode> attrNode;
  rv = attrMap->GetNamedItem(attributeToModify, getter_AddRefs(attrNode));
  NS_ENSURE_SUCCESS(rv, NS_OK); 

  if (attrNode)
  {
    nsAutoString oldValue;
    attrNode->GetNodeValue(oldValue);
    if (!oldValue.IsEmpty())
    {
      NS_ConvertUTF16toUTF8 oldCValue(oldValue);
      nsCOMPtr<nsIURI> currentNodeURI;
      rv = NS_NewURI(getter_AddRefs(currentNodeURI), oldCValue);
      if (NS_SUCCEEDED(rv))
      {
        nsCAutoString newRelativePath;
        aDestURL->GetRelativeSpec(currentNodeURI, newRelativePath);
        if (!newRelativePath.IsEmpty())
        {
          NS_ConvertUTF8toUTF16 newCValue(newRelativePath);
          attrNode->SetNodeValue(newCValue);
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsHTMLEditor::RelativizeURIInFragmentList(const nsCOMArray<nsIDOMNode> &aNodeList,
                                          const nsAString &aFlavor,
                                          nsIDOMDocument *aSourceDoc,
                                          nsIDOMNode *aTargetNode)
{
  
  nsCOMPtr<nsIDOMDocument> domDoc;
  GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> destDoc = do_QueryInterface(domDoc);
  NS_ENSURE_TRUE(destDoc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIURL> destURL = do_QueryInterface(destDoc->GetDocBaseURI());
  NS_ENSURE_TRUE(destURL, NS_ERROR_FAILURE);

  nsresult rv;
  nsCOMPtr<nsIDOMDocumentTraversal> trav = do_QueryInterface(domDoc, &rv);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  PRInt32 listCount = aNodeList.Count();
  PRInt32 j;
  for (j = 0; j < listCount; j++)
  {
    nsIDOMNode* somenode = aNodeList[j];

    nsCOMPtr<nsIDOMTreeWalker> walker;
    rv = trav->CreateTreeWalker(somenode, nsIDOMNodeFilter::SHOW_ELEMENT,
                                nsnull, PR_TRUE, getter_AddRefs(walker));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> currentNode;
    walker->GetCurrentNode(getter_AddRefs(currentNode));
    while (currentNode)
    {
      rv = RelativizeURIForNode(currentNode, destURL);
      NS_ENSURE_SUCCESS(rv, rv);

      walker->NextNode(getter_AddRefs(currentNode));
    }
  }

  return NS_OK;
}

nsresult
nsHTMLEditor::AddInsertionListener(nsIContentFilter *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  
  if (mContentFilters.IndexOfObject(aListener) == -1)
  {
    if (!mContentFilters.AppendObject(aListener))
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}
 
nsresult
nsHTMLEditor::RemoveInsertionListener(nsIContentFilter *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_FAILURE);

  if (!mContentFilters.RemoveObject(aListener))
    return NS_ERROR_FAILURE;

  return NS_OK;
}
 
nsresult
nsHTMLEditor::DoContentFilterCallback(const nsAString &aFlavor, 
                                      nsIDOMDocument *sourceDoc,
                                      PRBool aWillDeleteSelection,
                                      nsIDOMNode **aFragmentAsNode, 
                                      nsIDOMNode **aFragStartNode, 
                                      PRInt32 *aFragStartOffset,
                                      nsIDOMNode **aFragEndNode, 
                                      PRInt32 *aFragEndOffset,
                                      nsIDOMNode **aTargetNode,
                                      PRInt32 *aTargetOffset,
                                      PRBool *aDoContinue)
{
  *aDoContinue = PR_TRUE;

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

PRBool
nsHTMLEditor::IsInLink(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *outLink)
{
  NS_ENSURE_TRUE(aNode, PR_FALSE);
  if (outLink)
    *outLink = nsnull;
  nsCOMPtr<nsIDOMNode> tmp, node = aNode;
  while (node)
  {
    if (nsHTMLEditUtils::IsLink(node)) 
    {
      if (outLink)
        *outLink = node;
      return PR_TRUE;
    }
    tmp = node;
    tmp->GetParentNode(getter_AddRefs(node));
  }
  return PR_FALSE;
}


nsresult
nsHTMLEditor::StripFormattingNodes(nsIDOMNode *aNode, PRBool aListOnly)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsresult res = NS_OK;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (content->TextIsOnlyWhitespace())
  {
    nsCOMPtr<nsIDOMNode> parent, ignored;
    aNode->GetParentNode(getter_AddRefs(parent));
    if (parent)
    {
      if (!aListOnly || nsHTMLEditUtils::IsList(parent))
        res = parent->RemoveChild(aNode, getter_AddRefs(ignored));
      return res;
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
      res = StripFormattingNodes(child, aListOnly);
      NS_ENSURE_SUCCESS(res, res);
      child = tmp;
    }
  }
  return res;
}

NS_IMETHODIMP nsHTMLEditor::PrepareTransferable(nsITransferable **transferable)
{
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::PrepareHTMLTransferable(nsITransferable **aTransferable, 
                                                    PRBool aHavePrivFlavor)
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

      nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
      PRInt32 clipboardPasteOrder = 1; 

      if (prefs)
      {
        prefs->GetIntPref("clipboard.paste_image_type", &clipboardPasteOrder);
        switch (clipboardPasteOrder)
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
    }
    (*aTransferable)->AddDataFlavor(kUnicodeMime);
    (*aTransferable)->AddDataFlavor(kMozTextInternal);
  }
  
  return NS_OK;
}

PRInt32
FindPositiveIntegerAfterString(const char *aLeadingString, nsCString &aCStr)
{
  
  PRInt32 numFront = aCStr.Find(aLeadingString);
  if (numFront == -1)
    return -1;
  numFront += strlen(aLeadingString); 
  
  PRInt32 numBack = aCStr.FindCharInSet(CRLF, numFront);
  if (numBack == -1)
    return -1;
   
  nsCAutoString numStr(Substring(aCStr, numFront, numBack-numFront));
  PRInt32 errorCode;
  return numStr.ToInteger(&errorCode);
}

nsresult
RemoveFragComments(nsCString & aStr)
{
  
  PRInt32 startCommentIndx = aStr.Find("<!--StartFragment");
  if (startCommentIndx >= 0)
  {
    PRInt32 startCommentEnd = aStr.Find("-->", PR_FALSE, startCommentIndx);
    if (startCommentEnd > startCommentIndx)
      aStr.Cut(startCommentIndx, (startCommentEnd+3)-startCommentIndx);
  }  
  PRInt32 endCommentIndx = aStr.Find("<!--EndFragment");
  if (endCommentIndx >= 0)
  {
    PRInt32 endCommentEnd = aStr.Find("-->", PR_FALSE, endCommentIndx);
    if (endCommentEnd > endCommentIndx)
      aStr.Cut(endCommentIndx, (endCommentEnd+3)-endCommentIndx);
  }  
  return NS_OK;
}

nsresult
nsHTMLEditor::ParseCFHTML(nsCString & aCfhtml, PRUnichar **aStuffToPaste, PRUnichar **aCfcontext)
{
  
  PRInt32 startHTML     = FindPositiveIntegerAfterString("StartHTML:", aCfhtml);
  PRInt32 endHTML       = FindPositiveIntegerAfterString("EndHTML:", aCfhtml);
  PRInt32 startFragment = FindPositiveIntegerAfterString("StartFragment:", aCfhtml);
  PRInt32 endFragment   = FindPositiveIntegerAfterString("EndFragment:", aCfhtml);

  if ((startHTML<0) || (endHTML<0) || (startFragment<0) || (endFragment<0))
    return NS_ERROR_FAILURE;
 
  
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
  if (!*aStuffToPaste)
  {
    return NS_ERROR_FAILURE;
  }
  
  
  oldLengthInChars = cntxtUcs2Str.Length() + 1;  
  newLengthInChars = 0;
  *aCfcontext = nsLinebreakConverter::ConvertUnicharLineBreaks(cntxtUcs2Str.get(),
                                                           nsLinebreakConverter::eLinebreakAny, 
                                                           nsLinebreakConverter::eLinebreakContent, 
                                                           oldLengthInChars, &newLengthInChars);
  
  
  
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::InsertFromTransferable(nsITransferable *transferable, 
                                                   nsIDOMDocument *aSourceDoc,
                                                   const nsAString & aContextStr,
                                                   const nsAString & aInfoStr,
                                                   nsIDOMNode *aDestinationNode,
                                                   PRInt32 aDestOffset,
                                                   PRBool aDoDeleteSelection)
{
  nsresult rv = NS_OK;
  nsXPIDLCString bestFlavor;
  nsCOMPtr<nsISupports> genericDataObj;
  PRUint32 len = 0;
  if ( NS_SUCCEEDED(transferable->GetAnyTransferData(getter_Copies(bestFlavor), getter_AddRefs(genericDataObj), &len)) )
  {
    nsAutoTxnsConserveSelection dontSpazMySelection(this);
    nsAutoString flavor;
    flavor.AssignWithConversion(bestFlavor);
    nsAutoString stuffToPaste;
#ifdef DEBUG_clipboard
    printf("Got flavor [%s]\n", bestFlavor.get());
#endif

    
    PRBool isSafe = PR_FALSE;
    nsCOMPtr<nsIDOMDocument> destdomdoc;
    rv = GetDocument(getter_AddRefs(destdomdoc));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> destdoc = do_QueryInterface(destdomdoc);
    NS_ASSERTION(destdoc, "Where is our destination doc?");
    nsCOMPtr<nsISupports> container = destdoc->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> dsti(do_QueryInterface(container));
    nsCOMPtr<nsIDocShellTreeItem> root;
    if (dsti)
      dsti->GetRootTreeItem(getter_AddRefs(root));
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(root));
    PRUint32 appType;
    if (docShell && NS_SUCCEEDED(docShell->GetAppType(&appType)))
      isSafe = appType == nsIDocShell::APP_TYPE_EDITOR;
    if (!isSafe && aSourceDoc) {
      nsCOMPtr<nsIDocument> srcdoc = do_QueryInterface(aSourceDoc);
      NS_ASSERTION(srcdoc, "Where is our source doc?");

      nsIPrincipal* srcPrincipal = srcdoc->NodePrincipal();
      nsIPrincipal* destPrincipal = destdoc->NodePrincipal();
      NS_ASSERTION(srcPrincipal && destPrincipal, "How come we don't have a principal?");
      rv = srcPrincipal->Subsumes(destPrincipal, &isSafe);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    PRBool insertAsImage = PR_FALSE;
    nsCOMPtr<nsIURI> fileURI;
    if (0 == nsCRT::strcmp(bestFlavor, kFileMime))
    {
      nsCOMPtr<nsIFile> fileObj(do_QueryInterface(genericDataObj));
      if (fileObj && len > 0)
      {
        rv = NS_NewFileURI(getter_AddRefs(fileURI), fileObj);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIMIMEService> mime = do_GetService("@mozilla.org/mime;1");
        NS_ENSURE_TRUE(mime, NS_ERROR_FAILURE);
        nsCAutoString contentType;
        rv = mime->GetTypeFromFile(fileObj, contentType);
        NS_ENSURE_SUCCESS(rv, rv);

        
        if (StringBeginsWith(contentType, NS_LITERAL_CSTRING("image/"))) {
          insertAsImage = PR_TRUE;
          bestFlavor = contentType;
        }
      }
    }

    if (0 == nsCRT::strcmp(bestFlavor, kNativeHTMLMime))
    {
      
      nsCOMPtr<nsISupportsCString> textDataObj(do_QueryInterface(genericDataObj));
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
    else if (0 == nsCRT::strcmp(bestFlavor, kHTMLMime))
    {
      nsCOMPtr<nsISupportsString> textDataObj(do_QueryInterface(genericDataObj));
      if (textDataObj && len > 0)
      {
        nsAutoString text;
        textDataObj->GetData(text);
        NS_ASSERTION(text.Length() <= (len/2), "Invalid length!");
        stuffToPaste.Assign(text.get(), len / 2);
        nsAutoEditBatch beginBatching(this);
        rv = DoInsertHTMLWithContext(stuffToPaste,
                                     aContextStr, aInfoStr, flavor,
                                     aSourceDoc,
                                     aDestinationNode, aDestOffset,
                                     aDoDeleteSelection,
                                     isSafe);
      }
    }
    else if (0 == nsCRT::strcmp(bestFlavor, kUnicodeMime) ||
             0 == nsCRT::strcmp(bestFlavor, kMozTextInternal))
    {
      nsCOMPtr<nsISupportsString> textDataObj(do_QueryInterface(genericDataObj));
      if (textDataObj && len > 0)
      {
        nsAutoString text;
        textDataObj->GetData(text);
        NS_ASSERTION(text.Length() <= (len/2), "Invalid length!");
        stuffToPaste.Assign(text.get(), len / 2);
        nsAutoEditBatch beginBatching(this);
        
        rv = InsertTextAt(stuffToPaste, aDestinationNode, aDestOffset, aDoDeleteSelection);
      }
    }
    else if (0 == nsCRT::strcmp(bestFlavor, kJPEGImageMime) ||
             0 == nsCRT::strcmp(bestFlavor, kPNGImageMime) ||
             0 == nsCRT::strcmp(bestFlavor, kGIFImageMime) ||
             insertAsImage)
    {
      nsCOMPtr<nsIInputStream> imageStream;
      if (insertAsImage) {
        NS_ASSERTION(fileURI, "The file URI should be retrieved earlier");
        rv = NS_OpenURI(getter_AddRefs(imageStream), fileURI);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        imageStream = do_QueryInterface(genericDataObj);
        NS_ENSURE_TRUE(imageStream, NS_ERROR_FAILURE);
      }

      nsCString imageData;
      rv = NS_ConsumeStream(imageStream, PR_UINT32_MAX, imageData);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = imageStream->Close();
      NS_ENSURE_SUCCESS(rv, rv);

      char * base64 = PL_Base64Encode(imageData.get(), imageData.Length(), nsnull);
      NS_ENSURE_TRUE(base64, NS_ERROR_OUT_OF_MEMORY);

      stuffToPaste.AssignLiteral("<IMG src=\"data:");
      AppendUTF8toUTF16(bestFlavor, stuffToPaste);
      stuffToPaste.AppendLiteral(";base64,");
      AppendUTF8toUTF16(base64, stuffToPaste);
      stuffToPaste.AppendLiteral("\" alt=\"\" >");
      nsAutoEditBatch beginBatching(this);
      rv = DoInsertHTMLWithContext(stuffToPaste, EmptyString(), EmptyString(), 
                                   NS_LITERAL_STRING(kFileMime),
                                   aSourceDoc,
                                   aDestinationNode, aDestOffset,
                                   aDoDeleteSelection,
                                   isSafe);
      PR_Free(base64);
    }
  }
      
  

  
  
  if (NS_SUCCEEDED(rv))
    ScrollSelectionIntoView(PR_FALSE);

  return rv;
}

NS_IMETHODIMP nsHTMLEditor::InsertFromDrop(nsIDOMEvent* aDropEvent)
{
  ForceCompositionEnd();
  
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = 
           do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession)); 
  NS_ENSURE_TRUE(dragSession, NS_OK);

  
  nsCOMPtr<nsIDOMDocument> domdoc;
  GetDocument(getter_AddRefs(domdoc));

  
  
  PRBool bHavePrivateHTMLFlavor = PR_FALSE;
  rv = dragSession->IsDataFlavorSupported(kHTMLContext, &bHavePrivateHTMLFlavor);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareHTMLTransferable(getter_AddRefs(trans), bHavePrivateHTMLFlavor);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(trans, NS_OK);  

  PRUint32 numItems = 0; 
  rv = dragSession->GetNumDropItems(&numItems);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoEditBatch beginBatching(this);

  
  PRBool deleteSelection = PR_FALSE;
  nsCOMPtr<nsIDOMNode> newSelectionParent;
  PRInt32 newSelectionOffset = 0;

  
  nsCOMPtr<nsIDOMDocument> srcdomdoc;
  rv = dragSession->GetSourceDocument(getter_AddRefs(srcdomdoc));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 i; 
  PRBool doPlaceCaret = PR_TRUE;
  for (i = 0; i < numItems; ++i)
  {
    rv = dragSession->GetData(trans, i);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(trans, NS_OK); 

    
    nsAutoString contextStr, infoStr;
    nsCOMPtr<nsISupports> contextDataObj, infoDataObj;
    PRUint32 contextLen, infoLen;
    nsCOMPtr<nsISupportsString> textDataObj;
    
    nsCOMPtr<nsITransferable> contextTrans =
                      do_CreateInstance("@mozilla.org/widget/transferable;1");
    NS_ENSURE_TRUE(contextTrans, NS_ERROR_NULL_POINTER);
    contextTrans->AddDataFlavor(kHTMLContext);
    dragSession->GetData(contextTrans, i);
    contextTrans->GetTransferData(kHTMLContext, getter_AddRefs(contextDataObj), &contextLen);

    nsCOMPtr<nsITransferable> infoTrans =
                      do_CreateInstance("@mozilla.org/widget/transferable;1");
    NS_ENSURE_TRUE(infoTrans, NS_ERROR_NULL_POINTER);
    infoTrans->AddDataFlavor(kHTMLInfo);
    dragSession->GetData(infoTrans, i);
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

    if (doPlaceCaret)
    {
      
      
      PRBool userWantsCopy = PR_FALSE;

      nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent(do_QueryInterface(aDropEvent));
      NS_ENSURE_TRUE(nsuiEvent, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aDropEvent));
      if (mouseEvent)

#if defined(XP_MAC) || defined(XP_MACOSX)
        mouseEvent->GetAltKey(&userWantsCopy);
#else
        mouseEvent->GetCtrlKey(&userWantsCopy);
#endif

      
      nsCOMPtr<nsIDOMDocument>destdomdoc; 
      rv = GetDocument(getter_AddRefs(destdomdoc)); 
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsISelection> selection;
      rv = GetSelection(getter_AddRefs(selection));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

      PRBool isCollapsed;
      rv = selection->GetIsCollapsed(&isCollapsed);
      NS_ENSURE_SUCCESS(rv, rv);
      
      
      rv = nsuiEvent->GetRangeParent(getter_AddRefs(newSelectionParent));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(newSelectionParent, NS_ERROR_FAILURE);

      rv = nsuiEvent->GetRangeOffset(&newSelectionOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      

      nsCOMPtr<nsIDOMNode> userSelectNode = FindUserSelectAllNode(newSelectionParent);

      if (userSelectNode)
      {
        
        
        
        
        
        
        
        

        rv = GetNodeLocation(userSelectNode, address_of(newSelectionParent),
                             &newSelectionOffset);

        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(newSelectionParent, NS_ERROR_FAILURE);
      }

      
      PRBool cursorIsInSelection = PR_FALSE;

      
      if (!isCollapsed)
      {
        PRInt32 rangeCount;
        rv = selection->GetRangeCount(&rangeCount);
        NS_ENSURE_SUCCESS(rv, rv);

        for (PRInt32 j = 0; j < rangeCount; j++)
        {
          nsCOMPtr<nsIDOMRange> range;

          rv = selection->GetRangeAt(j, getter_AddRefs(range));
          if (NS_FAILED(rv) || !range) 
            continue;

          nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
          if (NS_FAILED(rv) || !nsrange) 
            continue;

          rv = nsrange->IsPointInRange(newSelectionParent, newSelectionOffset, &cursorIsInSelection);
          if(cursorIsInSelection)
            break;
        }
        if (cursorIsInSelection)
        {
          
          
          if (srcdomdoc == destdomdoc)
            return NS_OK;
          
          
          
          
          
        }
        else 
        {
          
          if (srcdomdoc == destdomdoc)
          {
            
            deleteSelection = !userWantsCopy;
          }
          else
          {
            
            deleteSelection = PR_FALSE;
          }
        }
      }

      
      doPlaceCaret = PR_FALSE;
    }
    
    
    if (!nsEditorHookUtils::DoInsertionHook(domdoc, aDropEvent, trans))
      return NS_OK;

    
    
    rv = InsertFromTransferable(trans, srcdomdoc, contextStr, infoStr,
                                newSelectionParent,
                                newSelectionOffset, deleteSelection);
  }

  return rv;
}

NS_IMETHODIMP nsHTMLEditor::CanDrag(nsIDOMEvent *aDragEvent, PRBool *aCanDrag)
{
  return nsPlaintextEditor::CanDrag(aDragEvent, aCanDrag);
}

nsresult
nsHTMLEditor::PutDragDataInTransferable(nsITransferable **aTransferable)
{
  NS_ENSURE_ARG_POINTER(aTransferable);
  *aTransferable = nsnull;
  nsCOMPtr<nsIDocumentEncoder> docEncoder;
  nsresult rv = SetupDocEncoder(getter_AddRefs(docEncoder));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

  
  nsAutoString buffer, parents, info;

  
  if (!IsPlaintextEditor())
  {
    
    rv = docEncoder->EncodeToStringWithContext(parents, info, buffer);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    
    rv = docEncoder->EncodeToString(buffer);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if ( buffer.IsEmpty() )
    return NS_OK;

  nsCOMPtr<nsISupportsString> dataWrapper, contextWrapper, infoWrapper;

  dataWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = dataWrapper->SetData(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransferable> trans = do_CreateInstance("@mozilla.org/widget/transferable;1");
  NS_ENSURE_TRUE(trans, NS_ERROR_FAILURE);

  if (IsPlaintextEditor())
  {
    
    rv = trans->AddDataFlavor(kUnicodeMime);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsISupports> genericDataObj(do_QueryInterface(dataWrapper));
    rv = trans->SetTransferData(kUnicodeMime, genericDataObj,
                                buffer.Length() * sizeof(PRUnichar));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    contextWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(contextWrapper, NS_ERROR_FAILURE);
    infoWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    NS_ENSURE_TRUE(infoWrapper, NS_ERROR_FAILURE);

    contextWrapper->SetData(parents);
    infoWrapper->SetData(info);

    rv = trans->AddDataFlavor(kHTMLMime);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFormatConverter> htmlConverter =
             do_CreateInstance("@mozilla.org/widget/htmlformatconverter;1");
    NS_ENSURE_TRUE(htmlConverter, NS_ERROR_FAILURE);

    rv = trans->SetConverter(htmlConverter);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> genericDataObj(do_QueryInterface(dataWrapper));
    rv = trans->SetTransferData(kHTMLMime, genericDataObj,
                                buffer.Length() * sizeof(PRUnichar));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!parents.IsEmpty())
    {
      
      trans->AddDataFlavor(kHTMLContext);
      genericDataObj = do_QueryInterface(contextWrapper);
      trans->SetTransferData(kHTMLContext, genericDataObj,
                             parents.Length() * sizeof(PRUnichar));
    }
    if (!info.IsEmpty())
    {
      
      trans->AddDataFlavor(kHTMLInfo);
      genericDataObj = do_QueryInterface(infoWrapper);
      trans->SetTransferData(kHTMLInfo, genericDataObj,
                             info.Length() * sizeof(PRUnichar));
    }
  }

  *aTransferable = trans; 
  NS_ADDREF(*aTransferable);
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::DoDrag(nsIDOMEvent *aDragEvent)
{
  return nsPlaintextEditor::DoDrag(aDragEvent);
}

PRBool nsHTMLEditor::HavePrivateHTMLFlavor(nsIClipboard *aClipboard)
{
  
  
  
  NS_ENSURE_TRUE(aClipboard, PR_FALSE);
  PRBool bHavePrivateHTMLFlavor = PR_FALSE;
  
  const char* flavArray[] = { kHTMLContext };
  
  if (NS_SUCCEEDED(aClipboard->HasDataMatchingFlavors(flavArray,
    NS_ARRAY_LENGTH(flavArray), nsIClipboard::kGlobalClipboard,
    &bHavePrivateHTMLFlavor )))
    return bHavePrivateHTMLFlavor;
    
  return PR_FALSE;
}


NS_IMETHODIMP nsHTMLEditor::Paste(PRInt32 aSelectionType)
{
  if (!FireClipboardEvent(NS_PASTE))
    return NS_OK;

  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  PRBool bHavePrivateHTMLFlavor = HavePrivateHTMLFlavor(clipboard);

  
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareHTMLTransferable(getter_AddRefs(trans), bHavePrivateHTMLFlavor);
  if (NS_SUCCEEDED(rv) && trans)
  {
    
    if (NS_SUCCEEDED(clipboard->GetData(trans, aSelectionType)) && IsModifiable())
    {
      
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

      
      
      rv = InsertFromTransferable(trans, nsnull, contextStr, infoStr,
                                  nsnull, 0, PR_TRUE);
    }
  }

  return rv;
}

NS_IMETHODIMP nsHTMLEditor::PasteTransferable(nsITransferable *aTransferable)
{
  if (!FireClipboardEvent(NS_PASTE))
    return NS_OK;

  
  nsCOMPtr<nsIDOMDocument> domdoc;
  GetDocument(getter_AddRefs(domdoc));
  if (!nsEditorHookUtils::DoInsertionHook(domdoc, nsnull, aTransferable))
    return NS_OK;

  
  
  nsAutoString contextStr, infoStr;
  return InsertFromTransferable(aTransferable, nsnull, contextStr, infoStr,
                                nsnull, 0, PR_TRUE);
}




NS_IMETHODIMP nsHTMLEditor::PasteNoFormatting(PRInt32 aSelectionType)
{
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
                                  PR_TRUE);
    }
  }

  return rv;
}





static const char* textEditorFlavors[] = { kUnicodeMime };
static const char* textHtmlEditorFlavors[] = { kUnicodeMime, kHTMLMime,
                                               kJPEGImageMime, kPNGImageMime,
                                               kGIFImageMime };

NS_IMETHODIMP nsHTMLEditor::CanPaste(PRInt32 aSelectionType, PRBool *aCanPaste)
{
  NS_ENSURE_ARG_POINTER(aCanPaste);
  *aCanPaste = PR_FALSE;

  
  if (!IsModifiable())
    return NS_OK;

  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool haveFlavors;

  
  if (IsPlaintextEditor())
    rv = clipboard->HasDataMatchingFlavors(textEditorFlavors,
                                           NS_ARRAY_LENGTH(textEditorFlavors),
                                           aSelectionType, &haveFlavors);
  else
    rv = clipboard->HasDataMatchingFlavors(textHtmlEditorFlavors,
                                           NS_ARRAY_LENGTH(textHtmlEditorFlavors),
                                           aSelectionType, &haveFlavors);
  
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aCanPaste = haveFlavors;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::CanPasteTransferable(nsITransferable *aTransferable, PRBool *aCanPaste)
{
  NS_ENSURE_ARG_POINTER(aCanPaste);

  
  if (!IsModifiable()) {
    *aCanPaste = PR_FALSE;
    return NS_OK;
  }

  
  if (!aTransferable) {
    *aCanPaste = PR_TRUE;
    return NS_OK;
  }

  

  
  const char ** flavors;
  unsigned length;
  if (IsPlaintextEditor()) {
    flavors = textEditorFlavors;
    length = NS_ARRAY_LENGTH(textEditorFlavors);
  } else {
    flavors = textHtmlEditorFlavors;
    length = NS_ARRAY_LENGTH(textHtmlEditorFlavors);
  }

  for (unsigned int i = 0; i < length; i++, flavors++) {
    nsCOMPtr<nsISupports> data;
    PRUint32 dataLen;
    nsresult rv = aTransferable->GetTransferData(*flavors,
                                                 getter_AddRefs(data),
                                                 &dataLen);
    if (NS_SUCCEEDED(rv) && data) {
      *aCanPaste = PR_TRUE;
      return NS_OK;
    }
  }
  
  *aCanPaste = PR_FALSE;
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
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (cancel) return NS_OK; 
  if (!handled)
  {
    nsCOMPtr<nsIDOMNode> newNode;
    res = DeleteSelectionAndCreateNode(NS_LITERAL_STRING("blockquote"), getter_AddRefs(newNode));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(newNode, NS_ERROR_NULL_POINTER);

    
    nsCOMPtr<nsIDOMElement> newElement (do_QueryInterface(newNode));
    if (newElement)
    {
      newElement->SetAttribute(NS_LITERAL_STRING("type"), NS_LITERAL_STRING("cite"));
    }

    
    res = selection->Collapse(newNode, 0);
    if (NS_FAILED(res))
    {
#ifdef DEBUG_akkana
      printf("Couldn't collapse");
#endif
      
    }

    res = Paste(aSelectionType);
  }
  return res;
}




NS_IMETHODIMP nsHTMLEditor::PasteAsPlaintextQuotation(PRInt32 aSelectionType)
{
  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransferable> trans =
                 do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  if (NS_SUCCEEDED(rv) && trans)
  {
    
    trans->AddDataFlavor(kUnicodeMime);

    
    clipboard->GetData(trans, aSelectionType);

    
    
    
    nsCOMPtr<nsISupports> genericDataObj;
    PRUint32 len = 0;
    char* flav = 0;
    rv = trans->GetAnyTransferData(&flav, getter_AddRefs(genericDataObj),
                                   &len);
    if (NS_FAILED(rv))
    {
#ifdef DEBUG_akkana
      printf("PasteAsPlaintextQuotation: GetAnyTransferData failed, %d\n", rv);
#endif
      return rv;
    }

    if (flav && 0 == nsCRT::strcmp((flav), kUnicodeMime))
    {
#ifdef DEBUG_clipboard
    printf("Got flavor [%s]\n", flav);
#endif
      nsCOMPtr<nsISupportsString> textDataObj(do_QueryInterface(genericDataObj));
      if (textDataObj && len > 0)
      {
        nsAutoString stuffToPaste;
        textDataObj->GetData(stuffToPaste);
        NS_ASSERTION(stuffToPaste.Length() <= (len/2), "Invalid length!");
        nsAutoEditBatch beginBatching(this);
        rv = InsertAsPlaintextQuotation(stuffToPaste, PR_TRUE, 0);
      }
    }
    NS_Free(flav);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLEditor::InsertTextWithQuotations(const nsAString &aStringToInsert)
{
  if (mWrapToWindow)
    return InsertText(aStringToInsert);

  
  BeginTransaction();

  
  
  
  

  static const PRUnichar cite('>');
  PRBool curHunkIsQuoted = (aStringToInsert.First() == cite);

  nsAString::const_iterator hunkStart, strEnd;
  aStringToInsert.BeginReading(hunkStart);
  aStringToInsert.EndReading(strEnd);

  
  
  
  
  
#ifdef DEBUG
  nsAString::const_iterator dbgStart (hunkStart);
  if (FindCharInReadable('\r', dbgStart, strEnd))
    NS_ASSERTION(PR_FALSE,
            "Return characters in DOM! InsertTextWithQuotations may be wrong");
#endif 

  
  nsresult rv = NS_OK;
  nsAString::const_iterator lineStart (hunkStart);
  while (1)   
  {
    
    PRBool found = FindCharInReadable('\n', lineStart, strEnd);
    PRBool quoted = PR_FALSE;
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
      rv = InsertAsPlaintextQuotation(curHunk, PR_FALSE,
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
    return InsertAsPlaintextQuotation(aQuotedText, PR_TRUE, aNodeInserted);

  nsAutoString citation;
  return InsertAsCitedQuotation(aQuotedText, citation, PR_FALSE,
                                aNodeInserted);
}





NS_IMETHODIMP
nsHTMLEditor::InsertAsPlaintextQuotation(const nsAString & aQuotedText,
                                         PRBool aAddCites,
                                         nsIDOMNode **aNodeInserted)
{
  if (mWrapToWindow)
    return nsPlaintextEditor::InsertAsQuotation(aQuotedText, aNodeInserted);

  nsresult rv;

  
  
  PRBool quotesInPre = PR_FALSE;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv) && prefBranch)
    prefBranch->GetBoolPref("editor.quotesPreformatted", &quotesInPre);

  nsCOMPtr<nsIDOMNode> preNode;
  
  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!selection)
  {
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  }
  else
  {
    nsAutoEditBatch beginBatching(this);
    nsAutoRules beginRulesSniffing(this, kOpInsertQuotation, nsIEditor::eNext);

    
    nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
    PRBool cancel, handled;
    rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
    NS_ENSURE_SUCCESS(rv, rv);
    if (cancel) return NS_OK; 
    if (!handled)
    {
      
      nsAutoString tag;
      if (quotesInPre)
        tag.AssignLiteral("pre");
      else
        tag.AssignLiteral("span");

      rv = DeleteSelectionAndCreateNode(tag, getter_AddRefs(preNode));
      
      
      
      
      
      if (NS_SUCCEEDED(rv) && preNode)
      {
        
        
        nsCOMPtr<nsIDOMElement> preElement (do_QueryInterface(preNode));
        if (preElement)
        {
          preElement->SetAttribute(NS_LITERAL_STRING("_moz_quote"),
                                   NS_LITERAL_STRING("true"));
          if (quotesInPre)
          {
            
            preElement->SetAttribute(NS_LITERAL_STRING("style"),
                                     NS_LITERAL_STRING("margin: 0 0 0 0px;"));
          }
          else
          {
            
            preElement->SetAttribute(NS_LITERAL_STRING("style"),
                                     NS_LITERAL_STRING("white-space: pre;"));
          }
        }

        
        selection->Collapse(preNode, 0);
      }

      if (aAddCites)
        rv = nsPlaintextEditor::InsertAsQuotation(aQuotedText, aNodeInserted);
      else
        rv = nsPlaintextEditor::InsertText(aQuotedText);
      
      
      

      if (aNodeInserted && NS_SUCCEEDED(rv))
      {
        *aNodeInserted = preNode;
        NS_IF_ADDREF(*aNodeInserted);
      }
    }
  }
    
  
  if (NS_SUCCEEDED(rv) && preNode)
  {
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    if (NS_SUCCEEDED(GetNodeLocation(preNode, address_of(parent), &offset)) && parent)
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
nsHTMLEditor::Rewrap(PRBool aRespectNewlines)
{
  return nsPlaintextEditor::Rewrap(aRespectNewlines);
}

NS_IMETHODIMP
nsHTMLEditor::InsertAsCitedQuotation(const nsAString & aQuotedText,
                                     const nsAString & aCitation,
                                     PRBool aInsertHTML,
                                     nsIDOMNode **aNodeInserted)
{
  
  if (IsPlaintextEditor())
  {
    NS_ASSERTION(!aInsertHTML, "InsertAsCitedQuotation: trying to insert html into plaintext editor");
    return InsertAsPlaintextQuotation(aQuotedText, PR_TRUE, aNodeInserted);
  }

  nsCOMPtr<nsIDOMNode> newNode;
  nsresult res = NS_OK;

  
  nsCOMPtr<nsISelection> selection;
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  if (!selection)
  {
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  }
  else
  {
    nsAutoEditBatch beginBatching(this);
    nsAutoRules beginRulesSniffing(this, kOpInsertQuotation, nsIEditor::eNext);

    
    nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
    PRBool cancel, handled;
    res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
    NS_ENSURE_SUCCESS(res, res);
    if (cancel) return NS_OK; 
    if (!handled)
    {
      res = DeleteSelectionAndCreateNode(NS_LITERAL_STRING("blockquote"), getter_AddRefs(newNode));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(newNode, NS_ERROR_NULL_POINTER);

      
      nsCOMPtr<nsIDOMElement> newElement (do_QueryInterface(newNode));
      if (newElement)
      {
        NS_NAMED_LITERAL_STRING(citestr, "cite");
        newElement->SetAttribute(NS_LITERAL_STRING("type"), citestr);

        if (!aCitation.IsEmpty())
          newElement->SetAttribute(citestr, aCitation);

        
        selection->Collapse(newNode, 0);
      }

      if (aInsertHTML)
        res = LoadHTML(aQuotedText);

      else
        res = InsertText(aQuotedText);  

      if (aNodeInserted)
      {
        if (NS_SUCCEEDED(res))
        {
          *aNodeInserted = newNode;
          NS_IF_ADDREF(*aNodeInserted);
        }
      }
    }
  }

  
  if (NS_SUCCEEDED(res) && newNode)
  {
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    if (NS_SUCCEEDED(GetNodeLocation(newNode, address_of(parent), &offset)) && parent)
      selection->Collapse(parent, offset+1);
  }
  return res;
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
                                                  PRBool aTrustedInput)
{
  NS_ENSURE_TRUE(outFragNode && outStartNode && outEndNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMDocumentFragment> docfrag;
  nsCOMPtr<nsIDOMNode> contextAsNode, tmp;  
  nsresult res = NS_OK;

  nsCOMPtr<nsIDOMDocument> domDoc;
  GetDocument(getter_AddRefs(domDoc));

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
  
  
  nsAutoTArray<nsString, 32> tagStack;
  nsCOMPtr<nsIDOMDocumentFragment> contextfrag;
  nsCOMPtr<nsIDOMNode> contextLeaf, junk;
  if (!aContextStr.IsEmpty())
  {
    res = ParseFragment(aContextStr, tagStack, doc, address_of(contextAsNode),
                        aTrustedInput);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(contextAsNode, NS_ERROR_FAILURE);

    res = StripFormattingNodes(contextAsNode);
    NS_ENSURE_SUCCESS(res, res);

    RemoveBodyAndHead(contextAsNode);

    res = FindTargetNode(contextAsNode, contextLeaf);
    if (res == NS_FOUND_TARGET)
      res = NS_OK;
    NS_ENSURE_SUCCESS(res, res);
  }

  
  res = CreateTagStack(tagStack, contextLeaf);
  NS_ENSURE_SUCCESS(res, res);

  
  res = ParseFragment(aInputString, tagStack, doc, outFragNode, aTrustedInput);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(*outFragNode, NS_ERROR_FAILURE);

  RemoveBodyAndHead(*outFragNode);

  if (contextAsNode)
  {
    
    contextLeaf->AppendChild(*outFragNode, getter_AddRefs(junk));
    *outFragNode = contextAsNode;
  }

  res = StripFormattingNodes(*outFragNode, PR_TRUE);
  NS_ENSURE_SUCCESS(res, res);

  
  
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
  return res;
}


nsresult nsHTMLEditor::ParseFragment(const nsAString & aFragStr,
                                     nsTArray<nsString> &aTagStack,
                                     nsIDocument* aTargetDocument,
                                     nsCOMPtr<nsIDOMNode> *outNode,
                                     PRBool aTrustedInput)
{
  
  PRBool bContext = aTagStack.IsEmpty();

  
  nsresult res;
  nsCOMPtr<nsIParser> parser = do_CreateInstance(kCParserCID, &res);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(parser, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIContentSink> sink;
  if (aTrustedInput) {
    if (bContext)
      sink = do_CreateInstance(NS_HTMLFRAGMENTSINK2_CONTRACTID);
    else
      sink = do_CreateInstance(NS_HTMLFRAGMENTSINK_CONTRACTID);
  } else {
    if (bContext)
      sink = do_CreateInstance(NS_HTMLPARANOIDFRAGMENTSINK2_CONTRACTID);
    else
      sink = do_CreateInstance(NS_HTMLPARANOIDFRAGMENTSINK_CONTRACTID);

    nsCOMPtr<nsIParanoidFragmentContentSink> paranoidSink(do_QueryInterface(sink));
    NS_ASSERTION(paranoidSink, "Our content sink is paranoid");
    if (bContext) {
      
      paranoidSink->AllowComments();
    } else {
      
      paranoidSink->AllowStyles();
    }
  }

  NS_ENSURE_TRUE(sink, NS_ERROR_FAILURE);
  nsCOMPtr<nsIFragmentContentSink> fragSink(do_QueryInterface(sink));
  NS_ENSURE_TRUE(fragSink, NS_ERROR_FAILURE);

  fragSink->SetTargetDocument(aTargetDocument);

  
  parser->SetContentSink(sink);
  if (bContext)
    parser->Parse(aFragStr, (void*)0, NS_LITERAL_CSTRING("text/html"), PR_TRUE, eDTDMode_fragment);
  else
    parser->ParseFragment(aFragStr, 0, aTagStack, PR_FALSE, NS_LITERAL_CSTRING("text/html"), eDTDMode_quirks);
  
  nsCOMPtr<nsIDOMDocumentFragment> contextfrag;
  res = fragSink->GetFragment(PR_TRUE, getter_AddRefs(contextfrag));
  NS_ENSURE_SUCCESS(res, res);
  *outNode = do_QueryInterface(contextfrag);
  
  return res;
}

nsresult nsHTMLEditor::CreateTagStack(nsTArray<nsString> &aTagStack, nsIDOMNode *aNode)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> node= aNode;
  PRBool bSeenBody = PR_FALSE;
  
  while (node) 
  {
    if (nsTextEditUtils::IsBody(node))
      bSeenBody = PR_TRUE;
    nsCOMPtr<nsIDOMNode> temp = node;
    PRUint16 nodeType;
    
    node->GetNodeType(&nodeType);
    if (nsIDOMNode::ELEMENT_NODE == nodeType)
    {
      nsString* tagName = aTagStack.AppendElement();
      NS_ENSURE_TRUE(tagName, NS_ERROR_OUT_OF_MEMORY);

      node->GetNodeName(*tagName);
      
    }

    res = temp->GetParentNode(getter_AddRefs(node));
    NS_ENSURE_SUCCESS(res, res);  
  }
  
  if (!bSeenBody)
  {
      aTagStack.AppendElement(NS_LITERAL_STRING("BODY"));
  }
  return res;
}


nsresult nsHTMLEditor::CreateListOfNodesToPaste(nsIDOMNode  *aFragmentAsNode,
                                                nsCOMArray<nsIDOMNode>& outNodeList,
                                                nsIDOMNode *aStartNode,
                                                PRInt32 aStartOffset,
                                                nsIDOMNode *aEndNode,
                                                PRInt32 aEndOffset)
{
  NS_ENSURE_TRUE(aFragmentAsNode, NS_ERROR_NULL_POINTER);

  nsresult res;

  
  
  if (!aStartNode)
  {
    PRInt32 fragLen;
    res = GetLengthOfDOMNode(aFragmentAsNode, (PRUint32&)fragLen);
    NS_ENSURE_SUCCESS(res, res);

    aStartNode = aFragmentAsNode;
    aStartOffset = 0;
    aEndNode = aFragmentAsNode;
    aEndOffset = fragLen;
  }

  nsCOMPtr<nsIDOMRange> docFragRange =
                          do_CreateInstance("@mozilla.org/content/range;1");
  NS_ENSURE_TRUE(docFragRange, NS_ERROR_OUT_OF_MEMORY);

  res = docFragRange->SetStart(aStartNode, aStartOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = docFragRange->SetEnd(aEndNode, aEndOffset);
  NS_ENSURE_SUCCESS(res, res);

  
  nsTrivialFunctor functor;
  nsDOMSubtreeIterator iter;
  res = iter.Init(docFragRange);
  NS_ENSURE_SUCCESS(res, res);
  res = iter.AppendList(functor, outNodeList);

  return res;
}

nsresult 
nsHTMLEditor::GetListAndTableParents(PRBool aEnd, 
                                     nsCOMArray<nsIDOMNode>& aListOfNodes,
                                     nsCOMArray<nsIDOMNode>& outArray)
{
  PRInt32 listCount = aListOfNodes.Count();
  if (listCount <= 0)
    return NS_ERROR_FAILURE;  
    
  
  
  PRInt32 idx = 0;
  if (aEnd) idx = listCount-1;
  
  nsCOMPtr<nsIDOMNode>  pNode = aListOfNodes[idx];
  while (pNode)
  {
    if (nsHTMLEditUtils::IsList(pNode) || nsHTMLEditUtils::IsTable(pNode))
    {
      if (!outArray.AppendObject(pNode))
      {
        return NS_ERROR_FAILURE;
      }
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
nsHTMLEditor::ScanForListAndTableStructure( PRBool aEnd,
                                            nsCOMArray<nsIDOMNode>& aNodes,
                                            nsIDOMNode *aListOrTable,
                                            nsCOMPtr<nsIDOMNode> *outReplaceNode)
{
  NS_ENSURE_TRUE(aListOrTable, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(outReplaceNode, NS_ERROR_NULL_POINTER);

  *outReplaceNode = 0;
  
  
  PRInt32 listCount = aNodes.Count(), idx = 0;
  if (aEnd) idx = listCount-1;
  PRBool bList = nsHTMLEditUtils::IsList(aListOrTable);
  
  nsCOMPtr<nsIDOMNode>  pNode = aNodes[idx];
  nsCOMPtr<nsIDOMNode>  originalNode = pNode;
  while (pNode)
  {
    if ( (bList && nsHTMLEditUtils::IsListItem(pNode)) ||
         (!bList && (nsHTMLEditUtils::IsTableElement(pNode) && !nsHTMLEditUtils::IsTable(pNode))) )
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
nsHTMLEditor::ReplaceOrphanedStructure(PRBool aEnd,
                                       nsCOMArray<nsIDOMNode>& aNodeArray,
                                       nsCOMArray<nsIDOMNode>& aListAndTableArray,
                                       PRInt32 aHighWaterMark)
{
  nsCOMPtr<nsIDOMNode> curNode = aListAndTableArray[aHighWaterMark];
  NS_ENSURE_TRUE(curNode, NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsIDOMNode> replaceNode, originalNode;
  
  
  nsresult res = ScanForListAndTableStructure(aEnd, aNodeArray, 
                                 curNode, address_of(replaceNode));
  NS_ENSURE_SUCCESS(res, res);
  
  
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

nsIDOMNode* nsHTMLEditor::GetArrayEndpoint(PRBool aEnd,
                                           nsCOMArray<nsIDOMNode>& aNodeArray)
{
  PRInt32 listCount = aNodeArray.Count();
  if (listCount <= 0) 
    return nsnull;

  if (aEnd)
  {
    return aNodeArray[listCount-1];
  }
  
  return aNodeArray[0];
}
