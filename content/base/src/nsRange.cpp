








































#include "nscore.h"
#include "nsRange.h"

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMText.h"
#include "nsDOMError.h"
#include "nsIContentIterator.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"

nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult);
nsresult NS_NewContentSubtreeIterator(nsIContentIterator** aInstancePtrResult);







#define VALIDATE_ACCESS(node_)                                                     \
  PR_BEGIN_MACRO                                                                   \
    if (!node_) {                                                                  \
      return NS_ERROR_DOM_NOT_OBJECT_ERR;                                          \
    }                                                                              \
    if (!nsContentUtils::CanCallerAccess(node_)) {                                 \
      return NS_ERROR_DOM_SECURITY_ERR;                                            \
    }                                                                              \
    if (mIsDetached) {                                                             \
      return NS_ERROR_DOM_INVALID_STATE_ERR;                                       \
    }                                                                              \
  PR_END_MACRO









nsresult
nsRange::CompareNodeToRange(nsIContent* aNode, nsIDOMRange* aRange,
                            PRBool *outNodeBefore, PRBool *outNodeAfter)
{
  nsresult rv;
  nsCOMPtr<nsIRange> range = do_QueryInterface(aRange, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return CompareNodeToRange(aNode, range, outNodeBefore, outNodeAfter);
}


nsresult
nsRange::CompareNodeToRange(nsIContent* aNode, nsIRange* aRange,
                            PRBool *outNodeBefore, PRBool *outNodeAfter)
{
  
  
  
  
  
  
  
  nsresult rv;
  nsCOMPtr<nsIRange> range = do_QueryInterface(aRange, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!range->IsPositioned()) 
    return NS_ERROR_UNEXPECTED; 
  
  
  PRInt32 nodeStart, nodeEnd;
  nsINode* parent = aNode->GetNodeParent();
  if (!parent) {
    
    
    
    parent = aNode;
    nodeStart = 0;
    nodeEnd = aNode->GetChildCount();
    if (!nodeEnd) {
      return NS_ERROR_FAILURE;
    }
  }
  else {
    nodeStart = parent->IndexOf(aNode);
    nodeEnd = nodeStart + 1;
  }

  nsINode* rangeStartParent = range->GetStartParent();
  nsINode* rangeEndParent = range->GetEndParent();
  PRInt32 rangeStartOffset = range->StartOffset();
  PRInt32 rangeEndOffset = range->EndOffset();

  
  *outNodeBefore = nsContentUtils::ComparePoints(rangeStartParent,
                                                 rangeStartOffset,
                                                 parent, nodeStart) > 0;
  
  *outNodeAfter = nsContentUtils::ComparePoints(rangeEndParent,
                                                rangeEndOffset,
                                                parent, nodeEnd) < 0;

  return NS_OK;
}





nsresult
NS_NewRangeUtils(nsIRangeUtils** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  nsRangeUtils* rangeUtil = new nsRangeUtils();
  if (!rangeUtil) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(rangeUtil, aResult);
}




NS_IMPL_ISUPPORTS1(nsRangeUtils, nsIRangeUtils)




 
NS_IMETHODIMP_(PRInt32) 
nsRangeUtils::ComparePoints(nsIDOMNode* aParent1, PRInt32 aOffset1,
                            nsIDOMNode* aParent2, PRInt32 aOffset2)
{
  nsCOMPtr<nsINode> parent1 = do_QueryInterface(aParent1);
  nsCOMPtr<nsINode> parent2 = do_QueryInterface(aParent2);

  NS_ENSURE_TRUE(parent1 && parent2, -1);

  return nsContentUtils::ComparePoints(parent1, aOffset1, parent2, aOffset2);
}

NS_IMETHODIMP
nsRangeUtils::CompareNodeToRange(nsIContent* aNode, nsIDOMRange* aRange,
                                 PRBool *outNodeBefore, PRBool *outNodeAfter)
{
  return nsRange::CompareNodeToRange(aNode, aRange, outNodeBefore,
                                     outNodeAfter);
}





nsresult
NS_NewRange(nsIDOMRange** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  nsRange * range = new nsRange();
  if (!range) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(range, aResult);
}





nsRange::~nsRange() 
{
  DoSetRange(nsnull, 0, nsnull, 0, nsnull);
  
} 






NS_INTERFACE_MAP_BEGIN(nsRange)
  NS_INTERFACE_MAP_ENTRY(nsIDOMRange)
  NS_INTERFACE_MAP_ENTRY(nsIRange)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSRange)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIRange)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(Range)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsRange)
NS_IMPL_RELEASE(nsRange)





void
nsRange::CharacterDataChanged(nsIDocument* aDocument,
                              nsIContent* aContent,
                              CharacterDataChangeInfo* aInfo)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  
  
  if (aContent == mStartParent &&
      aInfo->mChangeStart < (PRUint32)mStartOffset) {
    
    
    mStartOffset = (PRUint32)mStartOffset < aInfo->mChangeEnd ?
       aInfo->mChangeStart :
       mStartOffset + aInfo->mChangeStart - aInfo->mChangeEnd +
         aInfo->mReplaceLength;
  }

  
  if (aContent == mEndParent && aInfo->mChangeStart < (PRUint32)mEndOffset) {
    mEndOffset = (PRUint32)mEndOffset < aInfo->mChangeEnd ?
       aInfo->mChangeStart :
       mEndOffset + aInfo->mChangeStart - aInfo->mChangeEnd +
         aInfo->mReplaceLength;
  }
}

void
nsRange::ContentInserted(nsIDocument* aDocument,
                         nsIContent* aContainer,
                         nsIContent* aChild,
                         PRInt32 aIndexInContainer)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  nsINode* container = NODE_FROM(aContainer, aDocument);

  
  if (container == mStartParent && aIndexInContainer < mStartOffset) {
    ++mStartOffset;
  }
  if (container == mEndParent && aIndexInContainer < mEndOffset) {
    ++mEndOffset;
  }
}

void
nsRange::ContentRemoved(nsIDocument* aDocument,
                        nsIContent* aContainer,
                        nsIContent* aChild,
                        PRInt32 aIndexInContainer)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  nsINode* container = NODE_FROM(aContainer, aDocument);

  
  if (container == mStartParent && aIndexInContainer < mStartOffset) {
    --mStartOffset;
  }
  
  else if (nsContentUtils::ContentIsDescendantOf(mStartParent, aChild)) {
    mStartParent = container;
    mStartOffset = aIndexInContainer;
  }

  
  if (container == mEndParent && aIndexInContainer < mEndOffset) {
    --mEndOffset;
  }
  else if (nsContentUtils::ContentIsDescendantOf(mEndParent, aChild)) {
    mEndParent = container;
    mEndOffset = aIndexInContainer;
  }
}

void
nsRange::NodeWillBeDestroyed(const nsINode* aNode)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  
  
  
  DoSetRange(nsnull, 0, nsnull, 0, nsnull);
}




NS_IMETHODIMP
nsRange::IsPointInRange(nsIDOMNode* aParent, PRInt32 aOffset, PRBool* aResult)
{
  PRInt16 compareResult = 0;
  nsresult rv = ComparePoint(aParent, aOffset, &compareResult);
  *aResult = compareResult == 0;

  return rv;
}
  


NS_IMETHODIMP
nsRange::ComparePoint(nsIDOMNode* aParent, PRInt32 aOffset, PRInt16* aResult)
{
  if (mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  
  if (!mIsPositioned) 
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  NS_ENSURE_TRUE(parent, NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);

  if (!nsContentUtils::ContentIsDescendantOf(parent, mRoot)) {
    return NS_ERROR_DOM_WRONG_DOCUMENT_ERR;
  }
  
  PRInt32 cmp;
  if ((cmp = nsContentUtils::ComparePoints(parent, aOffset,
                                           mStartParent, mStartOffset)) <= 0) {
    
    *aResult = cmp;
  }
  else if (nsContentUtils::ComparePoints(mEndParent, mEndOffset,
                                         parent, aOffset) == -1) {
    *aResult = 1;
  }
  else {
    *aResult = 0;
  }
  
  return NS_OK;
}
  





static PRInt32 GetNodeLength(nsINode *aNode)
{
  if(aNode->IsNodeOfType(nsINode::eDATA_NODE)) {
    return NS_STATIC_CAST(nsIContent*, aNode)->TextLength();
  }

  return aNode->GetChildCount();
}






void
nsRange::DoSetRange(nsINode* aStartN, PRInt32 aStartOffset,
                    nsINode* aEndN, PRInt32 aEndOffset,
                    nsINode* aRoot)
{
  NS_PRECONDITION((aStartN && aEndN && aRoot) ||
                  (!aStartN && !aEndN && !aRoot),
                  "Set all or none");
  NS_PRECONDITION(!aRoot ||
                  (nsContentUtils::ContentIsDescendantOf(aStartN, aRoot) &&
                   nsContentUtils::ContentIsDescendantOf(aEndN, aRoot) &&
                   aRoot == IsValidBoundary(aStartN) &&
                   aRoot == IsValidBoundary(aEndN)),
                  "Wrong root");
  NS_PRECONDITION(!aRoot ||
                  (aStartN->IsNodeOfType(nsINode::eCONTENT) &&
                   aEndN->IsNodeOfType(nsINode::eCONTENT) &&
                   aRoot ==
                    NS_STATIC_CAST(nsIContent*, aStartN)->GetBindingParent() &&
                   aRoot ==
                    NS_STATIC_CAST(nsIContent*, aEndN)->GetBindingParent()) ||
                  (!aRoot->GetNodeParent() &&
                   (aRoot->IsNodeOfType(nsINode::eDOCUMENT) ||
                    aRoot->IsNodeOfType(nsINode::eATTRIBUTE) ||
                    aRoot->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT))),
                  "Bad root");

  if (mRoot != aRoot) {
    if (mRoot) {
      mRoot->RemoveMutationObserver(this);
    }
    if (aRoot) {
      aRoot->AddMutationObserver(this);
    }
  }
 
  mStartParent = aStartN;
  mStartOffset = aStartOffset;
  mEndParent = aEndN;
  mEndOffset = aEndOffset;
  mIsPositioned = !!mStartParent;
  mRoot = aRoot;
}

static PRInt32
IndexOf(nsIDOMNode* aChildNode)
{
  

  nsCOMPtr<nsINode> child = do_QueryInterface(aChildNode);
  if (!child) {
    return -1;
  }

  nsINode *parent = child->GetNodeParent();

  
  return parent ? parent->IndexOf(child) : -1;
}





nsINode*
nsRange::GetCommonAncestor()
{
  return mIsPositioned ?
    nsContentUtils::GetCommonAncestor(mStartParent, mEndParent) :
    nsnull;
}

void
nsRange::Reset()
{
  DoSetRange(nsnull, 0, nsnull, 0, nsnull);
}





nsresult nsRange::GetStartContainer(nsIDOMNode** aStartParent)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  return CallQueryInterface(mStartParent, aStartParent);
}

nsresult nsRange::GetStartOffset(PRInt32* aStartOffset)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aStartOffset = mStartOffset;

  return NS_OK;
}

nsresult nsRange::GetEndContainer(nsIDOMNode** aEndParent)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  return CallQueryInterface(mEndParent, aEndParent);
}

nsresult nsRange::GetEndOffset(PRInt32* aEndOffset)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aEndOffset = mEndOffset;

  return NS_OK;
}

nsresult nsRange::GetCollapsed(PRBool* aIsCollapsed)
{
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aIsCollapsed = Collapsed();

  return NS_OK;
}

nsresult nsRange::GetCommonAncestorContainer(nsIDOMNode** aCommonParent)
{
  *aCommonParent = nsnull;
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  nsINode* container = nsContentUtils::GetCommonAncestor(mStartParent, mEndParent);
  if (container) {
    return CallQueryInterface(container, aCommonParent);
  }

  return NS_ERROR_NOT_INITIALIZED;
}

nsINode* nsRange::IsValidBoundary(nsINode* aNode)
{
  if (!aNode) {
    return nsnull;
  }

  if (aNode->IsNodeOfType(nsINode::eCONTENT)) {
    nsIContent* content = NS_STATIC_CAST(nsIContent*, aNode);
    if (content->Tag() == nsGkAtoms::documentTypeNodeName) {
      return nsnull;
    }

    if (!mMaySpanAnonymousSubtrees) {
      
      
      nsINode* root = content->GetBindingParent();
      if (root) {
        return root;
      }
    }
  }

  
  
  nsINode* root = aNode->GetCurrentDoc();
  if (root) {
    return root;
  }

  root = aNode;
  while ((aNode = aNode->GetNodeParent())) {
    root = aNode;
  }

  NS_ASSERTION(!root->IsNodeOfType(nsINode::eDOCUMENT),
               "GetCurrentDoc should have returned a doc");

  if (root->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT) ||
      root->IsNodeOfType(nsINode::eATTRIBUTE)) {
    return root;
  }

#ifdef DEBUG_smaug
  nsCOMPtr<nsIContent> cont = do_QueryInterface(root);
  if (cont) {
    nsAutoString name;
    cont->Tag()->ToString(name);
    printf("nsRange::IsValidBoundary: node is not a valid boundary point [%s]\n",
           NS_ConvertUTF16toUTF8(name).get());
  }
#endif

  return nsnull;
}

nsresult nsRange::SetStart(nsIDOMNode* aParent, PRInt32 aOffset)
{
  VALIDATE_ACCESS(aParent);

  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  nsINode* newRoot = IsValidBoundary(parent);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR);

  PRInt32 len = GetNodeLength(parent);
  if (aOffset < 0 || aOffset > len)
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  
  
  if (!mIsPositioned || newRoot != mRoot ||
      nsContentUtils::ComparePoints(parent, aOffset,
                                    mEndParent, mEndOffset) == 1) {
    DoSetRange(parent, aOffset, parent, aOffset, newRoot);

    return NS_OK;
  }

  DoSetRange(parent, aOffset, mEndParent, mEndOffset, mRoot);
  
  return NS_OK;
}

nsresult nsRange::SetStartBefore(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> parent;
  nsresult rv = aSibling->GetParentNode(getter_AddRefs(parent));
  if (NS_FAILED(rv) || !parent) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  return SetStart(parent, IndexOf(aSibling));
}

nsresult nsRange::SetStartAfter(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);

  nsCOMPtr<nsIDOMNode> nParent;
  nsresult res = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(res) || !nParent) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  return SetStart(nParent, IndexOf(aSibling) + 1);
}

nsresult nsRange::SetEnd(nsIDOMNode* aParent, PRInt32 aOffset)
{
  VALIDATE_ACCESS(aParent);

  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  nsINode* newRoot = IsValidBoundary(parent);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR);

  PRInt32 len = GetNodeLength(parent);
  if (aOffset < 0 || aOffset > len) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  
  if (!mIsPositioned || newRoot != mRoot ||
      nsContentUtils::ComparePoints(mStartParent, mStartOffset,
                                    parent, aOffset) == 1) {
    DoSetRange(parent, aOffset, parent, aOffset, newRoot);

    return NS_OK;
  }

  DoSetRange(mStartParent, mStartOffset, parent, aOffset, mRoot);

  return NS_OK;
}

nsresult nsRange::SetEndBefore(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> nParent;
  nsresult rv = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(rv) || !nParent) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  return SetEnd(nParent, IndexOf(aSibling));
}

nsresult nsRange::SetEndAfter(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> nParent;
  nsresult res = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(res) || !nParent) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  return SetEnd(nParent, IndexOf(aSibling) + 1);
}

nsresult nsRange::Collapse(PRBool aToStart)
{
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  if (aToStart)
    DoSetRange(mStartParent, mStartOffset, mStartParent, mStartOffset, mRoot);
  else
    DoSetRange(mEndParent, mEndOffset, mEndParent, mEndOffset, mRoot);

  return NS_OK;
}

nsresult nsRange::SelectNode(nsIDOMNode* aN)
{
  VALIDATE_ACCESS(aN);
  
  nsCOMPtr<nsINode> node = do_QueryInterface(aN);
  NS_ENSURE_TRUE(node, NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR);

  nsINode* parent = node->GetNodeParent();
  nsINode* newRoot = IsValidBoundary(parent);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR);

  PRInt32 index = parent->IndexOf(node);
  if (index < 0) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  DoSetRange(parent, index, parent, index + 1, newRoot);
  
  return NS_OK;
}

nsresult nsRange::SelectNodeContents(nsIDOMNode* aN)
{
  VALIDATE_ACCESS(aN);

  nsCOMPtr<nsINode> node = do_QueryInterface(aN);
  nsINode* newRoot = IsValidBoundary(node);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR);
  
  DoSetRange(node, 0, node, GetNodeLength(node), newRoot);
  
  return NS_OK;
}













class RangeSubtreeIterator
{
private:

  enum RangeSubtreeIterState { eDone=0,
                               eUseStartCData,
                               eUseIterator,
                               eUseEndCData };

  nsCOMPtr<nsIContentIterator>  mIter;
  RangeSubtreeIterState         mIterState;

  nsCOMPtr<nsIDOMCharacterData> mStartCData;
  nsCOMPtr<nsIDOMCharacterData> mEndCData;

public:

  RangeSubtreeIterator()
    : mIterState(eDone)
  {
  }
  ~RangeSubtreeIterator()
  {
  }

  nsresult Init(nsIDOMRange *aRange);
  already_AddRefed<nsIDOMNode> GetCurrentNode();
  void First();
  void Last();
  void Next();
  void Prev();

  PRBool IsDone()
  {
    return mIterState == eDone;
  }
};

nsresult
RangeSubtreeIterator::Init(nsIDOMRange *aRange)
{
  mIterState = eDone;

  nsCOMPtr<nsIDOMNode> node;

  
  
  

  nsresult res = aRange->GetStartContainer(getter_AddRefs(node));
  if (!node) return NS_ERROR_FAILURE;

  mStartCData = do_QueryInterface(node);

  
  
  

  res = aRange->GetEndContainer(getter_AddRefs(node));
  if (!node) return NS_ERROR_FAILURE;

  mEndCData = do_QueryInterface(node);

  if (mStartCData && mStartCData == mEndCData)
  {
    
    
    

    mEndCData = nsnull;
  }
  else
  {
    
    

    res = NS_NewContentSubtreeIterator(getter_AddRefs(mIter));
    if (NS_FAILED(res)) return res;

    res = mIter->Init(aRange);
    if (NS_FAILED(res)) return res;

    if (mIter->IsDone())
    {
      
      
      

      mIter = nsnull;
    }
  }

  
  

  First();

  return NS_OK;
}

already_AddRefed<nsIDOMNode>
RangeSubtreeIterator::GetCurrentNode()
{
  nsIDOMNode *node = nsnull;

  if (mIterState == eUseStartCData && mStartCData) {
    NS_ADDREF(node = mStartCData);
  } else if (mIterState == eUseEndCData && mEndCData)
    NS_ADDREF(node = mEndCData);
  else if (mIterState == eUseIterator && mIter)
  {
    nsIContent *content = mIter->GetCurrentNode();

    if (content) {
      CallQueryInterface(content, &node);
    }
  }

  return node;
}

void
RangeSubtreeIterator::First()
{
  if (mStartCData)
    mIterState = eUseStartCData;
  else if (mIter)
  {
    mIter->First();

    mIterState = eUseIterator;
  }
  else if (mEndCData)
    mIterState = eUseEndCData;
  else
    mIterState = eDone;
}

void
RangeSubtreeIterator::Last()
{
  if (mEndCData)
    mIterState = eUseEndCData;
  else if (mIter)
  {
    mIter->Last();

    mIterState = eUseIterator;
  }
  else if (mStartCData)
    mIterState = eUseStartCData;
  else
    mIterState = eDone;
}

void
RangeSubtreeIterator::Next()
{
  if (mIterState == eUseStartCData)
  {
    if (mIter)
    {
      mIter->First();

      mIterState = eUseIterator;
    }
    else if (mEndCData)
      mIterState = eUseEndCData;
    else
      mIterState = eDone;
  }
  else if (mIterState == eUseIterator)
  {
    mIter->Next();

    if (mIter->IsDone())
    {
      if (mEndCData)
        mIterState = eUseEndCData;
      else
        mIterState = eDone;
    }
  }
  else
    mIterState = eDone;
}

void
RangeSubtreeIterator::Prev()
{
  if (mIterState == eUseEndCData)
  {
    if (mIter)
    {
      mIter->Last();

      mIterState = eUseIterator;
    }
    else if (mStartCData)
      mIterState = eUseStartCData;
    else
      mIterState = eDone;
  }
  else if (mIterState == eUseIterator)
  {
    mIter->Prev();

    if (mIter->IsDone())
    {
      if (mStartCData)
        mIterState = eUseStartCData;
      else
        mIterState = eDone;
    }
  }
  else
    mIterState = eDone;
}












static nsresult
CollapseRangeAfterDelete(nsIDOMRange *aRange)
{
  NS_ENSURE_ARG_POINTER(aRange);

  

  PRBool isCollapsed = PR_FALSE;
  nsresult res = aRange->GetCollapsed(&isCollapsed);
  if (NS_FAILED(res)) return res;

  if (isCollapsed)
  {
    
    
    
    
    
    
    
    
    
    
    

    return NS_OK;
  }

  
  

  nsCOMPtr<nsIDOMNode> commonAncestor;
  res = aRange->GetCommonAncestorContainer(getter_AddRefs(commonAncestor));
  if(NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMNode> startContainer, endContainer;

  res = aRange->GetStartContainer(getter_AddRefs(startContainer));
  if (NS_FAILED(res)) return res;

  res = aRange->GetEndContainer(getter_AddRefs(endContainer));
  if (NS_FAILED(res)) return res;

  
  
  
  

  if (startContainer == commonAncestor)
    return aRange->Collapse(PR_TRUE);
  if (endContainer == commonAncestor)
    return aRange->Collapse(PR_FALSE);

  
  
  

  nsCOMPtr<nsIDOMNode> nodeToSelect(startContainer), parent;

  while (nodeToSelect)
  {
    nsresult res = nodeToSelect->GetParentNode(getter_AddRefs(parent));
    if (NS_FAILED(res)) return res;

    if (parent == commonAncestor)
      break; 

    nodeToSelect = parent;
  }

  if (!nodeToSelect)
    return NS_ERROR_FAILURE; 

  res = aRange->SelectNode(nodeToSelect);
  if (NS_FAILED(res)) return res;

  return aRange->Collapse(PR_FALSE);
}

nsresult nsRange::DeleteContents()
{ 
  if(IsDetached())
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  
  mozAutoSubtreeModified subtree(mRoot ? mRoot->GetOwnerDoc(): nsnull, nsnull);

  
  

  nsCOMPtr<nsIDOMNode> startContainer = do_QueryInterface(mStartParent);
  PRInt32              startOffset = mStartOffset;
  nsCOMPtr<nsIDOMNode> endContainer = do_QueryInterface(mEndParent);
  PRInt32              endOffset = mEndOffset;

  
  

  RangeSubtreeIterator iter;

  nsresult res = iter.Init(this);
  if (NS_FAILED(res)) return res;

  if (iter.IsDone())
  {
    
    return CollapseRangeAfterDelete(this);
  }

  

  iter.Last();

  PRBool handled = PR_FALSE;

  
  
  

  while (!iter.IsDone())
  {
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());

    
    

    iter.Prev();

    handled = PR_FALSE;

    
    
    
    
    

    nsCOMPtr<nsIDOMCharacterData> charData(do_QueryInterface(node));

    if (charData)
    {
      PRUint32 dataLength = 0;

      if (node == startContainer)
      {
        if (node == endContainer)
        {
          
          

          if (endOffset > startOffset)
          {
            res = charData->DeleteData(startOffset, endOffset - startOffset);
            if (NS_FAILED(res)) return res;
          }

          handled = PR_TRUE;
        }
        else
        {
          

          res = charData->GetLength(&dataLength);
          if (NS_FAILED(res)) return res;

          if (dataLength > (PRUint32)startOffset)
          {
            res = charData->DeleteData(startOffset, dataLength - startOffset);
            if (NS_FAILED(res)) return res;
          }

          handled = PR_TRUE;
        }
      }
      else if (node == endContainer)
      {
        

        if (endOffset > 0)
        {
          res = charData->DeleteData(0, endOffset);
          if (NS_FAILED(res)) return res;
        }

        handled = PR_TRUE;
      }       
    }

    if (!handled)
    {
      
      

      nsCOMPtr<nsIDOMNode> parent, tmpNode;

      node->GetParentNode(getter_AddRefs(parent));

      if (parent) {
        res = parent->RemoveChild(node, getter_AddRefs(tmpNode));
        if (NS_FAILED(res)) return res;
      }
    }
  }

  
  
  
  
  
  
  
  
  
  

  return CollapseRangeAfterDelete(this);
}

NS_IMETHODIMP
nsRange::CompareBoundaryPoints(PRUint16 aHow, nsIDOMRange* aOtherRange,
                               PRInt16* aCmpRet)
{
  nsCOMPtr<nsIRange> otherRange = do_QueryInterface(aOtherRange);
  NS_ENSURE_TRUE(otherRange, NS_ERROR_NULL_POINTER);

  if(mIsDetached || otherRange->IsDetached())
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  if (!mIsPositioned || !otherRange->IsPositioned())
    return NS_ERROR_NOT_INITIALIZED;

  nsINode *ourNode, *otherNode;
  PRInt32 ourOffset, otherOffset;

  switch (aHow) {
    case nsIDOMRange::START_TO_START:
      ourNode = mStartParent;
      ourOffset = mStartOffset;
      otherNode = otherRange->GetStartParent();
      otherOffset = otherRange->StartOffset();
      break;
    case nsIDOMRange::START_TO_END:
      ourNode = mEndParent;
      ourOffset = mEndOffset;
      otherNode = otherRange->GetStartParent();
      otherOffset = otherRange->StartOffset();
      break;
    case nsIDOMRange::END_TO_START:
      ourNode = mStartParent;
      ourOffset = mStartOffset;
      otherNode = otherRange->GetEndParent();
      otherOffset = otherRange->EndOffset();
      break;
    case nsIDOMRange::END_TO_END:
      ourNode = mEndParent;
      ourOffset = mEndOffset;
      otherNode = otherRange->GetEndParent();
      otherOffset = otherRange->EndOffset();
      break;
    default:
      
      return NS_ERROR_ILLEGAL_VALUE;
  }

  if (mRoot != otherRange->GetRoot())
    return NS_ERROR_DOM_WRONG_DOCUMENT_ERR;

  *aCmpRet = nsContentUtils::ComparePoints(ourNode, ourOffset,
                                           otherNode, otherOffset);

  return NS_OK;
}

nsresult nsRange::ExtractContents(nsIDOMDocumentFragment** aReturn)
{ 
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  
  mozAutoSubtreeModified subtree(mRoot ? mRoot->GetOwnerDoc(): nsnull, nsnull);

  
  
  
  
  

  nsresult res = CloneContents(aReturn);
  if (NS_FAILED(res))
    return res;
  res = DeleteContents();
  return res; 
}

static nsresult
CloneParentsBetween(nsIDOMNode *aAncestor,
                    nsIDOMNode *aNode,
                    nsIDOMNode **aClosestAncestor,
                    nsIDOMNode **aFarthestAncestor)
{
  NS_ENSURE_ARG_POINTER((aAncestor && aNode && aClosestAncestor && aFarthestAncestor));

  *aClosestAncestor  = nsnull;
  *aFarthestAncestor = nsnull;

  if (aAncestor == aNode)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> parent, firstParent, lastParent;

  nsresult res = aNode->GetParentNode(getter_AddRefs(parent));

  while(parent && parent != aAncestor)
  {
    nsCOMPtr<nsIDOMNode> clone, tmpNode;

    res = parent->CloneNode(PR_FALSE, getter_AddRefs(clone));

    if (NS_FAILED(res)) return res;
    if (!clone)         return NS_ERROR_FAILURE;

    if (! firstParent)
      firstParent = lastParent = clone;
    else
    {
      res = clone->AppendChild(lastParent, getter_AddRefs(tmpNode));

      if (NS_FAILED(res)) return res;

      lastParent = clone;
    }

    tmpNode = parent;
    res = tmpNode->GetParentNode(getter_AddRefs(parent));
  }

  *aClosestAncestor  = firstParent;
  NS_IF_ADDREF(*aClosestAncestor);

  *aFarthestAncestor = lastParent;
  NS_IF_ADDREF(*aFarthestAncestor);

  return NS_OK;
}

nsresult nsRange::CloneContents(nsIDOMDocumentFragment** aReturn)
{
  if (IsDetached())
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  nsresult res;
  nsCOMPtr<nsIDOMNode> commonAncestor;
  res = GetCommonAncestorContainer(getter_AddRefs(commonAncestor));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMDocument> document =
    do_QueryInterface(mStartParent->GetOwnerDoc());
  NS_ASSERTION(document, "CloneContents needs a document to continue.");
  if (!document) return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsIDOMDocumentFragment> clonedFrag;

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(document));

  res = NS_NewDocumentFragment(getter_AddRefs(clonedFrag),
                               doc->NodeInfoManager());
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMNode> commonCloneAncestor(do_QueryInterface(clonedFrag));
  if (!commonCloneAncestor) return NS_ERROR_FAILURE;

  
  

  RangeSubtreeIterator iter;

  res = iter.Init(this);
  if (NS_FAILED(res)) return res;

  if (iter.IsDone())
  {
    

    *aReturn = clonedFrag;
    NS_IF_ADDREF(*aReturn);
    return NS_OK;
  }

  iter.First();

  
  
  
  
  
  
  
  

  while (!iter.IsDone())
  {
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());
    nsCOMPtr<nsINode> iNode = do_QueryInterface(node);
    

    nsCOMPtr<nsIDOMNode> clone;
    res = node->CloneNode(PR_TRUE, getter_AddRefs(clone));
    if (NS_FAILED(res)) return res;

    
    
    
    
    

    nsCOMPtr<nsIDOMCharacterData> charData(do_QueryInterface(clone));

    if (charData)
    {
      if (iNode == mEndParent)
      {
        
        

        PRUint32 dataLength = 0;
        res = charData->GetLength(&dataLength);
        if (NS_FAILED(res)) return res;

        if (dataLength > (PRUint32)mEndOffset)
        {
          res = charData->DeleteData(mEndOffset, dataLength - mEndOffset);
          if (NS_FAILED(res)) return res;
        }
      }       

      if (iNode == mStartParent)
      {
        
        

        if (mStartOffset > 0)
        {
          res = charData->DeleteData(0, mStartOffset);
          if (NS_FAILED(res)) return res;
        }
      }
    }

    

    nsCOMPtr<nsIDOMNode> closestAncestor, farthestAncestor;

    res = CloneParentsBetween(commonAncestor, node,
                              getter_AddRefs(closestAncestor),
                              getter_AddRefs(farthestAncestor));

    if (NS_FAILED(res)) return res;

    

    nsCOMPtr<nsIDOMNode> tmpNode;

    if (farthestAncestor)
    {
      res = commonCloneAncestor->AppendChild(farthestAncestor,
                                             getter_AddRefs(tmpNode));

      if (NS_FAILED(res)) return res;
    }

    

    if (closestAncestor)
    {
      
      

      res = closestAncestor->AppendChild(clone, getter_AddRefs(tmpNode));
    }
    else
    {
      
      

      res = commonCloneAncestor->AppendChild(clone, getter_AddRefs(tmpNode));
    }
    if (NS_FAILED(res)) return res;

    
    

    iter.Next();

    if (iter.IsDone())
      break; 

    nsCOMPtr<nsIDOMNode> nextNode(iter.GetCurrentNode());
    if (!nextNode) return NS_ERROR_FAILURE;

    
    nsContentUtils::GetCommonAncestor(node, nextNode, getter_AddRefs(commonAncestor));

    if (!commonAncestor)
      return NS_ERROR_FAILURE;

    

    while (node && node != commonAncestor)
    {
      tmpNode = node;
      res = tmpNode->GetParentNode(getter_AddRefs(node));
      if (NS_FAILED(res)) return res;
      if (!node) return NS_ERROR_FAILURE;

      tmpNode = clone;
      res = tmpNode->GetParentNode(getter_AddRefs(clone));
      if (NS_FAILED(res)) return res;
      if (!node) return NS_ERROR_FAILURE;
    }

    commonCloneAncestor = clone;
  }

  *aReturn = clonedFrag;
  NS_IF_ADDREF(*aReturn);

  return NS_OK;
}

nsresult nsRange::CloneRange(nsIDOMRange** aReturn)
{
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  if (aReturn == 0)
    return NS_ERROR_NULL_POINTER;

  nsRange* range = new nsRange();
  NS_ENSURE_TRUE(range, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aReturn = range);
  
  range->SetMaySpanAnonymousSubtrees(mMaySpanAnonymousSubtrees);

  range->DoSetRange(mStartParent, mStartOffset, mEndParent, mEndOffset, mRoot);

  return NS_OK;
}

nsresult nsRange::InsertNode(nsIDOMNode* aN)
{
  VALIDATE_ACCESS(aN);
  
  nsresult res;
  PRInt32 tStartOffset;
  this->GetStartOffset(&tStartOffset);

  nsCOMPtr<nsIDOMNode> tStartContainer;
  res = this->GetStartContainer(getter_AddRefs(tStartContainer));
  if(NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMText> startTextNode(do_QueryInterface(tStartContainer));
  if (startTextNode)
  {
    nsCOMPtr<nsIDOMNode> tSCParentNode;
    res = tStartContainer->GetParentNode(getter_AddRefs(tSCParentNode));
    if(NS_FAILED(res)) return res;
    
    PRBool isCollapsed;
    res = GetCollapsed(&isCollapsed);
    if(NS_FAILED(res)) return res;

    PRInt32 tEndOffset;
    GetEndOffset(&tEndOffset);

    nsCOMPtr<nsIDOMText> secondPart;
    res = startTextNode->SplitText(tStartOffset, getter_AddRefs(secondPart));
    if (NS_FAILED(res)) return res;
    
    
    if (!isCollapsed)
    {
      res = SetEnd(secondPart, tEndOffset - tStartOffset);
      if(NS_FAILED(res)) return res;
    }
    
    nsCOMPtr<nsIDOMNode> tResultNode;
    return tSCParentNode->InsertBefore(aN, secondPart, getter_AddRefs(tResultNode));
  }  

  nsCOMPtr<nsIDOMNodeList>tChildList;
  res = tStartContainer->GetChildNodes(getter_AddRefs(tChildList));
  if(NS_FAILED(res)) return res;
  PRUint32 tChildListLength;
  res = tChildList->GetLength(&tChildListLength);
  if(NS_FAILED(res)) return res;

  
  nsCOMPtr<nsIDOMNode>tChildNode;
  res = tChildList->Item(tStartOffset, getter_AddRefs(tChildNode));
  if(NS_FAILED(res)) return res;
  
  nsCOMPtr<nsIDOMNode> tResultNode;
  return tStartContainer->InsertBefore(aN, tChildNode, getter_AddRefs(tResultNode));
}

nsresult nsRange::SurroundContents(nsIDOMNode* aNewParent)
{
  VALIDATE_ACCESS(aNewParent);
  
  

  nsCOMPtr<nsIDOMDocumentFragment> docFrag;

  nsresult res = ExtractContents(getter_AddRefs(docFrag));

  if (NS_FAILED(res)) return res;
  if (!docFrag) return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsIDOMNodeList> children;
  res = aNewParent->GetChildNodes(getter_AddRefs(children));

  if (NS_FAILED(res)) return res;
  if (!children) return NS_ERROR_FAILURE;

  PRUint32 numChildren = 0;
  res = children->GetLength(&numChildren);
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMNode> tmpNode;

  while (numChildren)
  {
    nsCOMPtr<nsIDOMNode> child;
    res = children->Item(--numChildren, getter_AddRefs(child));

    if (NS_FAILED(res)) return res;
    if (!child) return NS_ERROR_FAILURE;

    res = aNewParent->RemoveChild(child, getter_AddRefs(tmpNode));
    if (NS_FAILED(res)) return res;
  }

  

  res = InsertNode(aNewParent);
  if (NS_FAILED(res)) return res;

  

  res = aNewParent->AppendChild(docFrag, getter_AddRefs(tmpNode));
  if (NS_FAILED(res)) return res;

  

  return SelectNode(aNewParent);
}

nsresult nsRange::ToString(nsAString& aReturn)
{ 
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  
  aReturn.Truncate();
  
  
  if (!mIsPositioned) {
    return NS_OK;
  }

#ifdef DEBUG_range
      printf("Range dump: -----------------------\n");
#endif 
    
  
  if (mStartParent == mEndParent)
  {
    nsCOMPtr<nsIDOMText> textNode( do_QueryInterface(mStartParent) );
    
    if (textNode)
    {
#ifdef DEBUG_range
      
      nsCOMPtr<nsIContent> cN (do_QueryInterface(mStartParent));
      if (cN) cN->List(stdout);
      printf("End Range dump: -----------------------\n");
#endif 

      
      if (NS_FAILED(textNode->SubstringData(mStartOffset,mEndOffset-mStartOffset,aReturn)))
        return NS_ERROR_UNEXPECTED;
      return NS_OK;
    }
  } 
  
  



  nsCOMPtr<nsIContentIterator> iter;
  NS_NewContentIterator(getter_AddRefs(iter));
  nsresult rv = iter->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString tempString;
 
  
  
  while (!iter->IsDone())
  {
    nsIContent *cN = iter->GetCurrentNode();

#ifdef DEBUG_range
    
    cN->List(stdout);
#endif 
    nsCOMPtr<nsIDOMText> textNode( do_QueryInterface(cN) );
    if (textNode) 
    {
      if (cN == mStartParent) 
      {
        PRUint32 strLength;
        textNode->GetLength(&strLength);
        textNode->SubstringData(mStartOffset,strLength-mStartOffset,tempString);
        aReturn += tempString;
      }
      else if (cN == mEndParent)  
      {
        textNode->SubstringData(0,mEndOffset,tempString);
        aReturn += tempString;
      }
      else  
      {
        textNode->GetData(tempString);
        aReturn += tempString;
      }
    }

    iter->Next();
  }

#ifdef DEBUG_range
  printf("End Range dump: -----------------------\n");
#endif 
  return NS_OK;
}



nsresult
nsRange::Detach()
{
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  mIsDetached = PR_TRUE;

  DoSetRange(nsnull, 0, nsnull, 0, nsnull);
  
  return NS_OK;
}


NS_IMETHODIMP    
nsRange::CreateContextualFragment(const nsAString& aFragment,
                                  nsIDOMDocumentFragment** aReturn)
{
  nsCOMPtr<nsIDOMNode> start = do_QueryInterface(mStartParent);
  return
    mIsPositioned
    ? nsContentUtils::CreateContextualFragment(start, aFragment, aReturn)
    : NS_ERROR_FAILURE;
}
