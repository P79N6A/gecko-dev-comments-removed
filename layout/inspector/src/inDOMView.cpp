





































#include "inDOMView.h"
#include "inIDOMUtils.h"

#include "inLayoutUtils.h"

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsISupportsArray.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMAttr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMMutationEvent.h"
#include "nsBindingManager.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIServiceManager.h"
#include "nsITreeColumns.h"

#ifdef ACCESSIBILITY
#include "nsIAccessible.h"
#include "nsIAccessibilityService.h"
#endif




class inDOMViewNode
{
public:
  inDOMViewNode() {}
  inDOMViewNode(nsIDOMNode* aNode);
  ~inDOMViewNode();

  nsCOMPtr<nsIDOMNode> node;

  inDOMViewNode* parent;
  inDOMViewNode* next;
  inDOMViewNode* previous;

  PRInt32 level;
  PRBool isOpen;
  PRBool isContainer;
  PRBool hasAnonymous;
  PRBool hasSubDocument;
};

inDOMViewNode::inDOMViewNode(nsIDOMNode* aNode) :
  node(aNode),
  parent(nsnull),
  next(nsnull),
  previous(nsnull),
  level(0),
  isOpen(PR_FALSE),
  isContainer(PR_FALSE),
  hasAnonymous(PR_FALSE),
  hasSubDocument(PR_FALSE)
{

}

inDOMViewNode::~inDOMViewNode()
{
}



nsIAtom* inDOMView::kAnonymousAtom = nsnull;
nsIAtom* inDOMView::kElementNodeAtom = nsnull;
nsIAtom* inDOMView::kAttributeNodeAtom = nsnull;
nsIAtom* inDOMView::kTextNodeAtom = nsnull;
nsIAtom* inDOMView::kCDataSectionNodeAtom = nsnull;
nsIAtom* inDOMView::kEntityReferenceNodeAtom = nsnull;
nsIAtom* inDOMView::kEntityNodeAtom = nsnull;
nsIAtom* inDOMView::kProcessingInstructionNodeAtom = nsnull;
nsIAtom* inDOMView::kCommentNodeAtom = nsnull;
nsIAtom* inDOMView::kDocumentNodeAtom = nsnull;
nsIAtom* inDOMView::kDocumentTypeNodeAtom = nsnull;
nsIAtom* inDOMView::kDocumentFragmentNodeAtom = nsnull;
nsIAtom* inDOMView::kNotationNodeAtom = nsnull;
nsIAtom* inDOMView::kAccessibleNodeAtom = nsnull;

inDOMView::inDOMView() :
  mShowAnonymous(PR_FALSE),
  mShowSubDocuments(PR_FALSE),
  mShowWhitespaceNodes(PR_TRUE),
  mShowAccessibleNodes(PR_FALSE),
  mWhatToShow(nsIDOMNodeFilter::SHOW_ALL)
{
}

inDOMView::~inDOMView()
{
  SetRootNode(nsnull);
}

 const nsStaticAtom inDOMView::Atoms_info[] = {
  {"anonymous", &inDOMView::kAnonymousAtom},
  {"ELEMENT_NODE", &inDOMView::kElementNodeAtom},
  {"ATTRIBUTE_NODE", &inDOMView::kAttributeNodeAtom},
  {"TEXT_NODE", &inDOMView::kTextNodeAtom},
  {"CDATA_SECTION_NODE", &inDOMView::kCDataSectionNodeAtom},
  {"ENTITY_REFERENCE_NODE", &inDOMView::kEntityReferenceNodeAtom},
  {"ENTITY_NODE", &inDOMView::kEntityNodeAtom},
  {"PROCESSING_INSTRUCTION_NODE", &inDOMView::kProcessingInstructionNodeAtom},
  {"COMMENT_NODE", &inDOMView::kCommentNodeAtom},
  {"DOCUMENT_NODE", &inDOMView::kDocumentNodeAtom},
  {"DOCUMENT_TYPE_NODE", &inDOMView::kDocumentTypeNodeAtom},
  {"DOCUMENT_FRAGMENT_NODE", &inDOMView::kDocumentFragmentNodeAtom},
  {"NOTATION_NODE", &inDOMView::kNotationNodeAtom},
  {"ACCESSIBLE_NODE", &inDOMView::kAccessibleNodeAtom}
};

 void
inDOMView::InitAtoms()
{
  NS_RegisterStaticAtoms(Atoms_info, NS_ARRAY_LENGTH(Atoms_info));
}




NS_IMPL_ISUPPORTS3(inDOMView,
                   inIDOMView,
                   nsITreeView,
                   nsIMutationObserver)




NS_IMETHODIMP
inDOMView::GetRootNode(nsIDOMNode** aNode)
{
  *aNode = mRootNode;
  NS_IF_ADDREF(*aNode);
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetRootNode(nsIDOMNode* aNode)
{
  if (mTree)
    mTree->BeginUpdateBatch();

  if (mRootDocument) {
    
    nsCOMPtr<nsINode> doc(do_QueryInterface(mRootDocument));
    if (doc)
      doc->RemoveMutationObserver(this);
  }

  RemoveAllNodes();

  mRootNode = aNode;

  if (aNode) {
    
    
    if (mWhatToShow & nsIDOMNodeFilter::SHOW_ELEMENT) {
      
      AppendNode(CreateNode(aNode, nsnull));
    } else {
      
      ExpandNode(-1);
    }

    
    
    mRootDocument = do_QueryInterface(aNode);
    if (!mRootDocument) {
      aNode->GetOwnerDocument(getter_AddRefs(mRootDocument));
    }

    
    nsCOMPtr<nsINode> doc(do_QueryInterface(mRootDocument));
    if (doc)
      doc->AddMutationObserver(this);
  } else {
    mRootDocument = nsnull;
  }

  if (mTree)
    mTree->EndUpdateBatch();

  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetNodeFromRowIndex(PRInt32 rowIndex, nsIDOMNode **_retval)
{
  inDOMViewNode* viewNode = nsnull;
  RowToNode(rowIndex, &viewNode);
  if (!viewNode) return NS_ERROR_FAILURE;
  *_retval = viewNode->node;
  NS_IF_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetRowIndexFromNode(nsIDOMNode *node, PRInt32 *_retval)
{
  NodeToRow(node, _retval);
  return NS_OK;
}


NS_IMETHODIMP
inDOMView::GetShowAnonymousContent(PRBool *aShowAnonymousContent)
{
  *aShowAnonymousContent = mShowAnonymous;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetShowAnonymousContent(PRBool aShowAnonymousContent)
{
  mShowAnonymous = aShowAnonymousContent;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetShowSubDocuments(PRBool *aShowSubDocuments)
{
  *aShowSubDocuments = mShowSubDocuments;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetShowSubDocuments(PRBool aShowSubDocuments)
{
  mShowSubDocuments = aShowSubDocuments;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetShowWhitespaceNodes(PRBool *aShowWhitespaceNodes)
{
  *aShowWhitespaceNodes = mShowWhitespaceNodes;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetShowWhitespaceNodes(PRBool aShowWhitespaceNodes)
{
  mShowWhitespaceNodes = aShowWhitespaceNodes;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetShowAccessibleNodes(PRBool *aShowAccessibleNodes)
{
  *aShowAccessibleNodes = mShowAccessibleNodes;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetShowAccessibleNodes(PRBool aShowAccessibleNodes)
{
  mShowAccessibleNodes = aShowAccessibleNodes;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetWhatToShow(PRUint32 *aWhatToShow)
{
  *aWhatToShow = mWhatToShow;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetWhatToShow(PRUint32 aWhatToShow)
{
  mWhatToShow = aWhatToShow;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::Rebuild()
{
  nsCOMPtr<nsIDOMNode> root;
  GetRootNode(getter_AddRefs(root));
  SetRootNode(root);
  return NS_OK;
}




NS_IMETHODIMP
inDOMView::GetRowCount(PRInt32 *aRowCount)
{
  *aRowCount = GetRowCount();
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetCellProperties(PRInt32 row, nsITreeColumn* col, nsISupportsArray *properties)
{
  inDOMViewNode* node = nsnull;
  RowToNode(row, &node);
  if (!node) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content = do_QueryInterface(node->node);
  if (content && content->IsInAnonymousSubtree()) {
    properties->AppendElement(kAnonymousAtom);
  }

  PRUint16 nodeType;
  node->node->GetNodeType(&nodeType);
  switch (nodeType) {
    case nsIDOMNode::ELEMENT_NODE:
      properties->AppendElement(kElementNodeAtom);
      break;
    case nsIDOMNode::ATTRIBUTE_NODE:
      properties->AppendElement(kAttributeNodeAtom);
      break;
    case nsIDOMNode::TEXT_NODE:
      properties->AppendElement(kTextNodeAtom);
      break;
    case nsIDOMNode::CDATA_SECTION_NODE:
      properties->AppendElement(kCDataSectionNodeAtom);
      break;
    case nsIDOMNode::ENTITY_REFERENCE_NODE:
      properties->AppendElement(kEntityReferenceNodeAtom);
      break;
    case nsIDOMNode::ENTITY_NODE:
      properties->AppendElement(kEntityNodeAtom);
      break;
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
      properties->AppendElement(kProcessingInstructionNodeAtom);
      break;
    case nsIDOMNode::COMMENT_NODE:
      properties->AppendElement(kCommentNodeAtom);
      break;
    case nsIDOMNode::DOCUMENT_NODE:
      properties->AppendElement(kDocumentNodeAtom);
      break;
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
      properties->AppendElement(kDocumentTypeNodeAtom);
      break;
    case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
      properties->AppendElement(kDocumentFragmentNodeAtom);
      break;
    case nsIDOMNode::NOTATION_NODE:
      properties->AppendElement(kNotationNodeAtom);
      break;
  }

#ifdef ACCESSIBILITY
  if (mShowAccessibleNodes) {
    nsCOMPtr<nsIAccessibilityService> accService(
      do_GetService("@mozilla.org/accessibilityService;1"));
    NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);

    nsCOMPtr<nsIAccessible> accessible;
    nsresult rv =
      accService->GetAttachedAccessibleFor(node->node,
                                           getter_AddRefs(accessible));
    if (NS_SUCCEEDED(rv) && accessible)
      properties->AppendElement(kAccessibleNodeAtom);
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetColumnProperties(nsITreeColumn* col, nsISupportsArray *properties)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetImageSrc(PRInt32 row, nsITreeColumn* col, nsAString& _retval)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetProgressMode(PRInt32 row, nsITreeColumn* col, PRInt32* _retval)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetCellValue(PRInt32 row, nsITreeColumn* col, nsAString& _retval)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetCellText(PRInt32 row, nsITreeColumn* col, nsAString& _retval)
{
  inDOMViewNode* node = nsnull;
  RowToNode(row, &node);
  if (!node) return NS_ERROR_FAILURE;

  nsIDOMNode* domNode = node->node;

  nsAutoString colID;
  col->GetId(colID);
  if (colID.EqualsLiteral("colNodeName"))
    domNode->GetNodeName(_retval);
  else if (colID.EqualsLiteral("colLocalName"))
    domNode->GetLocalName(_retval);
  else if (colID.EqualsLiteral("colPrefix"))
    domNode->GetPrefix(_retval);
  else if (colID.EqualsLiteral("colNamespaceURI"))
    domNode->GetNamespaceURI(_retval);
  else if (colID.EqualsLiteral("colNodeType")) {
    PRUint16 nodeType;
    domNode->GetNodeType(&nodeType);
    nsAutoString temp;
    temp.AppendInt(PRInt32(nodeType));
    _retval = temp;
  } else if (colID.EqualsLiteral("colNodeValue"))
    domNode->GetNodeValue(_retval);
  else {
    if (StringBeginsWith(colID, NS_LITERAL_STRING("col@"))) {
      nsCOMPtr<nsIDOMElement> el = do_QueryInterface(node->node);
      if (el) {
        nsAutoString attr;
        colID.Right(attr, colID.Length()-4); 
        el->GetAttribute(attr, _retval);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
inDOMView::IsContainer(PRInt32 index, PRBool *_retval)
{
  inDOMViewNode* node = nsnull;
  RowToNode(index, &node);
  if (!node) return NS_ERROR_FAILURE;

  *_retval = node->isContainer;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
  inDOMViewNode* node = nsnull;
  RowToNode(index, &node);
  if (!node) return NS_ERROR_FAILURE;

  *_retval = node->isOpen;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::IsContainerEmpty(PRInt32 index, PRBool *_retval)
{
  inDOMViewNode* node = nsnull;
  RowToNode(index, &node);
  if (!node) return NS_ERROR_FAILURE;

  *_retval = node->isContainer ? PR_FALSE : PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  inDOMViewNode* node = nsnull;
  RowToNode(index, &node);
  if (!node) return NS_ERROR_FAILURE;

  *_retval = node->level;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  inDOMViewNode* node = nsnull;
  RowToNode(rowIndex, &node);
  if (!node) return NS_ERROR_FAILURE;

  
  *_retval = -1;
  
  inDOMViewNode* checkNode = nsnull;
  PRInt32 i = rowIndex - 1;
  do {
    nsresult rv = RowToNode(i, &checkNode);
    if (NS_FAILED(rv)) {
      
      break;
    }
    
    if (checkNode == node->parent) {
      *_retval = i;
      return NS_OK;
    }
    --i;
  } while (checkNode);

  return NS_OK;
}

NS_IMETHODIMP
inDOMView::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, PRBool *_retval)
{
  inDOMViewNode* node = nsnull;
  RowToNode(rowIndex, &node);
  if (!node) return NS_ERROR_FAILURE;

  *_retval = node->next != nsnull;

  return NS_OK;
}

NS_IMETHODIMP
inDOMView::ToggleOpenState(PRInt32 index)
{
  inDOMViewNode* node = nsnull;
  RowToNode(index, &node);
  if (!node) return NS_ERROR_FAILURE;

  PRInt32 oldCount = GetRowCount();
  if (node->isOpen)
    CollapseNode(index);
  else
    ExpandNode(index);

  
  mTree->InvalidateRow(index);

  mTree->RowCountChanged(index+1, GetRowCount() - oldCount);

  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetTree(nsITreeBoxObject *tree)
{
  mTree = tree;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::GetSelection(nsITreeSelection * *aSelection)
{
  *aSelection = mSelection;
  NS_IF_ADDREF(*aSelection);
  return NS_OK;
}

NS_IMETHODIMP inDOMView::SetSelection(nsITreeSelection * aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SelectionChanged()
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetCellValue(PRInt32 row, nsITreeColumn* col, const nsAString& value)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::SetCellText(PRInt32 row, nsITreeColumn* col, const nsAString& value)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::CycleHeader(nsITreeColumn* col)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::CycleCell(PRInt32 row, nsITreeColumn* col)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::IsEditable(PRInt32 row, nsITreeColumn* col, PRBool *_retval)
{
  return NS_OK;
}


NS_IMETHODIMP
inDOMView::IsSelectable(PRInt32 row, nsITreeColumn* col, PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::IsSeparator(PRInt32 index, PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::IsSorted(PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::CanDrop(PRInt32 index, PRInt32 orientation,
                   nsIDOMDataTransfer* aDataTransfer, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::Drop(PRInt32 row, PRInt32 orientation, nsIDOMDataTransfer* aDataTransfer)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::PerformAction(const PRUnichar *action)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
  return NS_OK;
}

NS_IMETHODIMP
inDOMView::PerformActionOnCell(const PRUnichar* action, PRInt32 row, nsITreeColumn* col)
{
  return NS_OK;
}




void
inDOMView::NodeWillBeDestroyed(const nsINode* aNode)
{
  NS_NOTREACHED("Document destroyed while we're holding a strong ref to it");
}

void
inDOMView::AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                            PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                            PRInt32 aModType)
{
  if (!mTree) {
    return;
  }

  if (!(mWhatToShow & nsIDOMNodeFilter::SHOW_ATTRIBUTE)) {
    return;
  }

  
  nsCOMPtr<nsIDOMNode> content(do_QueryInterface(aContent));
  nsCOMPtr<nsIDOMElement> el(do_QueryInterface(aContent));
  nsCOMPtr<nsIDOMAttr> domAttr;
  nsAutoString attrStr;
  aAttribute->ToString(attrStr);
  if (aNameSpaceID) {
    nsCOMPtr<nsINameSpaceManager> nsm =
      do_GetService(NS_NAMESPACEMANAGER_CONTRACTID);
    if (!nsm) {
      
      return;
    }
    nsString attrNS;
    nsresult rv = nsm->GetNameSpaceURI(aNameSpaceID, attrNS);
    if (NS_FAILED(rv)) {
      return;
    }
    (void)el->GetAttributeNodeNS(attrNS, attrStr, getter_AddRefs(domAttr));
  } else {
    (void)el->GetAttributeNode(attrStr, getter_AddRefs(domAttr));
  }

  if (aModType == nsIDOMMutationEvent::MODIFICATION) {
    
    if (!domAttr) {
      return;
    }
    PRInt32 row = 0;
    NodeToRow(domAttr, &row);
    mTree->InvalidateRange(row, row);
  } else if (aModType == nsIDOMMutationEvent::ADDITION) {
    if (!domAttr) {
      return;
    }
    
    nsCOMPtr<nsIDOMNamedNodeMap> attrs;
    content->GetAttributes(getter_AddRefs(attrs));
    PRUint32 attrCount;
    attrs->GetLength(&attrCount);

    inDOMViewNode* contentNode = nsnull;
    PRInt32 contentRow;
    PRInt32 attrRow;
    if (mRootNode == content &&
        !(mWhatToShow & nsIDOMNodeFilter::SHOW_ELEMENT)) {
      
      
      attrRow = attrCount - 1;
    } else {
      if (NS_FAILED(NodeToRow(content, &contentRow))) {
        return;
      }
      RowToNode(contentRow, &contentNode);
      if (!contentNode->isOpen) {
        return;
      }
      attrRow = contentRow + attrCount;
    }

    inDOMViewNode* newNode = CreateNode(domAttr, contentNode);
    inDOMViewNode* insertNode = nsnull;
    RowToNode(attrRow, &insertNode);
    if (insertNode) {
      if (insertNode->level <= contentNode->level) {
        RowToNode(attrRow-1, &insertNode);
        InsertLinkAfter(newNode, insertNode);
      } else
        InsertLinkBefore(newNode, insertNode);
    }
    InsertNode(newNode, attrRow);
    mTree->RowCountChanged(attrRow, 1);
  } else if (aModType == nsIDOMMutationEvent::REMOVAL) {
    
    
    

    
    inDOMViewNode* contentNode = nsnull;
    PRInt32 contentRow;
    PRInt32 baseLevel;
    if (NS_SUCCEEDED(NodeToRow(content, &contentRow))) {
      RowToNode(contentRow, &contentNode);
      baseLevel = contentNode->level;
    } else {
      if (mRootNode == content) {
        contentRow = -1;
        baseLevel = -1;
      } else
        return;
    }

    
    inDOMViewNode* checkNode = nsnull;
    PRInt32 row = 0;
    for (row = contentRow+1; row < GetRowCount(); ++row) {
      checkNode = GetNodeAt(row);
      if (checkNode->level == baseLevel+1) {
        domAttr = do_QueryInterface(checkNode->node);
        if (domAttr) {
          nsAutoString attrName;
          domAttr->GetNodeName(attrName);
          if (attrName.Equals(attrStr)) {
            
            RemoveLink(checkNode);
            RemoveNode(row);
            mTree->RowCountChanged(row, -1);
            break;
          }
        }
      }
      if (checkNode->level <= baseLevel)
        break;
    }

 }
}

void
inDOMView::ContentAppended(nsIDocument *aDocument,
                           nsIContent* aContainer,
                           PRInt32 aNewIndexInContainer)
{
  if (!mTree) {
    return;
  }

  PRUint32 count = aContainer->GetChildCount();
  NS_ASSERTION((PRUint32)aNewIndexInContainer < count,
               "Bogus aNewIndexInContainer");

  while ((PRUint32)aNewIndexInContainer < count) {
    nsIContent *child = aContainer->GetChildAt(aNewIndexInContainer);

    ContentInserted(aDocument, aContainer, child, aNewIndexInContainer);
    ++aNewIndexInContainer;
  }
}

void
inDOMView::ContentInserted(nsIDocument *aDocument, nsIContent* aContainer,
                           nsIContent* aChild, PRInt32 aIndexInContainer)
{
  if (!mTree)
    return;

  nsresult rv;
  nsCOMPtr<nsIDOMNode> childDOMNode(do_QueryInterface(aChild));
  nsCOMPtr<nsIDOMNode> parent;
  if (!mDOMUtils) {
    mDOMUtils = do_GetService("@mozilla.org/inspector/dom-utils;1");
    if (!mDOMUtils) {
      return;
    }
  }
  mDOMUtils->GetParentForNode(childDOMNode, mShowAnonymous,
                              getter_AddRefs(parent));

  
  PRInt32 parentRow = 0;
  if (NS_FAILED(rv = NodeToRow(parent, &parentRow)))
    return;
  inDOMViewNode* parentNode = nsnull;
  if (NS_FAILED(rv = RowToNode(parentRow, &parentNode)))
    return;

  if (!parentNode->isOpen) {
    
    
    if (!parentNode->isContainer) {
      parentNode->isContainer = PR_TRUE;
      mTree->InvalidateRow(parentRow);
    }
    return;
  }

  
  nsCOMPtr<nsIDOMNode> previous;
  GetRealPreviousSibling(childDOMNode, parent, getter_AddRefs(previous));
  inDOMViewNode* previousNode = nsnull;

  PRInt32 row = 0;
  if (previous) {
    
    PRInt32 previousRow = 0;
    if (NS_FAILED(rv = NodeToRow(previous, &previousRow)))
      return;
    if (NS_FAILED(rv = RowToNode(previousRow, &previousNode)))
      return;

    
    
    GetLastDescendantOf(previousNode, previousRow, &row);
    ++row;
  } else {
    
    row = parentRow+1;
  }

  inDOMViewNode* newNode = CreateNode(childDOMNode, parentNode);

  if (previous) {
    InsertLinkAfter(newNode, previousNode);
  } else {
    PRInt32 firstChildRow;
    if (NS_SUCCEEDED(GetFirstDescendantOf(parentNode, parentRow, &firstChildRow))) {
      inDOMViewNode* firstChild;
      RowToNode(firstChildRow, &firstChild);
      InsertLinkBefore(newNode, firstChild);
    }
  }

  
  InsertNode(newNode, row);

  mTree->RowCountChanged(row, 1);
}

void
inDOMView::ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer, nsIContent* aChild, PRInt32 aIndexInContainer)
{
  if (!mTree)
    return;

  nsresult rv;

  
  nsCOMPtr<nsIDOMNode> oldDOMNode(do_QueryInterface(aChild));
  PRInt32 row = 0;
  if (NS_FAILED(rv = NodeToRow(oldDOMNode, &row)))
    return;
  inDOMViewNode* oldNode;
  if (NS_FAILED(rv = RowToNode(row, &oldNode)))
    return;

  
  
  inDOMViewNode* parentNode = oldNode->parent;
  
  
  
  PRInt32 oldCount = GetRowCount();
  
  if (oldNode->isOpen)
    CollapseNode(row);

  RemoveLink(oldNode);
  RemoveNode(row);

  nsINode* container = NODE_FROM(aContainer, aDocument);
  if (container->GetChildCount() == 0) {
    
    parentNode->isContainer = PR_FALSE;
    parentNode->isOpen = PR_FALSE;
    mTree->InvalidateRow(NodeToRow(parentNode));
  }
    
  mTree->RowCountChanged(row, GetRowCount() - oldCount);
}






inDOMViewNode*
inDOMView::GetNodeAt(PRInt32 aRow)
{
  return mNodes.ElementAt(aRow);
}

PRInt32
inDOMView::GetRowCount()
{
  return mNodes.Length();
}

PRInt32
inDOMView::NodeToRow(inDOMViewNode* aNode)
{
  return mNodes.IndexOf(aNode);
}

inDOMViewNode*
inDOMView::CreateNode(nsIDOMNode* aNode, inDOMViewNode* aParent)
{
  inDOMViewNode* viewNode = new inDOMViewNode(aNode);
  viewNode->level = aParent ? aParent->level+1 : 0;
  viewNode->parent = aParent;

  nsCOMArray<nsIDOMNode> grandKids;
  GetChildNodesFor(aNode, grandKids);
  viewNode->isContainer = (grandKids.Count() > 0);
  return viewNode;
}

PRBool
inDOMView::RowOutOfBounds(PRInt32 aRow, PRInt32 aCount)
{
  return aRow < 0 || aRow >= GetRowCount() || aCount+aRow > GetRowCount();
}

void
inDOMView::AppendNode(inDOMViewNode* aNode)
{
  mNodes.AppendElement(aNode);
}

void
inDOMView::InsertNode(inDOMViewNode* aNode, PRInt32 aRow)
{
  if (RowOutOfBounds(aRow, 1))
    AppendNode(aNode);
  else
    mNodes.InsertElementAt(aRow, aNode);
}

void
inDOMView::RemoveNode(PRInt32 aRow)
{
  if (RowOutOfBounds(aRow, 1))
    return;

  delete GetNodeAt(aRow);
  mNodes.RemoveElementAt(aRow);
}

void
inDOMView::ReplaceNode(inDOMViewNode* aNode, PRInt32 aRow)
{
  if (RowOutOfBounds(aRow, 1))
    return;

  delete GetNodeAt(aRow);
  mNodes.ElementAt(aRow) = aNode;
}

void
inDOMView::InsertNodes(nsTArray<inDOMViewNode*>& aNodes, PRInt32 aRow)
{
  if (aRow < 0 || aRow > GetRowCount())
    return;

  mNodes.InsertElementsAt(aRow, aNodes);
}

void
inDOMView::RemoveNodes(PRInt32 aRow, PRInt32 aCount)
{
  if (aRow < 0)
    return;

  PRInt32 rowCount = GetRowCount();
  for (PRInt32 i = aRow; i < aRow+aCount && i < rowCount; ++i) {
    delete GetNodeAt(i);
  }

  mNodes.RemoveElementsAt(aRow, aCount);
}

void
inDOMView::RemoveAllNodes()
{
  PRInt32 rowCount = GetRowCount();
  for (PRInt32 i = 0; i < rowCount; ++i) {
    delete GetNodeAt(i);
  }

  mNodes.Clear();
}

void
inDOMView::ExpandNode(PRInt32 aRow)
{
  inDOMViewNode* node = nsnull;
  RowToNode(aRow, &node);

  nsCOMArray<nsIDOMNode> kids;
  GetChildNodesFor(node ? node->node : mRootNode,
                   kids);
  PRInt32 kidCount = kids.Count();

  nsTArray<inDOMViewNode*> list(kidCount);

  inDOMViewNode* newNode = nsnull;
  inDOMViewNode* prevNode = nsnull;

  for (PRInt32 i = 0; i < kidCount; ++i) {
    newNode = CreateNode(kids[i], node);
    list.AppendElement(newNode);

    if (prevNode)
      prevNode->next = newNode;
    newNode->previous = prevNode;
    prevNode = newNode;
  }

  InsertNodes(list, aRow+1);

  if (node)
    node->isOpen = PR_TRUE;
}

void
inDOMView::CollapseNode(PRInt32 aRow)
{
  inDOMViewNode* node = nsnull;
  nsresult rv = RowToNode(aRow, &node);
  if (NS_FAILED(rv)) {
    return;
  }

  PRInt32 row = 0;
  GetLastDescendantOf(node, aRow, &row);

  RemoveNodes(aRow+1, row-aRow);

  node->isOpen = PR_FALSE;
}



nsresult
inDOMView::RowToNode(PRInt32 aRow, inDOMViewNode** aNode)
{
  if (aRow < 0 || aRow >= GetRowCount())
    return NS_ERROR_FAILURE;

  *aNode = GetNodeAt(aRow);
  return NS_OK;
}

nsresult
inDOMView::NodeToRow(nsIDOMNode* aNode, PRInt32* aRow)
{
  PRInt32 rowCount = GetRowCount();
  for (PRInt32 i = 0; i < rowCount; ++i) {
    if (GetNodeAt(i)->node == aNode) {
      *aRow = i;
      return NS_OK;
    }
  }

  *aRow = -1;
  return NS_ERROR_FAILURE;
}



void
inDOMView::InsertLinkAfter(inDOMViewNode* aNode, inDOMViewNode* aInsertAfter)
{
  if (aInsertAfter->next)
    aInsertAfter->next->previous = aNode;
  aNode->next = aInsertAfter->next;
  aInsertAfter->next = aNode;
  aNode->previous = aInsertAfter;
}

void
inDOMView::InsertLinkBefore(inDOMViewNode* aNode, inDOMViewNode* aInsertBefore)
{
  if (aInsertBefore->previous)
    aInsertBefore->previous->next = aNode;
  aNode->previous = aInsertBefore->previous;
  aInsertBefore->previous = aNode;
  aNode->next = aInsertBefore;
}

void
inDOMView::RemoveLink(inDOMViewNode* aNode)
{
  if (aNode->previous)
    aNode->previous->next = aNode->next;
  if (aNode->next)
    aNode->next->previous = aNode->previous;
}

void
inDOMView::ReplaceLink(inDOMViewNode* aNewNode, inDOMViewNode* aOldNode)
{
  if (aOldNode->previous)
    aOldNode->previous->next = aNewNode;
  if (aOldNode->next)
    aOldNode->next->previous = aNewNode;
  aNewNode->next = aOldNode->next;
  aNewNode->previous = aOldNode->previous;
}



nsresult
inDOMView::GetFirstDescendantOf(inDOMViewNode* aNode, PRInt32 aRow, PRInt32* aResult)
{
  
  PRInt32 row = 0;
  inDOMViewNode* node;
  for (row = aRow+1; row < GetRowCount(); ++row) {
    node = GetNodeAt(row);
    if (node->parent == aNode) {
      *aResult = row;
      return NS_OK;
    }
    if (node->level <= aNode->level)
      break;
  }
  return NS_ERROR_FAILURE;
}

nsresult
inDOMView::GetLastDescendantOf(inDOMViewNode* aNode, PRInt32 aRow, PRInt32* aResult)
{
  
  PRInt32 row = 0;
  for (row = aRow+1; row < GetRowCount(); ++row) {
    if (GetNodeAt(row)->level <= aNode->level)
      break;
  }
  *aResult = row-1;
  return NS_OK;
}



nsresult
inDOMView::GetChildNodesFor(nsIDOMNode* aNode, nsCOMArray<nsIDOMNode>& aResult)
{
  NS_ENSURE_ARG(aNode);
  
  
  nsCOMPtr<nsIDOMAttr> attr = do_QueryInterface(aNode);
  if (!attr) {
    
    if (mWhatToShow & nsIDOMNodeFilter::SHOW_ATTRIBUTE) {
      nsCOMPtr<nsIDOMNamedNodeMap> attrs;
      aNode->GetAttributes(getter_AddRefs(attrs));
      if (attrs) {
        AppendAttrsToArray(attrs, aResult);
      }
    }

    if (mWhatToShow & nsIDOMNodeFilter::SHOW_ELEMENT) {
      
      nsCOMPtr<nsIDOMNodeList> kids;
      if (mShowAnonymous) {
        nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
        if (content) {
          nsRefPtr<nsBindingManager> bindingManager = inLayoutUtils::GetBindingManagerFor(aNode);
          if (bindingManager) {
            bindingManager->GetAnonymousNodesFor(content, getter_AddRefs(kids));
            if (!kids) {
              bindingManager->GetContentListFor(content, getter_AddRefs(kids));
            }
          }
        }
      }

      if (!kids) {
        aNode->GetChildNodes(getter_AddRefs(kids));
      }
      if (kids) {
        AppendKidsToArray(kids, aResult);
      }
    }

    if (mShowSubDocuments) {
      nsCOMPtr<nsIDOMNode> domdoc =
        do_QueryInterface(inLayoutUtils::GetSubDocumentFor(aNode));
      if (domdoc) {
        aResult.AppendObject(domdoc);
      }
    }
  }

  return NS_OK;
}

nsresult
inDOMView::GetRealPreviousSibling(nsIDOMNode* aNode, nsIDOMNode* aRealParent, nsIDOMNode** aSibling)
{
  
  
  aNode->GetPreviousSibling(aSibling);
  return NS_OK;
}

nsresult
inDOMView::AppendKidsToArray(nsIDOMNodeList* aKids,
                             nsCOMArray<nsIDOMNode>& aArray)
{
  PRUint32 l = 0;
  aKids->GetLength(&l);
  nsCOMPtr<nsIDOMNode> kid;
  PRUint16 nodeType = 0;

  
  if (!mShowWhitespaceNodes && !mDOMUtils) {
    mDOMUtils = do_CreateInstance("@mozilla.org/inspector/dom-utils;1");
  }

  for (PRUint32 i = 0; i < l; ++i) {
    aKids->Item(i, getter_AddRefs(kid));
    kid->GetNodeType(&nodeType);

    NS_ASSERTION(nodeType && nodeType <= nsIDOMNode::NOTATION_NODE,
                 "Unknown node type. "
                 "Were new types added to the spec?");
    
    
    
    
    
    PRUint32 filterForNodeType = 1 << (nodeType - 1);

    if (mWhatToShow & filterForNodeType) {
      if ((nodeType == nsIDOMNode::TEXT_NODE ||
           nodeType == nsIDOMNode::COMMENT_NODE) &&
          !mShowWhitespaceNodes && mDOMUtils) {
        nsCOMPtr<nsIDOMCharacterData> data = do_QueryInterface(kid);
        NS_ASSERTION(data, "Does not implement nsIDOMCharacterData!");
        PRBool ignore;
        mDOMUtils->IsIgnorableWhitespace(data, &ignore);
        if (ignore) {
          continue;
        }
      }

      aArray.AppendObject(kid);
    }
  }

  return NS_OK;
}

nsresult
inDOMView::AppendAttrsToArray(nsIDOMNamedNodeMap* aKids,
                              nsCOMArray<nsIDOMNode>& aArray)
{
  PRUint32 l = 0;
  aKids->GetLength(&l);
  nsCOMPtr<nsIDOMNode> kid;
  for (PRUint32 i = 0; i < l; ++i) {
    aKids->Item(i, getter_AddRefs(kid));
    aArray.AppendObject(kid);
  }
  return NS_OK;
}
