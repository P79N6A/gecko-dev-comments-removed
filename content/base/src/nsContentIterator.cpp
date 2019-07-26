




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







static nsINode*
NodeToParentOffset(nsINode* aNode, int32_t* aOffset)
{
  *aOffset = 0;

  nsINode* parent = aNode->GetParentNode();

  if (parent) {
    *aOffset = parent->IndexOf(aNode);
  }

  return parent;
}





static bool
NodeIsInTraversalRange(nsINode* aNode, bool aIsPreMode,
                       nsINode* aStartNode, int32_t aStartOffset,
                       nsINode* aEndNode, int32_t aEndOffset)
{
  if (!aStartNode || !aEndNode || !aNode) {
    return false;
  }

  
  
  if (aNode->IsNodeOfType(nsINode::eDATA_NODE) &&
      (aNode == aStartNode || aNode == aEndNode)) {
    return true;
  }

  nsINode* parent = aNode->GetParentNode();
  if (!parent) {
    return false;
  }

  int32_t indx = parent->IndexOf(aNode);

  if (!aIsPreMode) {
    ++indx;
  }

  return nsContentUtils::ComparePoints(aStartNode, aStartOffset,
                                       parent, indx) <= 0 &&
         nsContentUtils::ComparePoints(aEndNode, aEndOffset,
                                       parent, indx) >= 0;
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

  virtual void First();

  virtual void Last();

  virtual void Next();

  virtual void Prev();

  virtual nsINode* GetCurrentNode();

  virtual bool IsDone();

  virtual nsresult PositionAt(nsINode* aCurNode);

protected:

  
  
  nsINode* GetDeepFirstChild(nsINode* aRoot,
                             nsTArray<int32_t>* aIndexes = nullptr);
  nsIContent* GetDeepFirstChild(nsIContent* aRoot,
                                nsTArray<int32_t>* aIndexes = nullptr);
  nsINode* GetDeepLastChild(nsINode* aRoot,
                            nsTArray<int32_t>* aIndexes = nullptr);
  nsIContent* GetDeepLastChild(nsIContent* aRoot,
                               nsTArray<int32_t>* aIndexes = nullptr);

  
  
  
  nsIContent* GetNextSibling(nsINode* aNode,
                             nsTArray<int32_t>* aIndexes = nullptr);
  nsIContent* GetPrevSibling(nsINode* aNode,
                             nsTArray<int32_t>* aIndexes = nullptr);

  nsINode* NextNode(nsINode* aNode, nsTArray<int32_t>* aIndexes = nullptr);
  nsINode* PrevNode(nsINode* aNode, nsTArray<int32_t>* aIndexes = nullptr);

  
  nsresult RebuildIndexStack();

  void MakeEmpty();

  nsCOMPtr<nsINode> mCurNode;
  nsCOMPtr<nsINode> mFirst;
  nsCOMPtr<nsINode> mLast;
  nsCOMPtr<nsINode> mCommonParent;

  
  nsAutoTArray<int32_t, 8> mIndexes;

  
  
  
  
  
  
  int32_t mCachedIndex;
  
  
  
  
  
  
  
  
  
  
  
  
  

  bool mIsDone;
  bool mPre;

private:

  
  nsContentIterator(const nsContentIterator&);
  nsContentIterator& operator=(const nsContentIterator&);

};






already_AddRefed<nsIContentIterator>
NS_NewContentIterator()
{
  nsCOMPtr<nsIContentIterator> iter = new nsContentIterator(false);
  return iter.forget();
}


already_AddRefed<nsIContentIterator>
NS_NewPreContentIterator()
{
  nsCOMPtr<nsIContentIterator> iter = new nsContentIterator(true);
  return iter.forget();
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
  if (!aRoot) {
    return NS_ERROR_NULL_POINTER;
  }

  mIsDone = false;
  mIndexes.Clear();

  if (mPre) {
    mFirst = aRoot;
    mLast  = GetDeepLastChild(aRoot);
  } else {
    mFirst = GetDeepFirstChild(aRoot);
    mLast  = aRoot;
  }

  mCommonParent = aRoot;
  mCurNode = mFirst;
  RebuildIndexStack();
  return NS_OK;
}

nsresult
nsContentIterator::Init(nsIDOMRange* aDOMRange)
{
  NS_ENSURE_ARG_POINTER(aDOMRange);
  nsRange* range = static_cast<nsRange*>(aDOMRange);

  mIsDone = false;

  
  mCommonParent = range->GetCommonAncestor();
  NS_ENSURE_TRUE(mCommonParent, NS_ERROR_FAILURE);

  
  int32_t startIndx = range->StartOffset();
  nsINode* startNode = range->GetStartParent();
  NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);

  
  int32_t endIndx = range->EndOffset();
  nsINode* endNode = range->GetEndParent();
  NS_ENSURE_TRUE(endNode, NS_ERROR_FAILURE);

  bool startIsData = startNode->IsNodeOfType(nsINode::eDATA_NODE);

  
  if (startNode == endNode) {
    
    
    
    
    
    

    if (!startIsData && startIndx == endIndx) {
      MakeEmpty();
      return NS_OK;
    }

    if (startIsData) {
      
      mFirst   = startNode->AsContent();
      mLast    = mFirst;
      mCurNode = mFirst;

      RebuildIndexStack();
      return NS_OK;
    }
  }

  

  nsIContent* cChild = nullptr;

  if (!startIsData && startNode->HasChildren()) {
    cChild = startNode->GetChildAt(startIndx);
  }

  if (!cChild) {
    
    
    
    

    if (mPre) {
      
      
      

      if (!startIsData) {
        mFirst = GetNextSibling(startNode);

        
        

        if (mFirst && !NodeIsInTraversalRange(mFirst, mPre, startNode,
                                              startIndx, endNode, endIndx)) {
          mFirst = nullptr;
        }
      } else {
        mFirst = startNode->AsContent();
      }
    } else {
      
      if (startNode->IsContent()) {
        mFirst = startNode->AsContent();
      } else {
        
        mFirst = nullptr;
      }
    }
  } else {
    if (mPre) {
      mFirst = cChild;
    } else {
      
      mFirst = GetDeepFirstChild(cChild);

      
      

      if (mFirst && !NodeIsInTraversalRange(mFirst, mPre, startNode, startIndx,
                                            endNode, endIndx)) {
        mFirst = nullptr;
      }
    }
  }


  

  bool endIsData = endNode->IsNodeOfType(nsINode::eDATA_NODE);

  if (endIsData || !endNode->HasChildren() || endIndx == 0) {
    if (mPre) {
      if (endNode->IsContent()) {
        mLast = endNode->AsContent();
      } else {
        
        mLast = nullptr;
      }
    } else {
      
      
      
      

      if (!endIsData) {
        mLast = GetPrevSibling(endNode);

        if (!NodeIsInTraversalRange(mLast, mPre, startNode, startIndx,
                                    endNode, endIndx)) {
          mLast = nullptr;
        }
      } else {
        mLast = endNode->AsContent();
      }
    }
  } else {
    int32_t indx = endIndx;

    cChild = endNode->GetChildAt(--indx);

    if (!cChild) {
      
      NS_NOTREACHED("nsContentIterator::nsContentIterator");
      return NS_ERROR_FAILURE;
    }

    if (mPre) {
      mLast  = GetDeepLastChild(cChild);

      if (!NodeIsInTraversalRange(mLast, mPre, startNode, startIndx,
                                  endNode, endIndx)) {
        mLast = nullptr;
      }
    } else {
      
      mLast = cChild;
    }
  }

  

  if (!mFirst || !mLast) {
    mFirst = nullptr;
    mLast  = nullptr;
  }

  mCurNode = mFirst;
  mIsDone  = !mCurNode;

  if (!mCurNode) {
    mIndexes.Clear();
  } else {
    RebuildIndexStack();
  }

  return NS_OK;
}






nsresult
nsContentIterator::RebuildIndexStack()
{
  
  
  
  nsINode* parent;
  nsINode* current;

  mIndexes.Clear();
  current = mCurNode;
  if (!current) {
    return NS_OK;
  }

  while (current != mCommonParent) {
    parent = current->GetParentNode();

    if (!parent) {
      return NS_ERROR_FAILURE;
    }

    mIndexes.InsertElementAt(0, parent->IndexOf(current));

    current = parent;
  }

  return NS_OK;
}

void
nsContentIterator::MakeEmpty()
{
  mCurNode      = nullptr;
  mFirst        = nullptr;
  mLast         = nullptr;
  mCommonParent = nullptr;
  mIsDone       = true;
  mIndexes.Clear();
}

nsINode*
nsContentIterator::GetDeepFirstChild(nsINode* aRoot,
                                     nsTArray<int32_t>* aIndexes)
{
  if (!aRoot || !aRoot->HasChildren()) {
    return aRoot;
  }
  
  
  
  if (aIndexes) {
    aIndexes->AppendElement(0);
  }
  return GetDeepFirstChild(aRoot->GetFirstChild(), aIndexes);
}

nsIContent*
nsContentIterator::GetDeepFirstChild(nsIContent* aRoot,
                                     nsTArray<int32_t>* aIndexes)
{
  if (!aRoot) {
    return nullptr;
  }

  nsIContent* node = aRoot;
  nsIContent* child = node->GetFirstChild();

  while (child) {
    if (aIndexes) {
      
      aIndexes->AppendElement(0);
    }
    node = child;
    child = node->GetFirstChild();
  }

  return node;
}

nsINode*
nsContentIterator::GetDeepLastChild(nsINode* aRoot,
                                    nsTArray<int32_t>* aIndexes)
{
  if (!aRoot || !aRoot->HasChildren()) {
    return aRoot;
  }
  
  
  
  if (aIndexes) {
    aIndexes->AppendElement(aRoot->GetChildCount() - 1);
  }
  return GetDeepLastChild(aRoot->GetLastChild(), aIndexes);
}

nsIContent*
nsContentIterator::GetDeepLastChild(nsIContent* aRoot,
                                    nsTArray<int32_t>* aIndexes)
{
  if (!aRoot) {
    return nullptr;
  }

  nsIContent* node = aRoot;
  int32_t numChildren = node->GetChildCount();

  while (numChildren) {
    nsIContent* child = node->GetChildAt(--numChildren);

    if (aIndexes) {
      
      aIndexes->AppendElement(numChildren);
    }
    numChildren = child->GetChildCount();
    node = child;
  }

  return node;
}


nsIContent*
nsContentIterator::GetNextSibling(nsINode* aNode,
                                  nsTArray<int32_t>* aIndexes)
{
  if (!aNode) {
    return nullptr;
  }

  nsINode* parent = aNode->GetParentNode();
  if (!parent) {
    return nullptr;
  }

  int32_t indx = 0;

  NS_ASSERTION(!aIndexes || !aIndexes->IsEmpty(),
               "ContentIterator stack underflow");
  if (aIndexes && !aIndexes->IsEmpty()) {
    
    indx = (*aIndexes)[aIndexes->Length()-1];
  } else {
    indx = mCachedIndex;
  }

  
  
  
  nsIContent* sib = parent->GetChildAt(indx);
  if (sib != aNode) {
    
    indx = parent->IndexOf(aNode);
  }

  
  if ((sib = parent->GetChildAt(++indx))) {
    
    if (aIndexes && !aIndexes->IsEmpty()) {
      aIndexes->ElementAt(aIndexes->Length()-1) = indx;
    } else {
      mCachedIndex = indx;
    }
  } else {
    if (parent != mCommonParent) {
      if (aIndexes) {
        
        
        
        if (aIndexes->Length() > 1) {
          aIndexes->RemoveElementAt(aIndexes->Length()-1);
        }
      }
    }

    
    sib = GetNextSibling(parent, aIndexes);
  }

  return sib;
}


nsIContent*
nsContentIterator::GetPrevSibling(nsINode* aNode,
                                  nsTArray<int32_t>* aIndexes)
{
  if (!aNode) {
    return nullptr;
  }

  nsINode* parent = aNode->GetParentNode();
  if (!parent) {
    return nullptr;
  }

  int32_t indx = 0;

  NS_ASSERTION(!aIndexes || !aIndexes->IsEmpty(),
               "ContentIterator stack underflow");
  if (aIndexes && !aIndexes->IsEmpty()) {
    
    indx = (*aIndexes)[aIndexes->Length()-1];
  } else {
    indx = mCachedIndex;
  }

  
  
  nsIContent* sib = parent->GetChildAt(indx);
  if (sib != aNode) {
    
    indx = parent->IndexOf(aNode);
  }

  
  if (indx > 0 && (sib = parent->GetChildAt(--indx))) {
    
    if (aIndexes && !aIndexes->IsEmpty()) {
      aIndexes->ElementAt(aIndexes->Length()-1) = indx;
    } else {
      mCachedIndex = indx;
    }
  } else if (parent != mCommonParent) {
    if (aIndexes && !aIndexes->IsEmpty()) {
      
      aIndexes->RemoveElementAt(aIndexes->Length()-1);
    }
    return GetPrevSibling(parent, aIndexes);
  }

  return sib;
}

nsINode*
nsContentIterator::NextNode(nsINode* aNode, nsTArray<int32_t>* aIndexes)
{
  nsINode* node = aNode;

  
  if (mPre) {
    
    if (node->HasChildren()) {
      nsIContent* firstChild = node->GetFirstChild();

      
      if (aIndexes) {
        
        aIndexes->AppendElement(0);
      } else {
        mCachedIndex = 0;
      }

      return firstChild;
    }

    
    return GetNextSibling(node, aIndexes);
  }

  
  nsINode* parent = node->GetParentNode();
  nsIContent* sibling = nullptr;
  int32_t indx = 0;

  
  NS_ASSERTION(!aIndexes || !aIndexes->IsEmpty(),
               "ContentIterator stack underflow");
  if (aIndexes && !aIndexes->IsEmpty()) {
    
    indx = (*aIndexes)[aIndexes->Length()-1];
  } else {
    indx = mCachedIndex;
  }

  
  
  
  if (indx >= 0) {
    sibling = parent->GetChildAt(indx);
  }
  if (sibling != node) {
    
    indx = parent->IndexOf(node);
  }

  
  sibling = parent->GetChildAt(++indx);
  if (sibling) {
    
    if (aIndexes && !aIndexes->IsEmpty()) {
      
      aIndexes->ElementAt(aIndexes->Length()-1) = indx;
    } else {
      mCachedIndex = indx;
    }

    
    return GetDeepFirstChild(sibling, aIndexes);
  }

  
  if (aIndexes) {
    
    
    
    if (aIndexes->Length() > 1) {
      aIndexes->RemoveElementAt(aIndexes->Length()-1);
    }
  } else {
    
    mCachedIndex = 0;
  }

  return parent;
}

nsINode*
nsContentIterator::PrevNode(nsINode* aNode, nsTArray<int32_t>* aIndexes)
{
  nsINode* node = aNode;

  
  if (mPre) {
    nsINode* parent = node->GetParentNode();
    nsIContent* sibling = nullptr;
    int32_t indx = 0;

    
    NS_ASSERTION(!aIndexes || !aIndexes->IsEmpty(),
                 "ContentIterator stack underflow");
    if (aIndexes && !aIndexes->IsEmpty()) {
      
      indx = (*aIndexes)[aIndexes->Length()-1];
    } else {
      indx = mCachedIndex;
    }

    
    
    
    if (indx >= 0) {
      sibling = parent->GetChildAt(indx);
    }

    if (sibling != node) {
      
      indx = parent->IndexOf(node);
    }

    
    if (indx && (sibling = parent->GetChildAt(--indx))) {
      
      if (aIndexes && !aIndexes->IsEmpty()) {
        
        aIndexes->ElementAt(aIndexes->Length()-1) = indx;
      } else {
        mCachedIndex = indx;
      }

      
      return GetDeepLastChild(sibling, aIndexes);
    }

    
    if (aIndexes && !aIndexes->IsEmpty()) {
      
      aIndexes->RemoveElementAt(aIndexes->Length()-1);
    } else {
      
      mCachedIndex = 0;
    }
    return parent;
  }

  
  int32_t numChildren = node->GetChildCount();

  
  if (numChildren) {
    nsIContent* lastChild = node->GetLastChild();
    numChildren--;

    
    if (aIndexes) {
      
      aIndexes->AppendElement(numChildren);
    } else {
      mCachedIndex = numChildren;
    }

    return lastChild;
  }

  
  return GetPrevSibling(node, aIndexes);
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

  mIsDone = mFirst == nullptr;
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

  mIsDone = mLast == nullptr;
}


void
nsContentIterator::Next()
{
  if (mIsDone || !mCurNode) {
    return;
  }

  if (mCurNode == mLast) {
    mIsDone = true;
    return;
  }

  mCurNode = NextNode(mCurNode, &mIndexes);
}


void
nsContentIterator::Prev()
{
  if (mIsDone || !mCurNode) {
    return;
  }

  if (mCurNode == mFirst) {
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
  if (!aCurNode) {
    return NS_ERROR_NULL_POINTER;
  }

  nsINode* newCurNode = aCurNode;
  nsINode* tempNode = mCurNode;

  mCurNode = aCurNode;
  
  if (mCurNode == tempNode) {
    mIsDone = false;  
    return NS_OK;
  }

  

  nsINode* firstNode = mFirst;
  nsINode* lastNode = mLast;
  int32_t firstOffset = 0, lastOffset = 0;

  if (firstNode && lastNode) {
    if (mPre) {
      firstNode = NodeToParentOffset(mFirst, &firstOffset);

      if (lastNode->GetChildCount()) {
        lastOffset = 0;
      } else {
        lastNode = NodeToParentOffset(mLast, &lastOffset);
        ++lastOffset;
      }
    } else {
      uint32_t numChildren = firstNode->GetChildCount();

      if (numChildren) {
        firstOffset = numChildren;
      } else {
        firstNode = NodeToParentOffset(mFirst, &firstOffset);
      }

      lastNode = NodeToParentOffset(mLast, &lastOffset);
      ++lastOffset;
    }
  }

  
  
  
  if (mFirst != mCurNode && mLast != mCurNode &&
      (!firstNode || !lastNode ||
       !NodeIsInTraversalRange(mCurNode, mPre, firstNode, firstOffset,
                               lastNode, lastOffset))) {
    mIsDone = true;
    return NS_ERROR_FAILURE;
  }

  
  
  nsAutoTArray<nsINode*, 8>     oldParentStack;
  nsAutoTArray<int32_t, 8>      newIndexes;

  
  
  
  
  

  
  oldParentStack.SetCapacity(mIndexes.Length() + 1);

  
  
  
  
  for (int32_t i = mIndexes.Length() + 1; i > 0 && tempNode; i--) {
    
    oldParentStack.InsertElementAt(0, tempNode);

    nsINode* parent = tempNode->GetParentNode();

    if (!parent) {
      
      break;
    }

    if (parent == mCurNode) {
      
      
      mIndexes.RemoveElementsAt(mIndexes.Length() - oldParentStack.Length(),
                                oldParentStack.Length());
      mIsDone = false;
      return NS_OK;
    }
    tempNode = parent;
  }

  
  while (newCurNode) {
    nsINode* parent = newCurNode->GetParentNode();

    if (!parent) {
      
      break;
    }

    int32_t indx = parent->IndexOf(newCurNode);

    
    newIndexes.InsertElementAt(0, indx);

    
    indx = oldParentStack.IndexOf(parent);
    if (indx >= 0) {
      
      
      
      
      
      int32_t numToDrop = oldParentStack.Length() - (1 + indx);
      if (numToDrop > 0) {
        mIndexes.RemoveElementsAt(mIndexes.Length() - numToDrop, numToDrop);
      }
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
    return nullptr;
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

  virtual void Next();

  virtual void Prev();

  virtual nsresult PositionAt(nsINode* aCurNode);

  
  virtual void First();

  
  virtual void Last();

protected:

  
  
  
  
  
  nsIContent* GetTopAncestorInRange(nsINode* aNode);

  
  nsContentSubtreeIterator(const nsContentSubtreeIterator&);
  nsContentSubtreeIterator& operator=(const nsContentSubtreeIterator&);

  nsRefPtr<nsRange> mRange;

  
  nsAutoTArray<nsIContent*, 8> mEndNodes;
  nsAutoTArray<int32_t, 8>     mEndOffsets;
};

NS_IMPL_ADDREF_INHERITED(nsContentSubtreeIterator, nsContentIterator)
NS_IMPL_RELEASE_INHERITED(nsContentSubtreeIterator, nsContentIterator)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsContentSubtreeIterator)
NS_INTERFACE_MAP_END_INHERITING(nsContentIterator)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsContentSubtreeIterator, nsContentIterator)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRange)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsContentSubtreeIterator, nsContentIterator)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRange)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END








already_AddRefed<nsIContentIterator>
NS_NewContentSubtreeIterator()
{
  nsCOMPtr<nsIContentIterator> iter = new nsContentSubtreeIterator();
  return iter.forget();
}








nsresult
nsContentSubtreeIterator::Init(nsINode* aRoot)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult
nsContentSubtreeIterator::Init(nsIDOMRange* aRange)
{
  MOZ_ASSERT(aRange);

  mIsDone = false;

  mRange = static_cast<nsRange*>(aRange);

  
  mCommonParent = mRange->GetCommonAncestor();
  nsINode* startParent = mRange->GetStartParent();
  int32_t startOffset = mRange->StartOffset();
  nsINode* endParent = mRange->GetEndParent();
  int32_t endOffset = mRange->EndOffset();
  MOZ_ASSERT(mCommonParent && startParent && endParent);
  
  MOZ_ASSERT(uint32_t(startOffset) <= startParent->Length() &&
             uint32_t(endOffset) <= endParent->Length());

  
  if (startParent == endParent) {
    nsINode* child = startParent->GetFirstChild();

    if (!child || startOffset == endOffset) {
      
      MakeEmpty();
      return NS_OK;
    }
  }

  
  nsContentUtils::GetAncestorsAndOffsets(endParent->AsDOMNode(), endOffset,
                                         &mEndNodes, &mEndOffsets);

  nsIContent* firstCandidate = nullptr;
  nsIContent* lastCandidate = nullptr;

  
  int32_t offset = mRange->StartOffset();

  nsINode* node;
  if (!startParent->GetChildCount()) {
    
    node = startParent;
  } else {
    nsIContent* child = startParent->GetChildAt(offset);
    if (!child) {
      
      node = startParent;
    } else {
      firstCandidate = child;
    }
  }

  if (!firstCandidate) {
    
    firstCandidate = GetNextSibling(node);

    if (!firstCandidate) {
      MakeEmpty();
      return NS_OK;
    }
  }

  firstCandidate = GetDeepFirstChild(firstCandidate);

  
  

  bool nodeBefore, nodeAfter;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    nsRange::CompareNodeToRange(firstCandidate, mRange, &nodeBefore, &nodeAfter)));

  if (nodeBefore || nodeAfter) {
    MakeEmpty();
    return NS_OK;
  }

  
  
  
  mFirst = GetTopAncestorInRange(firstCandidate);

  
  offset = mRange->EndOffset();
  int32_t numChildren = endParent->GetChildCount();

  if (offset > numChildren) {
    
    offset = numChildren;
  }
  if (!offset || !numChildren) {
    node = endParent;
  } else {
    lastCandidate = endParent->GetChildAt(--offset);
    NS_ASSERTION(lastCandidate,
                 "tree traversal trouble in nsContentSubtreeIterator::Init");
  }

  if (!lastCandidate) {
    
    lastCandidate = GetPrevSibling(node);
  }

  if (!lastCandidate) {
    MakeEmpty();
    return NS_OK;
  }

  lastCandidate = GetDeepLastChild(lastCandidate);

  
  

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    nsRange::CompareNodeToRange(lastCandidate, mRange, &nodeBefore, &nodeAfter)));

  if (nodeBefore || nodeAfter) {
    MakeEmpty();
    return NS_OK;
  }

  
  
  
  mLast = GetTopAncestorInRange(lastCandidate);

  mCurNode = mFirst;

  return NS_OK;
}






void
nsContentSubtreeIterator::First()
{
  mIsDone = mFirst == nullptr;

  mCurNode = mFirst;
}


void
nsContentSubtreeIterator::Last()
{
  mIsDone = mLast == nullptr;

  mCurNode = mLast;
}


void
nsContentSubtreeIterator::Next()
{
  if (mIsDone || !mCurNode) {
    return;
  }

  if (mCurNode == mLast) {
    mIsDone = true;
    return;
  }

  nsINode* nextNode = GetNextSibling(mCurNode);
  NS_ASSERTION(nextNode, "No next sibling!?! This could mean deadlock!");

  int32_t i = mEndNodes.IndexOf(nextNode);
  while (i != -1) {
    
    
    nextNode = nextNode->GetFirstChild();
    NS_ASSERTION(nextNode, "Iterator error, expected a child node!");

    
    
    
    
    i = mEndNodes.IndexOf(nextNode);
  }

  mCurNode = nextNode;

  
  
  
  mIsDone = mCurNode == nullptr;
}


void
nsContentSubtreeIterator::Prev()
{
  
  
  if (mIsDone || !mCurNode) {
    return;
  }

  if (mCurNode == mFirst) {
    mIsDone = true;
    return;
  }

  
  
  nsINode* prevNode = GetDeepFirstChild(mCurNode);

  prevNode = PrevNode(prevNode);

  prevNode = GetDeepLastChild(prevNode);

  mCurNode = GetTopAncestorInRange(prevNode);

  
  
  
  mIsDone = mCurNode == nullptr;
}


nsresult
nsContentSubtreeIterator::PositionAt(nsINode* aCurNode)
{
  NS_ERROR("Not implemented!");

  return NS_ERROR_NOT_IMPLEMENTED;
}





nsIContent*
nsContentSubtreeIterator::GetTopAncestorInRange(nsINode* aNode)
{
  if (!aNode || !aNode->GetParentNode()) {
    return nullptr;
  }

  
  nsIContent* content = aNode->AsContent();

  
  bool nodeBefore, nodeAfter;
  nsresult res = nsRange::CompareNodeToRange(aNode, mRange,
                                             &nodeBefore, &nodeAfter);
  NS_ASSERTION(NS_SUCCEEDED(res) && !nodeBefore && !nodeAfter,
               "aNode isn't in mRange, or something else weird happened");
  if (NS_FAILED(res) || nodeBefore || nodeAfter) {
    return nullptr;
  }

  while (content) {
    nsIContent* parent = content->GetParent();
    
    
    
    
    
    
    if (!parent || !parent->GetParentNode()) {
      return content;
    }
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      nsRange::CompareNodeToRange(parent, mRange, &nodeBefore, &nodeAfter)));

    if (nodeBefore || nodeAfter) {
      return content;
    }
    content = parent;
  }

  MOZ_NOT_REACHED("This should only be possible if aNode was null");
}
