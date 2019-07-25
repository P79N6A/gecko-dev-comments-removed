








































#include "nscore.h"
#include "nsRange.h"

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMText.h"
#include "nsDOMError.h"
#include "nsIContentIterator.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsGenericDOMDataNode.h"
#include "nsClientRect.h"
#include "nsLayoutUtils.h"
#include "nsTextFrame.h"
#include "nsFontFaceList.h"

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
nsRange::CompareNodeToRange(nsINode* aNode, nsIDOMRange* aRange,
                            bool *outNodeBefore, bool *outNodeAfter)
{
  nsresult rv;
  nsCOMPtr<nsIRange> range = do_QueryInterface(aRange, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return CompareNodeToRange(aNode, range, outNodeBefore, outNodeAfter);
}


nsresult
nsRange::CompareNodeToRange(nsINode* aNode, nsIRange* aRange,
                            bool *outNodeBefore, bool *outNodeAfter)
{
  NS_ENSURE_STATE(aNode);
  
  
  
  
  
  
  
  if (!aRange || !aRange->IsPositioned()) 
    return NS_ERROR_UNEXPECTED; 
  
  
  PRInt32 nodeStart, nodeEnd;
  nsINode* parent = aNode->GetNodeParent();
  if (!parent) {
    
    
    
    parent = aNode;
    nodeStart = 0;
    nodeEnd = aNode->GetChildCount();
  }
  else {
    nodeStart = parent->IndexOf(aNode);
    nodeEnd = nodeStart + 1;
  }

  nsINode* rangeStartParent = aRange->GetStartParent();
  nsINode* rangeEndParent = aRange->GetEndParent();
  PRInt32 rangeStartOffset = aRange->StartOffset();
  PRInt32 rangeEndOffset = aRange->EndOffset();

  
  bool disconnected = false;
  *outNodeBefore = nsContentUtils::ComparePoints(rangeStartParent,
                                                 rangeStartOffset,
                                                 parent, nodeStart,
                                                 &disconnected) > 0;
  NS_ENSURE_TRUE(!disconnected, NS_ERROR_DOM_WRONG_DOCUMENT_ERR);

  
  *outNodeAfter = nsContentUtils::ComparePoints(rangeEndParent,
                                                rangeEndOffset,
                                                parent, nodeEnd,
                                                &disconnected) < 0;
  NS_ENSURE_TRUE(!disconnected, NS_ERROR_DOM_WRONG_DOCUMENT_ERR);
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
                                 bool *outNodeBefore, bool *outNodeAfter)
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





NS_IMPL_CYCLE_COLLECTION_CLASS(nsRange)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsRange)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsRange)

DOMCI_DATA(Range, nsRange)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsRange)
  NS_INTERFACE_MAP_ENTRY(nsIDOMRange)
  NS_INTERFACE_MAP_ENTRY(nsIRange)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSRange)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIRange)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Range)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsRange)
  tmp->Reset();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsRange)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mStartParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mEndParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRoot)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END





void
nsRange::CharacterDataChanged(nsIDocument* aDocument,
                              nsIContent* aContent,
                              CharacterDataChangeInfo* aInfo)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  nsINode* newRoot = nsnull;
  nsINode* newStartNode = nsnull;
  nsINode* newEndNode = nsnull;
  PRUint32 newStartOffset = 0;
  PRUint32 newEndOffset = 0;

  
  
  if (aContent == mStartParent &&
      aInfo->mChangeStart < static_cast<PRUint32>(mStartOffset)) {
    if (aInfo->mDetails) {
      
      NS_ASSERTION(aInfo->mDetails->mType ==
                   CharacterDataChangeInfo::Details::eSplit,
                   "only a split can start before the end");
      NS_ASSERTION(static_cast<PRUint32>(mStartOffset) <= aInfo->mChangeEnd,
                   "mStartOffset is beyond the end of this node");
      newStartOffset = static_cast<PRUint32>(mStartOffset) - aInfo->mChangeStart;
      newStartNode = aInfo->mDetails->mNextSibling;
      if (NS_UNLIKELY(aContent == mRoot)) {
        newRoot = IsValidBoundary(newStartNode);
      }
    } else {
      
      
      mStartOffset = static_cast<PRUint32>(mStartOffset) <= aInfo->mChangeEnd ?
        aInfo->mChangeStart :
        mStartOffset + aInfo->mChangeStart - aInfo->mChangeEnd +
          aInfo->mReplaceLength;
    }
  }

  
  
  
  if (aContent == mEndParent &&
      aInfo->mChangeStart < static_cast<PRUint32>(mEndOffset)) {
    if (aInfo->mDetails && (aContent->GetParent() || newStartNode)) {
      
      NS_ASSERTION(aInfo->mDetails->mType ==
                   CharacterDataChangeInfo::Details::eSplit,
                   "only a split can start before the end");
      NS_ASSERTION(static_cast<PRUint32>(mEndOffset) <= aInfo->mChangeEnd,
                   "mEndOffset is beyond the end of this node");
      newEndOffset = static_cast<PRUint32>(mEndOffset) - aInfo->mChangeStart;
      newEndNode = aInfo->mDetails->mNextSibling;
    } else {
      mEndOffset = static_cast<PRUint32>(mEndOffset) <= aInfo->mChangeEnd ?
        aInfo->mChangeStart :
        mEndOffset + aInfo->mChangeStart - aInfo->mChangeEnd +
          aInfo->mReplaceLength;
    }
  }

  if (aInfo->mDetails &&
      aInfo->mDetails->mType == CharacterDataChangeInfo::Details::eMerge) {
    
    
    nsIContent* removed = aInfo->mDetails->mNextSibling;
    if (removed == mStartParent) {
      newStartOffset = static_cast<PRUint32>(mStartOffset) + aInfo->mChangeStart;
      newStartNode = aContent;
      if (NS_UNLIKELY(removed == mRoot)) {
        newRoot = IsValidBoundary(newStartNode);
      }
    }
    if (removed == mEndParent) {
      newEndOffset = static_cast<PRUint32>(mEndOffset) + aInfo->mChangeStart;
      newEndNode = aContent;
      if (NS_UNLIKELY(removed == mRoot)) {
        newRoot = IsValidBoundary(newEndNode);
      }
    }
  }
  if (newStartNode || newEndNode) {
    if (!newStartNode) {
      newStartNode = mStartParent;
      newStartOffset = mStartOffset;
    }
    if (!newEndNode) {
      newEndNode = mEndParent;
      newEndOffset = mEndOffset;
    }
    DoSetRange(newStartNode, newStartOffset, newEndNode, newEndOffset,
               newRoot ? newRoot : mRoot.get()
#ifdef DEBUG
               , !newEndNode->GetParent() || !newStartNode->GetParent()
#endif
               );
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
                        PRInt32 aIndexInContainer,
                        nsIContent* aPreviousSibling)
{
  NS_ASSERTION(mIsPositioned, "shouldn't be notified if not positioned");

  nsINode* container = NODE_FROM(aContainer, aDocument);

  
  if (container == mStartParent) {
    if (aIndexInContainer < mStartOffset) {
      --mStartOffset;
    }
  }
  
  else if (nsContentUtils::ContentIsDescendantOf(mStartParent, aChild)) {
    mStartParent = container;
    mStartOffset = aIndexInContainer;
  }

  
  if (container == mEndParent) {
    if (aIndexInContainer < mEndOffset) {
      --mEndOffset;
    }
  }
  else if (nsContentUtils::ContentIsDescendantOf(mEndParent, aChild)) {
    mEndParent = container;
    mEndOffset = aIndexInContainer;
  }
}

void
nsRange::ParentChainChanged(nsIContent *aContent)
{
  NS_ASSERTION(mRoot == aContent, "Wrong ParentChainChanged notification?");
  nsINode* newRoot = IsValidBoundary(mStartParent);
  NS_ASSERTION(newRoot, "No valid boundary or root found!");
  NS_ASSERTION(newRoot == IsValidBoundary(mEndParent),
               "Start parent and end parent give different root!");
  
  
  DoSetRange(mStartParent, mStartOffset, mEndParent, mEndOffset, newRoot);
}




NS_IMETHODIMP
nsRange::IsPointInRange(nsIDOMNode* aParent, PRInt32 aOffset, bool* aResult)
{
  PRInt16 compareResult = 0;
  nsresult rv = ComparePoint(aParent, aOffset, &compareResult);
  
  if (rv == NS_ERROR_DOM_WRONG_DOCUMENT_ERR) {
    *aResult = false;
    return NS_OK;
  }

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
  





static PRUint32 GetNodeLength(nsINode *aNode)
{
  if(aNode->IsNodeOfType(nsINode::eDATA_NODE)) {
    return static_cast<nsIContent*>(aNode)->TextLength();
  }

  return aNode->GetChildCount();
}






void
nsRange::DoSetRange(nsINode* aStartN, PRInt32 aStartOffset,
                    nsINode* aEndN, PRInt32 aEndOffset,
                    nsINode* aRoot
#ifdef DEBUG
                    , bool aNotInsertedYet
#endif
                    )
{
  NS_PRECONDITION((aStartN && aEndN && aRoot) ||
                  (!aStartN && !aEndN && !aRoot),
                  "Set all or none");
  NS_PRECONDITION(!aRoot || aNotInsertedYet ||
                  (nsContentUtils::ContentIsDescendantOf(aStartN, aRoot) &&
                   nsContentUtils::ContentIsDescendantOf(aEndN, aRoot) &&
                   aRoot == IsValidBoundary(aStartN) &&
                   aRoot == IsValidBoundary(aEndN)),
                  "Wrong root");
  NS_PRECONDITION(!aRoot ||
                  (aStartN->IsNodeOfType(nsINode::eCONTENT) &&
                   aEndN->IsNodeOfType(nsINode::eCONTENT) &&
                   aRoot ==
                    static_cast<nsIContent*>(aStartN)->GetBindingParent() &&
                   aRoot ==
                    static_cast<nsIContent*>(aEndN)->GetBindingParent()) ||
                  (!aRoot->GetNodeParent() &&
                   (aRoot->IsNodeOfType(nsINode::eDOCUMENT) ||
                    aRoot->IsNodeOfType(nsINode::eATTRIBUTE) ||
                    aRoot->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT) ||
                     
                    aRoot->IsNodeOfType(nsINode::eCONTENT))),
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
nsRange::GetCommonAncestor() const
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





NS_IMETHODIMP
nsRange::GetStartContainer(nsIDOMNode** aStartParent)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  return CallQueryInterface(mStartParent, aStartParent);
}

NS_IMETHODIMP
nsRange::GetStartOffset(PRInt32* aStartOffset)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aStartOffset = mStartOffset;

  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetEndContainer(nsIDOMNode** aEndParent)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  return CallQueryInterface(mEndParent, aEndParent);
}

NS_IMETHODIMP
nsRange::GetEndOffset(PRInt32* aEndOffset)
{
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aEndOffset = mEndOffset;

  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetCollapsed(bool* aIsCollapsed)
{
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  if (!mIsPositioned)
    return NS_ERROR_NOT_INITIALIZED;

  *aIsCollapsed = Collapsed();

  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetCommonAncestorContainer(nsIDOMNode** aCommonParent)
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
    nsIContent* content = static_cast<nsIContent*>(aNode);
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

#ifdef DEBUG_smaug
  NS_WARN_IF_FALSE(root->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT) ||
                   root->IsNodeOfType(nsINode::eATTRIBUTE),
                   "Creating a DOM Range using root which isn't in DOM!");
#endif

  
  return root;
}

NS_IMETHODIMP
nsRange::SetStart(nsIDOMNode* aParent, PRInt32 aOffset)
{
  VALIDATE_ACCESS(aParent);

  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  return SetStart(parent, aOffset);
}

 nsresult
nsRange::SetStart(nsINode* aParent, PRInt32 aOffset)
{
  nsINode* newRoot = IsValidBoundary(aParent);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR);

  PRInt32 len = GetNodeLength(aParent);
  if (aOffset < 0 || aOffset > len)
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  
  
  if (!mIsPositioned || newRoot != mRoot ||
      nsContentUtils::ComparePoints(aParent, aOffset,
                                    mEndParent, mEndOffset) == 1) {
    DoSetRange(aParent, aOffset, aParent, aOffset, newRoot);

    return NS_OK;
  }

  DoSetRange(aParent, aOffset, mEndParent, mEndOffset, mRoot);
  
  return NS_OK;
}

NS_IMETHODIMP
nsRange::SetStartBefore(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> parent;
  nsresult rv = aSibling->GetParentNode(getter_AddRefs(parent));
  if (NS_FAILED(rv) || !parent) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  return SetStart(parent, IndexOf(aSibling));
}

NS_IMETHODIMP
nsRange::SetStartAfter(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);

  nsCOMPtr<nsIDOMNode> nParent;
  nsresult res = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(res) || !nParent) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  return SetStart(nParent, IndexOf(aSibling) + 1);
}

NS_IMETHODIMP
nsRange::SetEnd(nsIDOMNode* aParent, PRInt32 aOffset)
{
  VALIDATE_ACCESS(aParent);

  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  return SetEnd(parent, aOffset);
}


 nsresult
nsRange::SetEnd(nsINode* aParent, PRInt32 aOffset)
{
  nsINode* newRoot = IsValidBoundary(aParent);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR);

  PRInt32 len = GetNodeLength(aParent);
  if (aOffset < 0 || aOffset > len) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  
  if (!mIsPositioned || newRoot != mRoot ||
      nsContentUtils::ComparePoints(mStartParent, mStartOffset,
                                    aParent, aOffset) == 1) {
    DoSetRange(aParent, aOffset, aParent, aOffset, newRoot);

    return NS_OK;
  }

  DoSetRange(mStartParent, mStartOffset, aParent, aOffset, mRoot);

  return NS_OK;
}

NS_IMETHODIMP
nsRange::SetEndBefore(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> nParent;
  nsresult rv = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(rv) || !nParent) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  return SetEnd(nParent, IndexOf(aSibling));
}

NS_IMETHODIMP
nsRange::SetEndAfter(nsIDOMNode* aSibling)
{
  VALIDATE_ACCESS(aSibling);
  
  nsCOMPtr<nsIDOMNode> nParent;
  nsresult res = aSibling->GetParentNode(getter_AddRefs(nParent));
  if (NS_FAILED(res) || !nParent) {
    return NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR;
  }

  return SetEnd(nParent, IndexOf(aSibling) + 1);
}

NS_IMETHODIMP
nsRange::Collapse(bool aToStart)
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

NS_IMETHODIMP
nsRange::SelectNode(nsIDOMNode* aN)
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

NS_IMETHODIMP
nsRange::SelectNodeContents(nsIDOMNode* aN)
{
  VALIDATE_ACCESS(aN);

  nsCOMPtr<nsINode> node = do_QueryInterface(aN);
  nsINode* newRoot = IsValidBoundary(node);
  NS_ENSURE_TRUE(newRoot, NS_ERROR_DOM_RANGE_INVALID_NODE_TYPE_ERR);
  
  DoSetRange(node, 0, node, GetNodeLength(node), newRoot);
  
  return NS_OK;
}














class NS_STACK_CLASS RangeSubtreeIterator
{
private:

  enum RangeSubtreeIterState { eDone=0,
                               eUseStart,
                               eUseIterator,
                               eUseEnd };

  nsCOMPtr<nsIContentIterator>  mIter;
  RangeSubtreeIterState         mIterState;

  nsCOMPtr<nsIDOMNode> mStart;
  nsCOMPtr<nsIDOMNode> mEnd;

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

  bool IsDone()
  {
    return mIterState == eDone;
  }
};

nsresult
RangeSubtreeIterator::Init(nsIDOMRange *aRange)
{
  mIterState = eDone;
  bool collapsed;
  aRange->GetCollapsed(&collapsed);
  if (collapsed) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> node;

  
  
  

  nsresult res = aRange->GetStartContainer(getter_AddRefs(node));
  if (!node) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCharacterData> startData = do_QueryInterface(node);
  if (startData) {
    mStart = node;
  } else {
    PRInt32 startIndex;
    aRange->GetStartOffset(&startIndex);
    nsCOMPtr<nsINode> iNode = do_QueryInterface(node);
    if (iNode->IsElement() && 
        PRInt32(iNode->AsElement()->GetChildCount()) == startIndex) {
      mStart = node;
    }
  }

  
  
  

  res = aRange->GetEndContainer(getter_AddRefs(node));
  if (!node) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCharacterData> endData = do_QueryInterface(node);
  if (endData) {
    mEnd = node;
  } else {
    PRInt32 endIndex;
    aRange->GetEndOffset(&endIndex);
    nsCOMPtr<nsINode> iNode = do_QueryInterface(node);
    if (iNode->IsElement() && endIndex == 0) {
      mEnd = node;
    }
  }

  if (mStart && mStart == mEnd)
  {
    
    
    

    mEnd = nsnull;
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

  if (mIterState == eUseStart && mStart) {
    NS_ADDREF(node = mStart);
  } else if (mIterState == eUseEnd && mEnd)
    NS_ADDREF(node = mEnd);
  else if (mIterState == eUseIterator && mIter)
  {
    nsINode* n = mIter->GetCurrentNode();

    if (n) {
      CallQueryInterface(n, &node);
    }
  }

  return node;
}

void
RangeSubtreeIterator::First()
{
  if (mStart)
    mIterState = eUseStart;
  else if (mIter)
  {
    mIter->First();

    mIterState = eUseIterator;
  }
  else if (mEnd)
    mIterState = eUseEnd;
  else
    mIterState = eDone;
}

void
RangeSubtreeIterator::Last()
{
  if (mEnd)
    mIterState = eUseEnd;
  else if (mIter)
  {
    mIter->Last();

    mIterState = eUseIterator;
  }
  else if (mStart)
    mIterState = eUseStart;
  else
    mIterState = eDone;
}

void
RangeSubtreeIterator::Next()
{
  if (mIterState == eUseStart)
  {
    if (mIter)
    {
      mIter->First();

      mIterState = eUseIterator;
    }
    else if (mEnd)
      mIterState = eUseEnd;
    else
      mIterState = eDone;
  }
  else if (mIterState == eUseIterator)
  {
    mIter->Next();

    if (mIter->IsDone())
    {
      if (mEnd)
        mIterState = eUseEnd;
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
  if (mIterState == eUseEnd)
  {
    if (mIter)
    {
      mIter->Last();

      mIterState = eUseIterator;
    }
    else if (mStart)
      mIterState = eUseStart;
    else
      mIterState = eDone;
  }
  else if (mIterState == eUseIterator)
  {
    mIter->Prev();

    if (mIter->IsDone())
    {
      if (mStart)
        mIterState = eUseStart;
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

  

  bool isCollapsed = false;
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
    return aRange->Collapse(true);
  if (endContainer == commonAncestor)
    return aRange->Collapse(false);

  
  
  

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

  return aRange->Collapse(false);
}






static nsresult
RemoveNode(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  nsCOMPtr<nsINode> parent = node->GetNodeParent();
  return parent ? parent->RemoveChild(node) : NS_OK;
}










static nsresult SplitDataNode(nsIDOMCharacterData* aStartNode,
                              PRUint32 aStartIndex,
                              nsIDOMCharacterData** aEndNode,
                              bool aCloneAfterOriginal = true)
{
  nsresult rv;
  nsCOMPtr<nsINode> node = do_QueryInterface(aStartNode);
  NS_ENSURE_STATE(node && node->IsNodeOfType(nsINode::eDATA_NODE));
  nsGenericDOMDataNode* dataNode = static_cast<nsGenericDOMDataNode*>(node.get());

  nsCOMPtr<nsIContent> newData;
  rv = dataNode->SplitData(aStartIndex, getter_AddRefs(newData),
                           aCloneAfterOriginal);
  NS_ENSURE_SUCCESS(rv, rv);
  return CallQueryInterface(newData, aEndNode);
}

NS_IMETHODIMP
PrependChild(nsIDOMNode* aParent, nsIDOMNode* aChild)
{
  nsCOMPtr<nsIDOMNode> first, tmpNode;
  aParent->GetFirstChild(getter_AddRefs(first));
  return aParent->InsertBefore(aChild, first, getter_AddRefs(tmpNode));
}

nsresult nsRange::CutContents(nsIDOMDocumentFragment** aFragment)
{ 
  if (aFragment) {
    *aFragment = nsnull;
  }

  if (IsDetached())
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  nsresult rv;

  nsCOMPtr<nsIDocument> doc = mStartParent->OwnerDoc();

  nsCOMPtr<nsIDOMNode> commonAncestor;
  rv = GetCommonAncestorContainer(getter_AddRefs(commonAncestor));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMDocumentFragment> retval;
  if (aFragment) {
    rv = NS_NewDocumentFragment(getter_AddRefs(retval),
                                doc->NodeInfoManager());
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsCOMPtr<nsIDOMNode> commonCloneAncestor(do_QueryInterface(retval));

  
  mozAutoSubtreeModified subtree(mRoot ? mRoot->OwnerDoc(): nsnull, nsnull);

  
  

  nsCOMPtr<nsIDOMNode> startContainer = do_QueryInterface(mStartParent);
  PRInt32              startOffset = mStartOffset;
  nsCOMPtr<nsIDOMNode> endContainer = do_QueryInterface(mEndParent);
  PRInt32              endOffset = mEndOffset;

  
  

  RangeSubtreeIterator iter;

  rv = iter.Init(this);
  if (NS_FAILED(rv)) return rv;

  if (iter.IsDone())
  {
    
    rv = CollapseRangeAfterDelete(this);
    if (NS_SUCCEEDED(rv) && aFragment) {
      NS_ADDREF(*aFragment = retval);
    }
    return rv;
  }

  

  iter.Last();

  bool handled = false;

  
  
  

  while (!iter.IsDone())
  {
    nsCOMPtr<nsIDOMNode> nodeToResult;
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());

    
    

    iter.Prev();

    handled = false;

    
    
    
    
    

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
            if (retval) {
              nsAutoString cutValue;
              rv = charData->SubstringData(startOffset, endOffset - startOffset,
                                           cutValue);
              NS_ENSURE_SUCCESS(rv, rv);
              nsCOMPtr<nsIDOMNode> clone;
              rv = charData->CloneNode(false, getter_AddRefs(clone));
              NS_ENSURE_SUCCESS(rv, rv);
              clone->SetNodeValue(cutValue);
              nodeToResult = clone;
            }

            rv = charData->DeleteData(startOffset, endOffset - startOffset);
            NS_ENSURE_SUCCESS(rv, rv);
          }

          handled = true;
        }
        else
        {
          

          rv = charData->GetLength(&dataLength);
          NS_ENSURE_SUCCESS(rv, rv);

          if (dataLength >= (PRUint32)startOffset)
          {
            nsCOMPtr<nsIDOMCharacterData> cutNode;
            rv = SplitDataNode(charData, startOffset, getter_AddRefs(cutNode));
            NS_ENSURE_SUCCESS(rv, rv);
            nodeToResult = cutNode;
          }

          handled = true;
        }
      }
      else if (node == endContainer)
      {
        

        if (endOffset >= 0)
        {
          nsCOMPtr<nsIDOMCharacterData> cutNode;
          


          rv = SplitDataNode(charData, endOffset, getter_AddRefs(cutNode),
                             false);
          NS_ENSURE_SUCCESS(rv, rv);
          nodeToResult = cutNode;
        }

        handled = true;
      }       
    }

    if (!handled && (node == endContainer || node == startContainer))
    {
      nsCOMPtr<nsINode> iNode = do_QueryInterface(node);
      if (iNode && iNode->IsElement() &&
          ((node == endContainer && endOffset == 0) ||
           (node == startContainer &&
            PRInt32(iNode->AsElement()->GetChildCount()) == startOffset)))
      {
        if (retval) {
          nsCOMPtr<nsIDOMNode> clone;
          rv = node->CloneNode(false, getter_AddRefs(clone));
          NS_ENSURE_SUCCESS(rv, rv);
          nodeToResult = clone;
        }
        handled = true;
      }
    }

    if (!handled)
    {
      
      
      nodeToResult = node;
    }

    PRUint32 parentCount = 0;
    nsCOMPtr<nsIDOMNode> tmpNode;
    
    if (retval) {
      nsCOMPtr<nsIDOMNode> oldCommonAncestor = commonAncestor;
      if (!iter.IsDone()) {
        
        nsCOMPtr<nsIDOMNode> prevNode(iter.GetCurrentNode());
        NS_ENSURE_STATE(prevNode);

        
        
        nsContentUtils::GetCommonAncestor(node, prevNode,
                                          getter_AddRefs(commonAncestor));
        NS_ENSURE_STATE(commonAncestor);

        nsCOMPtr<nsIDOMNode> parentCounterNode = node;
        while (parentCounterNode && parentCounterNode != commonAncestor)
        {
          ++parentCount;
          tmpNode = parentCounterNode;
          tmpNode->GetParentNode(getter_AddRefs(parentCounterNode));
          NS_ENSURE_STATE(parentCounterNode);
        }
      }

      
      nsCOMPtr<nsIDOMNode> closestAncestor, farthestAncestor;
      rv = CloneParentsBetween(oldCommonAncestor, node,
                               getter_AddRefs(closestAncestor),
                               getter_AddRefs(farthestAncestor));
      NS_ENSURE_SUCCESS(rv, rv);

      if (farthestAncestor)
      {
        rv = PrependChild(commonCloneAncestor, farthestAncestor);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = closestAncestor ? PrependChild(closestAncestor, nodeToResult)
                           : PrependChild(commonCloneAncestor, nodeToResult);
      NS_ENSURE_SUCCESS(rv, rv);
    } else if (nodeToResult) {
      rv = RemoveNode(nodeToResult);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!iter.IsDone() && retval) {
      
      nsCOMPtr<nsIDOMNode> newCloneAncestor = nodeToResult;
      for (PRUint32 i = parentCount; i; --i)
      {
        tmpNode = newCloneAncestor;
        tmpNode->GetParentNode(getter_AddRefs(newCloneAncestor));
        NS_ENSURE_STATE(newCloneAncestor);
      }
      commonCloneAncestor = newCloneAncestor;
    }
  }

  rv = CollapseRangeAfterDelete(this);
  if (NS_SUCCEEDED(rv) && aFragment) {
    NS_ADDREF(*aFragment = retval);
  }
  return rv;
}

NS_IMETHODIMP
nsRange::DeleteContents()
{
  return CutContents(nsnull);
}

NS_IMETHODIMP
nsRange::ExtractContents(nsIDOMDocumentFragment** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  return CutContents(aReturn);
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

 nsresult
nsRange::CloneParentsBetween(nsIDOMNode *aAncestor,
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

    res = parent->CloneNode(false, getter_AddRefs(clone));

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

NS_IMETHODIMP
nsRange::CloneContents(nsIDOMDocumentFragment** aReturn)
{
  if (IsDetached())
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  nsresult res;
  nsCOMPtr<nsIDOMNode> commonAncestor;
  res = GetCommonAncestorContainer(getter_AddRefs(commonAncestor));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIDOMDocument> document =
    do_QueryInterface(mStartParent->OwnerDoc());
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
    bool deepClone = !iNode->IsElement() ||
                       (!(iNode == mEndParent && mEndOffset == 0) &&
                        !(iNode == mStartParent &&
                          mStartOffset ==
                            PRInt32(iNode->AsElement()->GetChildCount())));

    

    nsCOMPtr<nsIDOMNode> clone;
    res = node->CloneNode(deepClone, getter_AddRefs(clone));
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
      if (!clone) return NS_ERROR_FAILURE;
    }

    commonCloneAncestor = clone;
  }

  *aReturn = clonedFrag;
  NS_IF_ADDREF(*aReturn);

  return NS_OK;
}

nsresult nsRange::DoCloneRange(nsIRange** aReturn) const
{
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  if (aReturn == 0)
    return NS_ERROR_NULL_POINTER;

  nsRefPtr<nsRange> range = new nsRange();
  NS_ENSURE_TRUE(range, NS_ERROR_OUT_OF_MEMORY);

  range->SetMaySpanAnonymousSubtrees(mMaySpanAnonymousSubtrees);

  range->DoSetRange(mStartParent, mStartOffset, mEndParent, mEndOffset, mRoot);

  *aReturn = range.forget().get();

  return NS_OK;
}

NS_IMETHODIMP nsRange::CloneRange(nsIDOMRange** aReturn)
{
  nsIRange* clone;
  nsresult rv = DoCloneRange(&clone);
  if (NS_SUCCEEDED(rv)) {
    *aReturn = clone;
  }
  return rv;
}

 nsresult
nsRange::CloneRange(nsIRange** aReturn) const
{
  return DoCloneRange(aReturn);
}

NS_IMETHODIMP
nsRange::InsertNode(nsIDOMNode* aN)
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
    NS_ENSURE_STATE(tSCParentNode);

    PRInt32 tEndOffset;
    GetEndOffset(&tEndOffset);

    nsCOMPtr<nsIDOMNode> tEndContainer;
    res = this->GetEndContainer(getter_AddRefs(tEndContainer));
    if(NS_FAILED(res)) return res;

    nsCOMPtr<nsIDOMText> secondPart;
    res = startTextNode->SplitText(tStartOffset, getter_AddRefs(secondPart));
    if (NS_FAILED(res)) return res;

    nsCOMPtr<nsIDOMNode> tResultNode;
    res = tSCParentNode->InsertBefore(aN, secondPart, getter_AddRefs(tResultNode));
    if (NS_FAILED(res)) return res;

    if (tEndContainer == tStartContainer && tEndOffset != tStartOffset)
      res = SetEnd(secondPart, tEndOffset - tStartOffset);

    return res;
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

NS_IMETHODIMP
nsRange::SurroundContents(nsIDOMNode* aNewParent)
{
  VALIDATE_ACCESS(aNewParent);

  NS_ENSURE_TRUE(mRoot, NS_ERROR_DOM_INVALID_STATE_ERR);
  
  
  if (mStartParent != mEndParent) {
    bool startIsText = mStartParent->IsNodeOfType(nsINode::eTEXT);
    bool endIsText = mEndParent->IsNodeOfType(nsINode::eTEXT);
    nsINode* startGrandParent = mStartParent->GetNodeParent();
    nsINode* endGrandParent = mEndParent->GetNodeParent();
    NS_ENSURE_TRUE((startIsText && endIsText &&
                    startGrandParent &&
                    startGrandParent == endGrandParent) ||
                   (startIsText &&
                    startGrandParent &&
                    startGrandParent == mEndParent) ||
                   (endIsText &&
                    endGrandParent &&
                    endGrandParent == mStartParent),
                   NS_ERROR_DOM_RANGE_BAD_BOUNDARYPOINTS_ERR);
  }

  

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

NS_IMETHODIMP
nsRange::ToString(nsAString& aReturn)
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
  nsresult rv = NS_NewContentIterator(getter_AddRefs(iter));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = iter->Init(static_cast<nsIRange*>(this));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString tempString;
 
  
  
  while (!iter->IsDone())
  {
    nsINode *n = iter->GetCurrentNode();

#ifdef DEBUG_range
    
    n->List(stdout);
#endif 
    nsCOMPtr<nsIDOMText> textNode(do_QueryInterface(n));
    if (textNode) 
    {
      if (n == mStartParent) 
      {
        PRUint32 strLength;
        textNode->GetLength(&strLength);
        textNode->SubstringData(mStartOffset,strLength-mStartOffset,tempString);
        aReturn += tempString;
      }
      else if (n == mEndParent)  
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



NS_IMETHODIMP
nsRange::Detach()
{
  if(mIsDetached)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  mIsDetached = true;

  DoSetRange(nsnull, 0, nsnull, 0, nsnull);
  
  return NS_OK;
}


NS_IMETHODIMP    
nsRange::CreateContextualFragment(const nsAString& aFragment,
                                  nsIDOMDocumentFragment** aReturn)
{
  if (mIsPositioned) {
    return nsContentUtils::CreateContextualFragment(mStartParent, aFragment,
                                                    false, aReturn);
  }
  return NS_ERROR_FAILURE;
}

static void ExtractRectFromOffset(nsIFrame* aFrame,
                                  const nsIFrame* aRelativeTo, 
                                  const PRInt32 aOffset, nsRect* aR, bool aKeepLeft)
{
  nsPoint point;
  aFrame->GetPointFromOffset(aOffset, &point);

  point += aFrame->GetOffsetTo(aRelativeTo);

  
  
  NS_ASSERTION(aR->x <= point.x && point.x <= aR->XMost(),
                   "point.x should not be outside of rect r");

  if (aKeepLeft) {
    aR->width = point.x - aR->x;
  } else {
    aR->width = aR->XMost() - point.x;
    aR->x = point.x;
  }
}

static nsresult GetPartialTextRect(nsLayoutUtils::RectCallback* aCallback,
                                   nsIContent* aContent, PRInt32 aStartOffset, PRInt32 aEndOffset)
{
  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame && frame->GetType() == nsGkAtoms::textFrame) {
    nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
    nsIFrame* relativeTo = nsLayoutUtils::GetContainingBlockForClientRect(textFrame);
    for (nsTextFrame* f = textFrame; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
      PRInt32 fstart = f->GetContentOffset(), fend = f->GetContentEnd();
      if (fend <= aStartOffset || fstart >= aEndOffset)
        continue;

      
      f->EnsureTextRun();
      NS_ENSURE_TRUE(f->GetTextRun(), NS_ERROR_OUT_OF_MEMORY);
      bool rtl = f->GetTextRun()->IsRightToLeft();
      nsRect r(f->GetOffsetTo(relativeTo), f->GetSize());
      if (fstart < aStartOffset) {
        
        ExtractRectFromOffset(f, relativeTo, aStartOffset, &r, rtl);
      }
      if (fend > aEndOffset) {
        
        ExtractRectFromOffset(f, relativeTo, aEndOffset, &r, !rtl);
      }
      aCallback->AddRect(r);
    }
  }
  return NS_OK;
}

static void CollectClientRects(nsLayoutUtils::RectCallback* aCollector, 
                               nsRange* aRange,
                               nsINode* aStartParent, PRInt32 aStartOffset,
                               nsINode* aEndParent, PRInt32 aEndOffset)
{
  
  nsCOMPtr<nsIDOMNode> startContainer = do_QueryInterface(aStartParent);
  nsCOMPtr<nsIDOMNode> endContainer = do_QueryInterface(aEndParent);

  
  if (!aStartParent->IsInDoc()) {
    return;
  }

  aStartParent->GetCurrentDoc()->FlushPendingNotifications(Flush_Layout);

  
  if (!aStartParent->IsInDoc()) {
    return;
  }

  RangeSubtreeIterator iter;

  nsresult rv = iter.Init(aRange);
  if (NS_FAILED(rv)) return;

  if (iter.IsDone()) {
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(aStartParent);
    if (content && content->IsNodeOfType(nsINode::eTEXT)) {
      nsIFrame* frame = content->GetPrimaryFrame();
      if (frame && frame->GetType() == nsGkAtoms::textFrame) {
        nsTextFrame* textFrame = static_cast<nsTextFrame*>(frame);
        PRInt32 outOffset;
        nsIFrame* outFrame;
        textFrame->GetChildFrameContainingOffset(aStartOffset, false, 
          &outOffset, &outFrame);
        if (outFrame) {
           nsIFrame* relativeTo = 
             nsLayoutUtils::GetContainingBlockForClientRect(outFrame);
           nsRect r(outFrame->GetOffsetTo(relativeTo), outFrame->GetSize());
           ExtractRectFromOffset(outFrame, relativeTo, aStartOffset, &r, false);
           r.width = 0;
           aCollector->AddRect(r);
        }
      }
    }
    return;
  }

  do {
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());
    iter.Next();
    nsCOMPtr<nsIContent> content = do_QueryInterface(node);
    if (!content)
      continue;
    if (content->IsNodeOfType(nsINode::eTEXT)) {
       if (node == startContainer) {
         PRInt32 offset = startContainer == endContainer ? 
           aEndOffset : content->GetText()->GetLength();
         GetPartialTextRect(aCollector, content, aStartOffset, offset);
         continue;
       } else if (node == endContainer) {
         GetPartialTextRect(aCollector, content, 0, aEndOffset);
         continue;
       }
    }

    nsIFrame* frame = content->GetPrimaryFrame();
    if (frame) {
      nsLayoutUtils::GetAllInFlowRects(frame,
        nsLayoutUtils::GetContainingBlockForClientRect(frame), aCollector);
    }
  } while (!iter.IsDone());
}

NS_IMETHODIMP
nsRange::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  *aResult = nsnull;

  
  nsClientRect* rect = new nsClientRect();
  if (!rect)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = rect);

  if (!mStartParent)
    return NS_OK;

  nsLayoutUtils::RectAccumulator accumulator;
  
  CollectClientRects(&accumulator, this, mStartParent, mStartOffset, 
    mEndParent, mEndOffset);

  nsRect r = accumulator.mResultRect.IsEmpty() ? accumulator.mFirstRect : 
    accumulator.mResultRect;
  rect->SetLayoutRect(r);
  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetClientRects(nsIDOMClientRectList** aResult)
{
  *aResult = nsnull;

  if (!mStartParent)
    return NS_OK;

  nsRefPtr<nsClientRectList> rectList = new nsClientRectList();
  if (!rectList)
    return NS_ERROR_OUT_OF_MEMORY;

  nsLayoutUtils::RectListBuilder builder(rectList);

  CollectClientRects(&builder, this, mStartParent, mStartOffset, 
    mEndParent, mEndOffset);

  if (NS_FAILED(builder.mRV))
    return builder.mRV;
  rectList.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsRange::GetUsedFontFaces(nsIDOMFontFaceList** aResult)
{
  *aResult = nsnull;

  NS_ENSURE_TRUE(mStartParent, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMNode> startContainer = do_QueryInterface(mStartParent);
  nsCOMPtr<nsIDOMNode> endContainer = do_QueryInterface(mEndParent);

  
  nsIDocument* doc = mStartParent->OwnerDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);
  doc->FlushPendingNotifications(Flush_Frames);

  
  NS_ENSURE_TRUE(mStartParent->IsInDoc(), NS_ERROR_UNEXPECTED);

  nsRefPtr<nsFontFaceList> fontFaceList = new nsFontFaceList();

  RangeSubtreeIterator iter;
  nsresult rv = iter.Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  while (!iter.IsDone()) {
    
    nsCOMPtr<nsIDOMNode> node(iter.GetCurrentNode());
    iter.Next();

    nsCOMPtr<nsIContent> content = do_QueryInterface(node);
    if (!content) {
      continue;
    }
    nsIFrame* frame = content->GetPrimaryFrame();
    if (!frame) {
      continue;
    }

    if (content->IsNodeOfType(nsINode::eTEXT)) {
       if (node == startContainer) {
         PRInt32 offset = startContainer == endContainer ? 
           mEndOffset : content->GetText()->GetLength();
         nsLayoutUtils::GetFontFacesForText(frame, mStartOffset, offset,
                                            true, fontFaceList);
         continue;
       }
       if (node == endContainer) {
         nsLayoutUtils::GetFontFacesForText(frame, 0, mEndOffset,
                                            true, fontFaceList);
         continue;
       }
    }

    nsLayoutUtils::GetFontFacesForFrames(frame, fontFaceList);
  }

  fontFaceList.forget(aResult);
  return NS_OK;
}
