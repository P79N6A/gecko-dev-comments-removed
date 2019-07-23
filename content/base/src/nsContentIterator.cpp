





































#include "nsISupports.h"
#include "nsIDOMNodeList.h"
#include "nsIContentIterator.h"
#include "nsRange.h"
#include "nsIContent.h"
#include "nsIDOMText.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIComponentManager.h"
#include "nsContentCID.h"
#include "nsLayoutCID.h"
#include "nsVoidArray.h"
#include "nsContentUtils.h"
#include "nsINode.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);






static PRUint32
GetNumChildren(nsIDOMNode *aNode) 
{
  if (!aNode)
    return 0;

  PRUint32 numChildren = 0;
  PRBool hasChildNodes;
  aNode->HasChildNodes(&hasChildNodes);
  if (hasChildNodes)
  {
    nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));

    if (content)
      return content->GetChildCount();

    nsCOMPtr<nsIDOMNodeList>nodeList;
    aNode->GetChildNodes(getter_AddRefs(nodeList));
    if (nodeList) 
      nodeList->GetLength(&numChildren);
  }

  return numChildren;
}




static nsCOMPtr<nsIDOMNode> 
GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset)
{
  nsCOMPtr<nsIDOMNode> resultNode;

  if (!aParent) 
    return resultNode;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aParent));

  if (content) {
    resultNode = do_QueryInterface(content->GetChildAt(aOffset));
  } else if (aParent) {
    PRBool hasChildNodes;
    aParent->HasChildNodes(&hasChildNodes);
    if (hasChildNodes)
    {
      nsCOMPtr<nsIDOMNodeList>nodeList;
      aParent->GetChildNodes(getter_AddRefs(nodeList));
      if (nodeList) 
        nodeList->Item(aOffset, getter_AddRefs(resultNode));
    }
  }
  
  return resultNode;
}
  



static inline PRBool
NodeHasChildren(nsINode *aNode)
{
  return aNode->GetChildCount() > 0;
}





static void
ContentToParentOffset(nsIContent *aContent, nsIDOMNode **aParent,
                      PRInt32 *aOffset)
{
  *aParent = nsnull;
  *aOffset  = 0;

  nsIContent* parent = aContent->GetParent();

  if (!parent)
    return;

  *aOffset = parent->IndexOf(aContent);

  CallQueryInterface(parent, aParent);
}





static PRBool
ContentIsInTraversalRange(nsIContent *aContent, PRBool aIsPreMode,
                          nsINode *aStartNode, PRInt32 aStartOffset,
                          nsINode *aEndNode, PRInt32 aEndOffset)
{
  if (!aStartNode || !aEndNode || !aContent)
    return PR_FALSE;

  
  
  if (aContent->IsNodeOfType(nsINode::eDATA_NODE) &&
      (aContent == aStartNode || aContent == aEndNode)) {
    return PR_TRUE;
  }

  nsIContent* parent = aContent->GetParent();
  if (!parent)
    return PR_FALSE;

  PRInt32 indx = parent->IndexOf(aContent);

  if (!aIsPreMode)
    ++indx;

  return (nsContentUtils::ComparePoints(aStartNode, aStartOffset,
                                        parent, indx) <= 0) &&
         (nsContentUtils::ComparePoints(aEndNode, aEndOffset,
                                        parent, indx) >= 0);
}






class nsContentIterator : public nsIContentIterator 
{
public:
  NS_DECL_ISUPPORTS

  nsContentIterator();
  virtual ~nsContentIterator();

  

  virtual nsresult Init(nsIContent* aRoot);

  virtual nsresult Init(nsIDOMRange* aRange);

  virtual void First();

  virtual void Last();
  
  virtual void Next();

  virtual void Prev();

  virtual nsIContent *GetCurrentNode();

  virtual PRBool IsDone();

  virtual nsresult PositionAt(nsIContent* aCurNode);

  
  
  

protected:

  nsIContent *GetDeepFirstChild(nsIContent *aRoot, nsVoidArray *aIndexes);
  nsIContent *GetDeepLastChild(nsIContent *aRoot, nsVoidArray *aIndexes);

  
  
  nsIContent *GetNextSibling(nsINode *aNode, nsVoidArray *aIndexes);

  
  
  nsIContent *GetPrevSibling(nsINode *aNode, nsVoidArray *aIndexes);

  nsIContent *NextNode(nsIContent *aNode, nsVoidArray *aIndexes);
  nsIContent *PrevNode(nsIContent *aNode, nsVoidArray *aIndexes);

  
  nsresult RebuildIndexStack();

  void MakeEmpty();
  
  nsCOMPtr<nsIContent> mCurNode;
  nsCOMPtr<nsIContent> mFirst;
  nsCOMPtr<nsIContent> mLast;
  nsCOMPtr<nsINode> mCommonParent;

  
  nsAutoVoidArray mIndexes;

  
  
  
  
  
  PRInt32 mCachedIndex;
  
  
  
  
  
  
  
  
  
  
  
  PRBool mIsDone;
  PRBool mPre;
  
private:

  
  nsContentIterator(const nsContentIterator&);
  nsContentIterator& operator=(const nsContentIterator&);

};






class nsPreContentIterator : public nsContentIterator
{
public:
  nsPreContentIterator() { mPre = PR_TRUE; }
};







nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult)
{
  nsContentIterator * iter = new nsContentIterator();
  if (!iter) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = iter);

  return NS_OK;
}


nsresult NS_NewPreContentIterator(nsIContentIterator** aInstancePtrResult)
{
  nsContentIterator * iter = new nsPreContentIterator();
  if (!iter) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = iter);

  return NS_OK;
}





 
NS_IMPL_ISUPPORTS1(nsContentIterator, nsIContentIterator)






nsContentIterator::nsContentIterator() :
  
  mCachedIndex(0), mIsDone(PR_FALSE), mPre(PR_FALSE)
{
}


nsContentIterator::~nsContentIterator()
{
}







nsresult
nsContentIterator::Init(nsIContent* aRoot)
{
  if (!aRoot) 
    return NS_ERROR_NULL_POINTER; 
  mIsDone = PR_FALSE;
  mIndexes.Clear();
  
  if (mPre)
  {
    mFirst = aRoot;
    mLast  = GetDeepLastChild(aRoot, nsnull);
  }
  else
  {
    mFirst = GetDeepFirstChild(aRoot, nsnull); 
    mLast  = aRoot;
  }

  mCommonParent = aRoot;
  mCurNode = mFirst;
  RebuildIndexStack();
  return NS_OK;
}


nsresult
nsContentIterator::Init(nsIDOMRange* aRange)
{
  nsCOMPtr<nsIRange> range = do_QueryInterface(aRange);
  NS_ENSURE_TRUE(range, NS_ERROR_NULL_POINTER);

  mIsDone = PR_FALSE;

  
  mCommonParent = range->GetCommonAncestor();
  NS_ENSURE_TRUE(mCommonParent, NS_ERROR_FAILURE);

  
  PRInt32 startIndx = range->StartOffset();
  nsINode* startNode = range->GetStartParent();
  NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);

  
  PRInt32 endIndx = range->EndOffset();
  nsINode* endNode = range->GetEndParent();
  NS_ENSURE_TRUE(endNode, NS_ERROR_FAILURE);

  PRBool startIsData = startNode->IsNodeOfType(nsINode::eDATA_NODE);

  
  if (startNode == endNode)
  {
    
    
    
    
    
    

    if (!startIsData && startIndx == endIndx)
    {
      MakeEmpty();
      return NS_OK;
    }

    if (startIsData)
    {
      
      NS_ASSERTION(startNode->IsNodeOfType(nsINode::eCONTENT),
                   "Data node that's not content?");

      mFirst   = NS_STATIC_CAST(nsIContent*, startNode);
      mLast    = mFirst;
      mCurNode = mFirst;

      RebuildIndexStack();
      return NS_OK;
    }
  }
  
  

  nsIContent *cChild = nsnull;

  if (!startIsData && NodeHasChildren(startNode))
    cChild = startNode->GetChildAt(startIndx);

  if (!cChild) 
  {
    
    
    if (mPre)
    {
      
      
      

      if (!startIsData)
      {
        mFirst = GetNextSibling(startNode, nsnull);

        
        
        
  
        if (mFirst &&
            !ContentIsInTraversalRange(mFirst, mPre, startNode, startIndx,
                                       endNode, endIndx)) {
          mFirst = nsnull;
        }
      }
      else {
        NS_ASSERTION(startNode->IsNodeOfType(nsINode::eCONTENT),
                   "Data node that's not content?");

        mFirst = NS_STATIC_CAST(nsIContent*, startNode);
      }
    }
    else {
      
      if (startNode->IsNodeOfType(nsINode::eCONTENT)) {
        mFirst = NS_STATIC_CAST(nsIContent*, startNode);
      } else {
        
        mFirst = nsnull;
      }
    }
  }
  else
  {
    if (mPre)
      mFirst = cChild;
    else 
    {
      mFirst = GetDeepFirstChild(cChild, nsnull);

      
      
      
  
      if (mFirst &&
          !ContentIsInTraversalRange(mFirst, mPre, startNode, startIndx,
                                     endNode, endIndx))
        mFirst = nsnull;
    }
  }


  

  PRBool endIsData = endNode->IsNodeOfType(nsINode::eDATA_NODE);

  if (endIsData || !NodeHasChildren(endNode) || endIndx == 0)
  {
    if (mPre) {
      if (endNode->IsNodeOfType(nsINode::eCONTENT)) {
        mLast = NS_STATIC_CAST(nsIContent*, endNode);
      } else {
        
        mLast = nsnull;
      }
    }
    else 
    {
      
      
      

      if (!endIsData)
      {
        mLast = GetPrevSibling(endNode, nsnull);

        if (!ContentIsInTraversalRange(mLast, mPre, startNode, startIndx,
                                       endNode, endIndx))
          mLast = nsnull;
      }
      else {
        NS_ASSERTION(endNode->IsNodeOfType(nsINode::eCONTENT),
                     "Data node that's not content?");

        mLast = NS_STATIC_CAST(nsIContent*, endNode);
      }
    }
  }
  else
  {
    PRInt32 indx = endIndx;

    cChild = endNode->GetChildAt(--indx);

    if (!cChild)  
    {
      NS_NOTREACHED("nsContentIterator::nsContentIterator");
      return NS_ERROR_FAILURE; 
    }

    if (mPre)
    {
      mLast  = GetDeepLastChild(cChild, nsnull);

      if (!ContentIsInTraversalRange(mLast, mPre, startNode, startIndx,
                                     endNode, endIndx)) {
        mLast = nsnull;
      }
    }
    else { 
      mLast = cChild;
    }
  }

  
  

  if (!mFirst || !mLast)
  {
    mFirst = nsnull;
    mLast  = nsnull;
  }
  
  mCurNode = mFirst;
  mIsDone  = !mCurNode;

  if (!mCurNode)
    mIndexes.Clear();
  else
    RebuildIndexStack();

  return NS_OK;
}






nsresult nsContentIterator::RebuildIndexStack()
{
  
  
  
  nsIContent* parent;
  nsIContent* current;

  mIndexes.Clear();
  current = mCurNode;
  if (!current) {
    return NS_OK;
  }

  while (current != mCommonParent)
  {
    parent = current->GetParent();
    
    if (!parent)
      return NS_ERROR_FAILURE;
  
    mIndexes.InsertElementAt(NS_INT32_TO_PTR(parent->IndexOf(current)), 0);

    current = parent;
  }
  return NS_OK;
}

void
nsContentIterator::MakeEmpty()
{
  mCurNode      = nsnull;
  mFirst        = nsnull;
  mLast         = nsnull;
  mCommonParent = nsnull;
  mIsDone       = PR_TRUE;
  mIndexes.Clear();
}

nsIContent *
nsContentIterator::GetDeepFirstChild(nsIContent *aRoot, nsVoidArray *aIndexes)
{
  if (!aRoot) {
    return nsnull;
  }

  nsIContent *cN = aRoot;
  nsIContent *cChild = cN->GetChildAt(0);

  while (cChild)
  {
    if (aIndexes)
    {
      
      aIndexes->AppendElement(NS_INT32_TO_PTR(0));
    }
    cN = cChild;
    cChild = cN->GetChildAt(0);
  }

  return cN;
}

nsIContent *
nsContentIterator::GetDeepLastChild(nsIContent *aRoot, nsVoidArray *aIndexes)
{
  if (!aRoot) {
    return nsnull;
  }

  nsIContent *deepLastChild = aRoot;

  nsIContent *cN = aRoot;
  PRInt32 numChildren = cN->GetChildCount();

  while (numChildren)
  {
    nsIContent *cChild = cN->GetChildAt(--numChildren);

    if (aIndexes)
    {
      
      aIndexes->AppendElement(NS_INT32_TO_PTR(numChildren));
    }
    numChildren = cChild->GetChildCount();
    cN = cChild;

    deepLastChild = cN;
  }

  return deepLastChild;
}


nsIContent *
nsContentIterator::GetNextSibling(nsINode *aNode, 
                                  nsVoidArray *aIndexes)
{
  if (!aNode) 
    return nsnull;

  nsINode *parent = aNode->GetNodeParent();
  if (!parent)
    return nsnull;

  PRInt32 indx;

  if (aIndexes)
  {
    NS_ASSERTION(aIndexes->Count() > 0, "ContentIterator stack underflow");
    
    indx = NS_PTR_TO_INT32((*aIndexes)[aIndexes->Count()-1]);
  }
  else
    indx = mCachedIndex;

  
  
  
  nsIContent *sib = parent->GetChildAt(indx);
  if (sib != aNode)
  {
    
    indx = parent->IndexOf(aNode);
  }

  
  if ((sib = parent->GetChildAt(++indx)))
  {
    
    if (aIndexes)
    {
      aIndexes->ReplaceElementAt(NS_INT32_TO_PTR(indx),aIndexes->Count()-1);
    }
    else mCachedIndex = indx;
  }
  else
  {
    if (parent != mCommonParent)
    {
      if (aIndexes)
      {
        
        
        
        if (aIndexes->Count() > 1)
          aIndexes->RemoveElementAt(aIndexes->Count()-1);
      }
    }

    
    sib = GetNextSibling(parent, aIndexes);
  }
  
  return sib;
}


nsIContent *
nsContentIterator::GetPrevSibling(nsINode *aNode, 
                                  nsVoidArray *aIndexes)
{
  if (!aNode)
    return nsnull;

  nsINode *parent = aNode->GetNodeParent();
  if (!parent)
    return nsnull;

  PRInt32 indx;

  if (aIndexes)
  {
    NS_ASSERTION(aIndexes->Count() > 0, "ContentIterator stack underflow");
    
    indx = NS_PTR_TO_INT32((*aIndexes)[aIndexes->Count()-1]);
  }
  else
    indx = mCachedIndex;

  
  
  nsIContent *sib = parent->GetChildAt(indx);
  if (sib != aNode)
  {
    
    indx = parent->IndexOf(aNode);
  }

  
  if (indx > 0 && (sib = parent->GetChildAt(--indx)))
  {
    
    if (aIndexes)
    {
      aIndexes->ReplaceElementAt(NS_INT32_TO_PTR(indx),aIndexes->Count()-1);
    }
    else mCachedIndex = indx;
  }
  else if (parent != mCommonParent)
  {
    if (aIndexes)
    {
      
      aIndexes->RemoveElementAt(aIndexes->Count()-1);
    }
    return GetPrevSibling(parent, aIndexes);
  }

  return sib;
}

nsIContent *
nsContentIterator::NextNode(nsIContent *aNode, nsVoidArray *aIndexes)
{
  nsIContent *cN = aNode;
  nsIContent *nextNode = nsnull;

  if (mPre)  
  {
    
    if (NodeHasChildren(cN))
    {
      nsIContent *cFirstChild = cN->GetChildAt(0);

      
      if (aIndexes)
      {
        
        aIndexes->AppendElement(NS_INT32_TO_PTR(0));
      }
      else mCachedIndex = 0;
      
      return cFirstChild;
    }

    
    nextNode = GetNextSibling(cN, aIndexes);
  }
  else  
  {
    nsIContent *parent = cN->GetParent();
    nsIContent *cSibling = nsnull;
    PRInt32 indx;

    
    if (aIndexes)
    {
      NS_ASSERTION(aIndexes->Count() > 0, "ContentIterator stack underflow");
      
      indx = NS_PTR_TO_INT32((*aIndexes)[aIndexes->Count()-1]);
    }
    else indx = mCachedIndex;

    
    
    
    if (indx >= 0)
      cSibling = parent->GetChildAt(indx);
    if (cSibling != cN)
    {
      
      indx = parent->IndexOf(cN);
    }

    
    cSibling = parent->GetChildAt(++indx);
    if (cSibling)
    {
      
      if (aIndexes)
      {
        
        aIndexes->ReplaceElementAt(NS_INT32_TO_PTR(indx),aIndexes->Count()-1);
      }
      else mCachedIndex = indx;
      
      
      return GetDeepFirstChild(cSibling, aIndexes); 
    }
  
    
    
    if (aIndexes)
    {
      
      
      
      if (aIndexes->Count() > 1)
        aIndexes->RemoveElementAt(aIndexes->Count()-1);
    }
    else mCachedIndex = 0;   
    nextNode = parent;
  }

  return nextNode;
}

nsIContent *
nsContentIterator::PrevNode(nsIContent *aNode, nsVoidArray *aIndexes)
{
  nsIContent *prevNode = nsnull;
  nsIContent *cN = aNode;
   
  if (mPre)  
  {
    nsIContent *parent = cN->GetParent();
    nsIContent *cSibling = nsnull;
    PRInt32 indx;

    
    if (aIndexes)
    {
      NS_ASSERTION(aIndexes->Count() > 0, "ContentIterator stack underflow");
      
      indx = NS_PTR_TO_INT32((*aIndexes)[aIndexes->Count()-1]);
    }
    else indx = mCachedIndex;

    
    
    
    if (indx >= 0)
      cSibling = parent->GetChildAt(indx);

    if (cSibling != cN)
    {
      
      indx = parent->IndexOf(cN);
    }

    
    if (indx && (cSibling = parent->GetChildAt(--indx)))
    {
      
      if (aIndexes)
      {
        
        aIndexes->ReplaceElementAt(NS_INT32_TO_PTR(indx),aIndexes->Count()-1);
      }
      else mCachedIndex = indx;
      
      
      return GetDeepLastChild(cSibling, aIndexes); 
    }
  
    
    
    if (aIndexes)
    {
      
      aIndexes->RemoveElementAt(aIndexes->Count()-1);
    }
    else mCachedIndex = 0;   
    prevNode = parent;
  }
  else  
  {
    PRInt32 numChildren = cN->GetChildCount();
  
    
    if (numChildren)
    {
      nsIContent *cLastChild = cN->GetChildAt(--numChildren);

      
      if (aIndexes)
      {
        
        aIndexes->AppendElement(NS_INT32_TO_PTR(numChildren));
      }
      else mCachedIndex = numChildren;
      
      return cLastChild;
    }

    
    prevNode = GetPrevSibling(cN, aIndexes);
  }

  return prevNode;
}





void
nsContentIterator::First()
{
  NS_ASSERTION(mFirst, "No first node!");

  if (mFirst) {
#ifdef DEBUG
    nsresult rv =
#endif
    PositionAt(mFirst);

    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to position iterator!");
  }

  mIsDone = mFirst == nsnull;
}


void
nsContentIterator::Last()
{
  NS_ASSERTION(mLast, "No last node!");

  if (mLast) {
#ifdef DEBUG
    nsresult rv =
#endif
    PositionAt(mLast);

    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to position iterator!");
  }

  mIsDone = mLast == nsnull;
}


void
nsContentIterator::Next()
{
  if (mIsDone || !mCurNode) 
    return;

  if (mCurNode == mLast) 
  {
    mIsDone = PR_TRUE;
    return;
  }

  mCurNode = NextNode(mCurNode, &mIndexes);
}


void
nsContentIterator::Prev()
{
  if (mIsDone || !mCurNode) 
    return;

  if (mCurNode == mFirst) 
  {
    mIsDone = PR_TRUE;
    return;
  }

  mCurNode = PrevNode(mCurNode, &mIndexes);
}


PRBool
nsContentIterator::IsDone()
{
  return mIsDone;
}




nsresult
nsContentIterator::PositionAt(nsIContent* aCurNode)
{
  if (!aCurNode)
    return NS_ERROR_NULL_POINTER;

  nsIContent *newCurNode = aCurNode;
  nsIContent *tempNode = mCurNode;

  mCurNode = aCurNode;
  
  if (mCurNode == tempNode)
  {
    mIsDone = PR_FALSE;  
    return NS_OK;
  }

  

  nsCOMPtr<nsIDOMNode> firstNode(do_QueryInterface(mFirst));
  nsCOMPtr<nsIDOMNode> lastNode(do_QueryInterface(mLast));
  PRInt32 firstOffset=0, lastOffset=0;

  if (firstNode && lastNode)
  {
    PRUint32 numChildren;

    if (mPre)
    {
      ContentToParentOffset(mFirst, getter_AddRefs(firstNode), &firstOffset);

      numChildren = GetNumChildren(lastNode);

      if (numChildren)
        lastOffset = 0;
      else
      {
        ContentToParentOffset(mLast, getter_AddRefs(lastNode), &lastOffset);
        ++lastOffset;
      }
    }
    else
    {
      numChildren = GetNumChildren(firstNode);

      if (numChildren)
        firstOffset = numChildren;
      else
        ContentToParentOffset(mFirst, getter_AddRefs(firstNode), &firstOffset);

      ContentToParentOffset(mLast, getter_AddRefs(lastNode), &lastOffset);
      ++lastOffset;
    }
  }

  if (!firstNode || !lastNode ||
      !ContentIsInTraversalRange(mCurNode, mPre, mFirst, firstOffset,
                                 mLast, lastOffset))
  {
    mIsDone = PR_TRUE;
    return NS_ERROR_FAILURE;
  }

  
  
  nsAutoVoidArray      oldParentStack;
  nsAutoVoidArray      newIndexes;

  
  
  
  
  
  

  
  
  if (!oldParentStack.SizeTo(mIndexes.Count()+1))
    return NS_ERROR_FAILURE;

  
  
  
  
  for (PRInt32 i = mIndexes.Count()+1; i > 0 && tempNode; i--)
  {
    
    oldParentStack.InsertElementAt(tempNode,0);

    nsIContent *parent = tempNode->GetParent();

    if (!parent)  
      break;

    if (parent == mCurNode)
    {
      
      
      mIndexes.RemoveElementsAt(mIndexes.Count() - oldParentStack.Count(),
                                oldParentStack.Count());
      mIsDone = PR_FALSE;
      return NS_OK;
    }
    tempNode = parent;
  }

  
  while (newCurNode)
  {
    nsIContent *parent = newCurNode->GetParent();

    if (!parent)  
      break;

    PRInt32 indx = parent->IndexOf(newCurNode);

    
    newIndexes.InsertElementAt(NS_INT32_TO_PTR(indx),0);

    
    indx = oldParentStack.IndexOf(parent);
    if (indx >= 0)
    {
      
      
      
      
      
      PRInt32 numToDrop = oldParentStack.Count()-(1+indx);
      if (numToDrop > 0)
        mIndexes.RemoveElementsAt(mIndexes.Count() - numToDrop,numToDrop);
      mIndexes.AppendElements(newIndexes);

      break;
    }
    newCurNode = parent;
  }

  

  mIsDone = PR_FALSE;
  return NS_OK;
}


nsIContent *
nsContentIterator::GetCurrentNode()
{
  if (mIsDone) {
    return nsnull;
  }

  NS_ASSERTION(mCurNode, "Null current node in an iterator that's not done!");

  return mCurNode;
}





















class nsContentSubtreeIterator : public nsContentIterator 
{
public:
  nsContentSubtreeIterator() {}
  virtual ~nsContentSubtreeIterator() {}

  

  virtual nsresult Init(nsIContent* aRoot);

  virtual nsresult Init(nsIDOMRange* aRange);

  virtual void Next();

  virtual void Prev();

  virtual nsresult PositionAt(nsIContent* aCurNode);

  
  virtual void First();

  
  virtual void Last();

protected:

  nsresult GetTopAncestorInRange(nsIContent *aNode,
                                 nsCOMPtr<nsIContent> *outAnestor);

  
  nsContentSubtreeIterator(const nsContentSubtreeIterator&);
  nsContentSubtreeIterator& operator=(const nsContentSubtreeIterator&);

  nsCOMPtr<nsIDOMRange> mRange;
  
#if 0
  nsAutoVoidArray mStartNodes;
  nsAutoVoidArray mStartOffsets;
#endif

  nsAutoVoidArray mEndNodes;
  nsAutoVoidArray mEndOffsets;
};

nsresult NS_NewContentSubtreeIterator(nsIContentIterator** aInstancePtrResult);








nsresult NS_NewContentSubtreeIterator(nsIContentIterator** aInstancePtrResult)
{
  nsContentIterator * iter = new nsContentSubtreeIterator();
  if (!iter) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = iter);

  return NS_OK;
}








nsresult nsContentSubtreeIterator::Init(nsIContent* aRoot)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult nsContentSubtreeIterator::Init(nsIDOMRange* aRange)
{
  if (!aRange) 
    return NS_ERROR_NULL_POINTER; 

  mIsDone = PR_FALSE;

  mRange = aRange;
  
  
  nsCOMPtr<nsIDOMNode> commonParent;
  nsCOMPtr<nsIDOMNode> startParent;
  nsCOMPtr<nsIDOMNode> endParent;
  nsCOMPtr<nsIContent> cStartP;
  nsCOMPtr<nsIContent> cEndP;
  nsCOMPtr<nsIContent> cN;
  nsIContent *firstCandidate = nsnull;
  nsIContent *lastCandidate = nsnull;
  nsCOMPtr<nsIDOMNode> dChild;
  nsCOMPtr<nsIContent> cChild;
  PRInt32 indx, startIndx, endIndx;
  PRInt32 numChildren;

  
  if (NS_FAILED(aRange->GetCommonAncestorContainer(getter_AddRefs(commonParent))) || !commonParent)
    return NS_ERROR_FAILURE;
  mCommonParent = do_QueryInterface(commonParent);

  
  if (NS_FAILED(aRange->GetStartContainer(getter_AddRefs(startParent))) || !startParent)
    return NS_ERROR_FAILURE;
  cStartP = do_QueryInterface(startParent);
  aRange->GetStartOffset(&startIndx);

  
  if (NS_FAILED(aRange->GetEndContainer(getter_AddRefs(endParent))) || !endParent)
    return NS_ERROR_FAILURE;
  cEndP = do_QueryInterface(endParent);
  aRange->GetEndOffset(&endIndx);

  if (!cStartP || !cEndP)
  {
    
    
    return NS_ERROR_FAILURE;
  }
  
  
  if (startParent == endParent)
  {
    cChild = cStartP->GetChildAt(0);
  
    if (!cChild) 
    {
      
      MakeEmpty();
      return NS_OK;
    }
    else
    {
      if (startIndx == endIndx)  
      {
        MakeEmpty();
        return NS_OK;
      }
    }
  }
  
  
#if 0
  nsContentUtils::GetAncestorsAndOffsets(startParent, startIndx,
                                         &mStartNodes, &mStartOffsets);
#endif
  nsContentUtils::GetAncestorsAndOffsets(endParent, endIndx,
                                         &mEndNodes, &mEndOffsets);

  
  aRange->GetStartOffset(&indx);
  numChildren = GetNumChildren(startParent);
  
  if (!numChildren) 
  {
    cN = cStartP; 
  }
  else
  {
    dChild = GetChildAt(startParent, indx);
    cChild = do_QueryInterface(dChild);
    if (!cChild)  
    {
      cN = cStartP;
    }
    else
    {
      firstCandidate = cChild;
    }
  }
  
  if (!firstCandidate)
  {
    
    firstCandidate = GetNextSibling(cN, nsnull);

    if (!firstCandidate)
    {
      MakeEmpty();
      return NS_OK;
    }
  }
  
  firstCandidate = GetDeepFirstChild(firstCandidate, nsnull);
  
  
  
  
  
  PRBool nodeBefore, nodeAfter;
  if (NS_FAILED(nsRange::CompareNodeToRange(firstCandidate, aRange,
                                            &nodeBefore, &nodeAfter)))
    return NS_ERROR_FAILURE;

  if (nodeBefore || nodeAfter)
  {
    MakeEmpty();
    return NS_OK;
  }

  
  
  
  if (NS_FAILED(GetTopAncestorInRange(firstCandidate, address_of(mFirst))))
    return NS_ERROR_FAILURE;

  
  aRange->GetEndOffset(&indx);
  numChildren = GetNumChildren(endParent);

  if (indx > numChildren) indx = numChildren;
  if (!indx)
  {
    cN = cEndP;
  }
  else
  {
    if (!numChildren) 
    {
      cN = cEndP; 
    }
    else
    {
      dChild = GetChildAt(endParent, --indx);
      cChild = do_QueryInterface(dChild);
      if (!cChild)  
      {
        NS_ASSERTION(0,"tree traversal trouble in nsContentSubtreeIterator::Init");
        return NS_ERROR_FAILURE;
      }
      else
      {
        lastCandidate = cChild;
      }
    }
  }
  
  if (!lastCandidate)
  {
    
    lastCandidate = GetPrevSibling(cN, nsnull);
  }
  
  lastCandidate = GetDeepLastChild(lastCandidate, nsnull);
  
  
  
  
  
  if (NS_FAILED(nsRange::CompareNodeToRange(lastCandidate, aRange, &nodeBefore,
                                            &nodeAfter)))
    return NS_ERROR_FAILURE;

  if (nodeBefore || nodeAfter)
  {
    MakeEmpty();
    return NS_OK;
  }

  
  
  
  if (NS_FAILED(GetTopAncestorInRange(lastCandidate, address_of(mLast))))
    return NS_ERROR_FAILURE;
  
  mCurNode = mFirst;

  return NS_OK;
}







void
nsContentSubtreeIterator::First()
{
  mIsDone = mFirst == nsnull;

  mCurNode = mFirst;
}


void
nsContentSubtreeIterator::Last()
{
  mIsDone = mLast == nsnull;

  mCurNode = mLast;
}


void
nsContentSubtreeIterator::Next()
{
  if (mIsDone || !mCurNode) 
    return;

  if (mCurNode == mLast) 
  {
    mIsDone = PR_TRUE;
    return;
  }

  nsIContent *nextNode = GetNextSibling(mCurNode, nsnull);
  NS_ASSERTION(nextNode, "No next sibling!?! This could mean deadlock!");





  PRInt32 i = mEndNodes.IndexOf(nextNode);
  while (i != -1)
  {
    
    
    nextNode = nextNode->GetChildAt(0);
    NS_ASSERTION(nextNode, "Iterator error, expected a child node!");

    
    
    
    
    i = mEndNodes.IndexOf(nextNode);
  }

  mCurNode = nextNode;

  
  
  
  mIsDone = mCurNode == nsnull;

  return;
}


void
nsContentSubtreeIterator::Prev()
{
  
  
  if (mIsDone || !mCurNode) 
    return;

  if (mCurNode == mFirst) 
  {
    mIsDone = PR_TRUE;
    return;
  }

  nsIContent *prevNode = PrevNode(GetDeepFirstChild(mCurNode, nsnull), nsnull);

  prevNode = GetDeepLastChild(prevNode, nsnull);
  
  GetTopAncestorInRange(prevNode, address_of(mCurNode));

  
  
  
  mIsDone = mCurNode == nsnull;
}


nsresult
nsContentSubtreeIterator::PositionAt(nsIContent* aCurNode)
{
  NS_ERROR("Not implemented!");

  return NS_ERROR_NOT_IMPLEMENTED;
}





nsresult
nsContentSubtreeIterator::GetTopAncestorInRange(nsIContent *aNode,
                                                nsCOMPtr<nsIContent> *outAnestor)
{
  if (!aNode) 
    return NS_ERROR_NULL_POINTER;
  if (!outAnestor) 
    return NS_ERROR_NULL_POINTER;
  
  
  
  PRBool nodeBefore, nodeAfter;
  if (NS_FAILED(nsRange::CompareNodeToRange(aNode, mRange, &nodeBefore,
                                            &nodeAfter)))
    return NS_ERROR_FAILURE;

  if (nodeBefore || nodeAfter)
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIContent> parent, tmp;
  while (aNode)
  {
    parent = aNode->GetParent();
    if (!parent)
    {
      if (tmp)
      {
        *outAnestor = tmp;
        return NS_OK;
      }
      else return NS_ERROR_FAILURE;
    }
    if (NS_FAILED(nsRange::CompareNodeToRange(parent, mRange, &nodeBefore,
                                              &nodeAfter)))
      return NS_ERROR_FAILURE;

    if (nodeBefore || nodeAfter)
    {
      *outAnestor = aNode;
      return NS_OK;
    }
    tmp = aNode;
    aNode = parent;
  }
  return NS_ERROR_FAILURE;
}

