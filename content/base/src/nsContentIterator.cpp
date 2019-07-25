





































#include "nsISupports.h"
#include "nsIDOMNodeList.h"
#include "nsIContentIterator.h"
#include "nsRange.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsContentUtils.h"
#include "nsINode.h"
#include "nsCycleCollectionParticipant.h"






static inline bool
NodeHasChildren(nsINode *aNode)
{
  return aNode->GetChildCount() > 0;
}





static nsINode*
NodeToParentOffset(nsINode *aNode, PRInt32 *aOffset)
{
  *aOffset  = 0;

  nsINode* parent = aNode->GetNodeParent();

  if (parent) {
    *aOffset = parent->IndexOf(aNode);
  }
  
  return parent;
}





static bool
NodeIsInTraversalRange(nsINode *aNode, bool aIsPreMode,
                       nsINode *aStartNode, PRInt32 aStartOffset,
                       nsINode *aEndNode, PRInt32 aEndOffset)
{
  if (!aStartNode || !aEndNode || !aNode)
    return false;

  
  
  if (aNode->IsNodeOfType(nsINode::eDATA_NODE) &&
      (aNode == aStartNode || aNode == aEndNode)) {
    return true;
  }

  nsINode* parent = aNode->GetNodeParent();
  if (!parent)
    return false;

  PRInt32 indx = parent->IndexOf(aNode);

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
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsContentIterator)

  explicit nsContentIterator(bool aPre);
  virtual ~nsContentIterator();

  

  virtual nsresult Init(nsINode* aRoot);

  virtual nsresult Init(nsIDOMRange* aRange);
  virtual nsresult Init(nsIRange* aRange);

  virtual void First();

  virtual void Last();
  
  virtual void Next();

  virtual void Prev();

  virtual nsINode *GetCurrentNode();

  virtual bool IsDone();

  virtual nsresult PositionAt(nsINode* aCurNode);

  
  
  

protected:

  nsINode* GetDeepFirstChild(nsINode *aRoot, nsTArray<PRInt32> *aIndexes);
  nsINode* GetDeepLastChild(nsINode *aRoot, nsTArray<PRInt32> *aIndexes);

  
  
  nsINode* GetNextSibling(nsINode *aNode, nsTArray<PRInt32> *aIndexes);

  
  
  nsINode* GetPrevSibling(nsINode *aNode, nsTArray<PRInt32> *aIndexes);

  nsINode* NextNode(nsINode *aNode, nsTArray<PRInt32> *aIndexes);
  nsINode* PrevNode(nsINode *aNode, nsTArray<PRInt32> *aIndexes);

  
  nsresult RebuildIndexStack();

  void MakeEmpty();
  
  nsCOMPtr<nsINode> mCurNode;
  nsCOMPtr<nsINode> mFirst;
  nsCOMPtr<nsINode> mLast;
  nsCOMPtr<nsINode> mCommonParent;

  
  nsAutoTArray<PRInt32, 8> mIndexes;

  
  
  
  
  
  PRInt32 mCachedIndex;
  
  
  
  
  
  
  
  
  
  
  
  bool mIsDone;
  bool mPre;
  
private:

  
  nsContentIterator(const nsContentIterator&);
  nsContentIterator& operator=(const nsContentIterator&);

};






nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult)
{
  nsContentIterator * iter = new nsContentIterator(false);
  if (!iter) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = iter);

  return NS_OK;
}


nsresult NS_NewPreContentIterator(nsIContentIterator** aInstancePtrResult)
{
  nsContentIterator * iter = new nsContentIterator(true);
  if (!iter) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = iter);

  return NS_OK;
}





 
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsContentIterator)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsContentIterator)

NS_INTERFACE_MAP_BEGIN(nsContentIterator)
  NS_INTERFACE_MAP_ENTRY(nsIContentIterator)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentIterator)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsContentIterator)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_4(nsContentIterator,
                           mCurNode,
                           mFirst,
                           mLast,
                           mCommonParent)





nsContentIterator::nsContentIterator(bool aPre) :
  
  mCachedIndex(0), mIsDone(false), mPre(aPre)
{
}


nsContentIterator::~nsContentIterator()
{
}







nsresult
nsContentIterator::Init(nsINode* aRoot)
{
  if (!aRoot) 
    return NS_ERROR_NULL_POINTER; 

  mIsDone = false;
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
  return Init(range);

}

nsresult
nsContentIterator::Init(nsIRange* aRange)
{
  NS_ENSURE_ARG_POINTER(aRange);

  mIsDone = false;

  
  mCommonParent = aRange->GetCommonAncestor();
  NS_ENSURE_TRUE(mCommonParent, NS_ERROR_FAILURE);

  
  PRInt32 startIndx = aRange->StartOffset();
  nsINode* startNode = aRange->GetStartParent();
  NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);

  
  PRInt32 endIndx = aRange->EndOffset();
  nsINode* endNode = aRange->GetEndParent();
  NS_ENSURE_TRUE(endNode, NS_ERROR_FAILURE);

  bool startIsData = startNode->IsNodeOfType(nsINode::eDATA_NODE);

  
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

      mFirst   = static_cast<nsIContent*>(startNode);
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
            !NodeIsInTraversalRange(mFirst, mPre, startNode, startIndx,
                                    endNode, endIndx)) {
          mFirst = nsnull;
        }
      }
      else {
        NS_ASSERTION(startNode->IsNodeOfType(nsINode::eCONTENT),
                   "Data node that's not content?");

        mFirst = static_cast<nsIContent*>(startNode);
      }
    }
    else {
      
      if (startNode->IsNodeOfType(nsINode::eCONTENT)) {
        mFirst = static_cast<nsIContent*>(startNode);
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
          !NodeIsInTraversalRange(mFirst, mPre, startNode, startIndx,
                                  endNode, endIndx))
        mFirst = nsnull;
    }
  }


  

  bool endIsData = endNode->IsNodeOfType(nsINode::eDATA_NODE);

  if (endIsData || !NodeHasChildren(endNode) || endIndx == 0)
  {
    if (mPre) {
      if (endNode->IsNodeOfType(nsINode::eCONTENT)) {
        mLast = static_cast<nsIContent*>(endNode);
      } else {
        
        mLast = nsnull;
      }
    }
    else 
    {
      
      
      

      if (!endIsData)
      {
        mLast = GetPrevSibling(endNode, nsnull);

        if (!NodeIsInTraversalRange(mLast, mPre, startNode, startIndx,
                                    endNode, endIndx))
          mLast = nsnull;
      }
      else {
        NS_ASSERTION(endNode->IsNodeOfType(nsINode::eCONTENT),
                     "Data node that's not content?");

        mLast = static_cast<nsIContent*>(endNode);
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

      if (!NodeIsInTraversalRange(mLast, mPre, startNode, startIndx,
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
  
  
  
  nsINode* parent;
  nsINode* current;

  mIndexes.Clear();
  current = mCurNode;
  if (!current) {
    return NS_OK;
  }

  while (current != mCommonParent)
  {
    parent = current->GetNodeParent();
    
    if (!parent)
      return NS_ERROR_FAILURE;
  
    mIndexes.InsertElementAt(0, parent->IndexOf(current));

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
  mIsDone       = true;
  mIndexes.Clear();
}

nsINode*
nsContentIterator::GetDeepFirstChild(nsINode *aRoot,
                                     nsTArray<PRInt32> *aIndexes)
{
  if (!aRoot) {
    return nsnull;
  }

  nsINode *n = aRoot;
  nsINode *nChild = n->GetFirstChild();

  while (nChild)
  {
    if (aIndexes)
    {
      
      aIndexes->AppendElement(0);
    }
    n = nChild;
    nChild = n->GetFirstChild();
  }

  return n;
}

nsINode*
nsContentIterator::GetDeepLastChild(nsINode *aRoot, nsTArray<PRInt32> *aIndexes)
{
  if (!aRoot) {
    return nsnull;
  }

  nsINode *deepLastChild = aRoot;

  nsINode *n = aRoot;
  PRInt32 numChildren = n->GetChildCount();

  while (numChildren)
  {
    nsINode *nChild = n->GetChildAt(--numChildren);

    if (aIndexes)
    {
      
      aIndexes->AppendElement(numChildren);
    }
    numChildren = nChild->GetChildCount();
    n = nChild;

    deepLastChild = n;
  }

  return deepLastChild;
}


nsINode *
nsContentIterator::GetNextSibling(nsINode *aNode, 
                                  nsTArray<PRInt32> *aIndexes)
{
  if (!aNode) 
    return nsnull;

  nsINode *parent = aNode->GetNodeParent();
  if (!parent)
    return nsnull;

  PRInt32 indx = 0;

  NS_ASSERTION(!aIndexes || !aIndexes->IsEmpty(),
               "ContentIterator stack underflow");
  if (aIndexes && !aIndexes->IsEmpty())
  {
    
    indx = (*aIndexes)[aIndexes->Length()-1];
  }
  else
    indx = mCachedIndex;

  
  
  
  nsINode *sib = parent->GetChildAt(indx);
  if (sib != aNode)
  {
    
    indx = parent->IndexOf(aNode);
  }

  
  if ((sib = parent->GetChildAt(++indx)))
  {
    
    if (aIndexes && !aIndexes->IsEmpty())
    {
      aIndexes->ElementAt(aIndexes->Length()-1) = indx;
    }
    else mCachedIndex = indx;
  }
  else
  {
    if (parent != mCommonParent)
    {
      if (aIndexes)
      {
        
        
        
        if (aIndexes->Length() > 1)
          aIndexes->RemoveElementAt(aIndexes->Length()-1);
      }
    }

    
    sib = GetNextSibling(parent, aIndexes);
  }
  
  return sib;
}


nsINode*
nsContentIterator::GetPrevSibling(nsINode *aNode, 
                                  nsTArray<PRInt32> *aIndexes)
{
  if (!aNode)
    return nsnull;

  nsINode *parent = aNode->GetNodeParent();
  if (!parent)
    return nsnull;

  PRInt32 indx = 0;

  NS_ASSERTION(!aIndexes || !aIndexes->IsEmpty(),
               "ContentIterator stack underflow");
  if (aIndexes && !aIndexes->IsEmpty())
  {
    
    indx = (*aIndexes)[aIndexes->Length()-1];
  }
  else
    indx = mCachedIndex;

  
  
  nsINode *sib = parent->GetChildAt(indx);
  if (sib != aNode)
  {
    
    indx = parent->IndexOf(aNode);
  }

  
  if (indx > 0 && (sib = parent->GetChildAt(--indx)))
  {
    
    if (aIndexes && !aIndexes->IsEmpty())
    {
      aIndexes->ElementAt(aIndexes->Length()-1) = indx;
    }
    else mCachedIndex = indx;
  }
  else if (parent != mCommonParent)
  {
    if (aIndexes && !aIndexes->IsEmpty())
    {
      
      aIndexes->RemoveElementAt(aIndexes->Length()-1);
    }
    return GetPrevSibling(parent, aIndexes);
  }

  return sib;
}

nsINode*
nsContentIterator::NextNode(nsINode *aNode, nsTArray<PRInt32> *aIndexes)
{
  nsINode *n = aNode;
  nsINode *nextNode = nsnull;

  if (mPre)  
  {
    
    if (NodeHasChildren(n))
    {
      nsINode *nFirstChild = n->GetFirstChild();

      
      if (aIndexes)
      {
        
        aIndexes->AppendElement(0);
      }
      else mCachedIndex = 0;
      
      return nFirstChild;
    }

    
    nextNode = GetNextSibling(n, aIndexes);
  }
  else  
  {
    nsINode *parent = n->GetNodeParent();
    nsINode *nSibling = nsnull;
    PRInt32 indx = 0;

    
    NS_ASSERTION(!aIndexes || !aIndexes->IsEmpty(),
                 "ContentIterator stack underflow");
    if (aIndexes && !aIndexes->IsEmpty())
    {
      
      indx = (*aIndexes)[aIndexes->Length()-1];
    }
    else indx = mCachedIndex;

    
    
    
    if (indx >= 0)
      nSibling = parent->GetChildAt(indx);
    if (nSibling != n)
    {
      
      indx = parent->IndexOf(n);
    }

    
    nSibling = parent->GetChildAt(++indx);
    if (nSibling)
    {
      
      if (aIndexes && !aIndexes->IsEmpty())
      {
        
        aIndexes->ElementAt(aIndexes->Length()-1) = indx;
      }
      else mCachedIndex = indx;
      
      
      return GetDeepFirstChild(nSibling, aIndexes); 
    }
  
    
    
    if (aIndexes)
    {
      
      
      
      if (aIndexes->Length() > 1)
        aIndexes->RemoveElementAt(aIndexes->Length()-1);
    }
    else mCachedIndex = 0;   
    nextNode = parent;
  }

  return nextNode;
}

nsINode*
nsContentIterator::PrevNode(nsINode *aNode, nsTArray<PRInt32> *aIndexes)
{
  nsINode *prevNode = nsnull;
  nsINode *n = aNode;
   
  if (mPre)  
  {
    nsINode *parent = n->GetNodeParent();
    nsINode *nSibling = nsnull;
    PRInt32 indx = 0;

    
    NS_ASSERTION(!aIndexes || !aIndexes->IsEmpty(),
                 "ContentIterator stack underflow");
    if (aIndexes && !aIndexes->IsEmpty())
    {
      
      indx = (*aIndexes)[aIndexes->Length()-1];
    }
    else indx = mCachedIndex;

    
    
    
    if (indx >= 0)
      nSibling = parent->GetChildAt(indx);

    if (nSibling != n)
    {
      
      indx = parent->IndexOf(n);
    }

    
    if (indx && (nSibling = parent->GetChildAt(--indx)))
    {
      
      if (aIndexes && !aIndexes->IsEmpty())
      {
        
        aIndexes->ElementAt(aIndexes->Length()-1) = indx;
      }
      else mCachedIndex = indx;
      
      
      return GetDeepLastChild(nSibling, aIndexes); 
    }
  
    
    
    if (aIndexes && !aIndexes->IsEmpty())
    {
      
      aIndexes->RemoveElementAt(aIndexes->Length()-1);
    }
    else mCachedIndex = 0;   
    prevNode = parent;
  }
  else  
  {
    PRInt32 numChildren = n->GetChildCount();
  
    
    if (numChildren)
    {
      nsINode *nLastChild = n->GetLastChild();
      numChildren--;

      
      if (aIndexes)
      {
        
        aIndexes->AppendElement(numChildren);
      }
      else mCachedIndex = numChildren;
      
      return nLastChild;
    }

    
    prevNode = GetPrevSibling(n, aIndexes);
  }

  return prevNode;
}





void
nsContentIterator::First()
{
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
    mIsDone = true;
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
    mIsDone = true;
    return;
  }

  mCurNode = PrevNode(mCurNode, &mIndexes);
}


bool
nsContentIterator::IsDone()
{
  return mIsDone;
}




nsresult
nsContentIterator::PositionAt(nsINode* aCurNode)
{
  if (!aCurNode)
    return NS_ERROR_NULL_POINTER;

  nsINode *newCurNode = aCurNode;
  nsINode *tempNode = mCurNode;

  mCurNode = aCurNode;
  
  if (mCurNode == tempNode)
  {
    mIsDone = false;  
    return NS_OK;
  }

  

  nsINode* firstNode = mFirst;
  nsINode* lastNode = mLast;
  PRInt32 firstOffset=0, lastOffset=0;

  if (firstNode && lastNode)
  {
    if (mPre)
    {
      firstNode = NodeToParentOffset(mFirst, &firstOffset);

      if (lastNode->GetChildCount())
        lastOffset = 0;
      else
      {
        lastNode = NodeToParentOffset(mLast, &lastOffset);
        ++lastOffset;
      }
    }
    else
    {
      PRUint32 numChildren = firstNode->GetChildCount();

      if (numChildren)
        firstOffset = numChildren;
      else
        firstNode = NodeToParentOffset(mFirst, &firstOffset);

      lastNode = NodeToParentOffset(mLast, &lastOffset);
      ++lastOffset;
    }
  }

  
  
  
  if (mFirst != mCurNode && mLast != mCurNode &&
      (!firstNode || !lastNode ||
       !NodeIsInTraversalRange(mCurNode, mPre, firstNode, firstOffset,
                               lastNode, lastOffset)))
  {
    mIsDone = true;
    return NS_ERROR_FAILURE;
  }

  
  
  nsAutoTArray<nsINode*, 8>     oldParentStack;
  nsAutoTArray<PRInt32, 8>      newIndexes;

  
  
  
  
  
  

  
  
  if (!oldParentStack.SetCapacity(mIndexes.Length()+1))
    return NS_ERROR_FAILURE;

  
  
  
  
  for (PRInt32 i = mIndexes.Length()+1; i > 0 && tempNode; i--)
  {
    
    oldParentStack.InsertElementAt(0, tempNode);

    nsINode *parent = tempNode->GetNodeParent();

    if (!parent)  
      break;

    if (parent == mCurNode)
    {
      
      
      mIndexes.RemoveElementsAt(mIndexes.Length() - oldParentStack.Length(),
                                oldParentStack.Length());
      mIsDone = false;
      return NS_OK;
    }
    tempNode = parent;
  }

  
  while (newCurNode)
  {
    nsINode *parent = newCurNode->GetNodeParent();

    if (!parent)  
      break;

    PRInt32 indx = parent->IndexOf(newCurNode);

    
    newIndexes.InsertElementAt(0, indx);

    
    indx = oldParentStack.IndexOf(parent);
    if (indx >= 0)
    {
      
      
      
      
      
      PRInt32 numToDrop = oldParentStack.Length()-(1+indx);
      if (numToDrop > 0)
        mIndexes.RemoveElementsAt(mIndexes.Length() - numToDrop, numToDrop);
      mIndexes.AppendElements(newIndexes);

      break;
    }
    newCurNode = parent;
  }

  

  mIsDone = false;
  return NS_OK;
}

nsINode*
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
  nsContentSubtreeIterator() : nsContentIterator(false) {}
  virtual ~nsContentSubtreeIterator() {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsContentSubtreeIterator, nsContentIterator)

  

  virtual nsresult Init(nsINode* aRoot);

  virtual nsresult Init(nsIDOMRange* aRange);
  virtual nsresult Init(nsIRange* aRange);

  virtual void Next();

  virtual void Prev();

  virtual nsresult PositionAt(nsINode* aCurNode);

  
  virtual void First();

  
  virtual void Last();

protected:

  nsresult GetTopAncestorInRange(nsINode *aNode,
                                 nsCOMPtr<nsINode> *outAnestor);

  
  nsContentSubtreeIterator(const nsContentSubtreeIterator&);
  nsContentSubtreeIterator& operator=(const nsContentSubtreeIterator&);

  nsCOMPtr<nsIDOMRange> mRange;
  
#if 0
  nsAutoTArray<nsIContent*, 8> mStartNodes;
  nsAutoTArray<PRInt32, 8>     mStartOffsets;
#endif

  nsAutoTArray<nsIContent*, 8> mEndNodes;
  nsAutoTArray<PRInt32, 8>     mEndOffsets;
};

NS_IMPL_ADDREF_INHERITED(nsContentSubtreeIterator, nsContentIterator)
NS_IMPL_RELEASE_INHERITED(nsContentSubtreeIterator, nsContentIterator)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsContentSubtreeIterator)
NS_INTERFACE_MAP_END_INHERITING(nsContentIterator)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsContentSubtreeIterator)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsContentSubtreeIterator, nsContentIterator)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRange)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsContentSubtreeIterator, nsContentIterator)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRange)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

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








nsresult nsContentSubtreeIterator::Init(nsINode* aRoot)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult nsContentSubtreeIterator::Init(nsIDOMRange* aRange)
{
  if (!aRange) 
    return NS_ERROR_NULL_POINTER; 

  mIsDone = false;

  mRange = aRange;
  
  
  nsCOMPtr<nsIDOMNode> commonParent;
  nsCOMPtr<nsIDOMNode> startParent;
  nsCOMPtr<nsIDOMNode> endParent;
  nsCOMPtr<nsINode> nStartP;
  nsCOMPtr<nsINode> nEndP;
  nsCOMPtr<nsINode> n;
  nsINode *firstCandidate = nsnull;
  nsINode *lastCandidate = nsnull;
  PRInt32 indx, startIndx, endIndx;

  
  if (NS_FAILED(aRange->GetCommonAncestorContainer(getter_AddRefs(commonParent))) || !commonParent)
    return NS_ERROR_FAILURE;
  mCommonParent = do_QueryInterface(commonParent);

  
  if (NS_FAILED(aRange->GetStartContainer(getter_AddRefs(startParent))) || !startParent)
    return NS_ERROR_FAILURE;
  nStartP = do_QueryInterface(startParent);
  aRange->GetStartOffset(&startIndx);

  
  if (NS_FAILED(aRange->GetEndContainer(getter_AddRefs(endParent))) || !endParent)
    return NS_ERROR_FAILURE;
  nEndP = do_QueryInterface(endParent);
  aRange->GetEndOffset(&endIndx);

  
  if (startParent == endParent)
  {
    nsINode* nChild = nStartP->GetFirstChild();
  
    if (!nChild) 
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

  if (!nStartP->GetChildCount()) 
  {
    n = nStartP;
  }
  else
  {
    nsINode* nChild = nStartP->GetChildAt(indx);
    if (!nChild)  
    {
      n = nStartP;
    }
    else
    {
      firstCandidate = nChild;
    }
  }
  
  if (!firstCandidate)
  {
    
    firstCandidate = GetNextSibling(n, nsnull);

    if (!firstCandidate)
    {
      MakeEmpty();
      return NS_OK;
    }
  }
  
  firstCandidate = GetDeepFirstChild(firstCandidate, nsnull);
  
  
  
  
  
  bool nodeBefore, nodeAfter;
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
  PRInt32 numChildren = nEndP->GetChildCount();

  if (indx > numChildren) indx = numChildren;
  if (!indx)
  {
    n = nEndP;
  }
  else
  {
    if (!numChildren) 
    {
      n = nEndP;
    }
    else
    {
      lastCandidate = nEndP->GetChildAt(--indx);
      NS_ASSERTION(lastCandidate,
                   "tree traversal trouble in nsContentSubtreeIterator::Init");
    }
  }
  
  if (!lastCandidate)
  {
    
    lastCandidate = GetPrevSibling(n, nsnull);
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

nsresult nsContentSubtreeIterator::Init(nsIRange* aRange)
{
  nsCOMPtr<nsIDOMRange> range = do_QueryInterface(aRange);
  return Init(range);
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
    mIsDone = true;
    return;
  }

  nsINode *nextNode = GetNextSibling(mCurNode, nsnull);
  NS_ASSERTION(nextNode, "No next sibling!?! This could mean deadlock!");





  PRInt32 i = mEndNodes.IndexOf(nextNode);
  while (i != -1)
  {
    
    
    nextNode = nextNode->GetFirstChild();
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
    mIsDone = true;
    return;
  }

  nsINode *prevNode = PrevNode(GetDeepFirstChild(mCurNode, nsnull), nsnull);

  prevNode = GetDeepLastChild(prevNode, nsnull);
  
  GetTopAncestorInRange(prevNode, address_of(mCurNode));

  
  
  
  mIsDone = mCurNode == nsnull;
}


nsresult
nsContentSubtreeIterator::PositionAt(nsINode* aCurNode)
{
  NS_ERROR("Not implemented!");

  return NS_ERROR_NOT_IMPLEMENTED;
}





nsresult
nsContentSubtreeIterator::GetTopAncestorInRange(nsINode *aNode,
                                                nsCOMPtr<nsINode> *outAncestor)
{
  if (!aNode) 
    return NS_ERROR_NULL_POINTER;
  if (!outAncestor) 
    return NS_ERROR_NULL_POINTER;
  
  
  
  bool nodeBefore, nodeAfter;
  if (NS_FAILED(nsRange::CompareNodeToRange(aNode, mRange, &nodeBefore,
                                            &nodeAfter)))
    return NS_ERROR_FAILURE;

  if (nodeBefore || nodeAfter)
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsINode> parent, tmp;
  while (aNode)
  {
    parent = aNode->GetNodeParent();
    if (!parent)
    {
      if (tmp)
      {
        *outAncestor = tmp;
        return NS_OK;
      }
      else return NS_ERROR_FAILURE;
    }
    if (NS_FAILED(nsRange::CompareNodeToRange(parent, mRange, &nodeBefore,
                                              &nodeAfter)))
      return NS_ERROR_FAILURE;

    if (nodeBefore || nodeAfter)
    {
      *outAncestor = aNode;
      return NS_OK;
    }
    tmp = aNode;
    aNode = parent;
  }
  return NS_ERROR_FAILURE;
}

