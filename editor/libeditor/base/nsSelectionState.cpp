




































#include "nsSelectionState.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsISelection.h"
#include "nsEditor.h"
#include "nsEditorUtils.h"







nsSelectionState::nsSelectionState() : mArray(){}

nsSelectionState::~nsSelectionState() 
{
  MakeEmpty();
}

nsresult  
nsSelectionState::SaveSelection(nsISelection *aSel)
{
  if (!aSel) return NS_ERROR_NULL_POINTER;
  PRInt32 i,rangeCount, arrayCount = mArray.Count();
  aSel->GetRangeCount(&rangeCount);
  nsRangeStore *item;
  
  
  if (arrayCount<rangeCount)
  {
    PRInt32 count = rangeCount-arrayCount;
    for (i=0; i<count; i++)
    {
      item = new nsRangeStore;
      mArray.AppendElement(item);
    }
  }
  
  
  else if (arrayCount>rangeCount)
  {
    for (i = arrayCount-1; i >= rangeCount; i--)
    {
      item = (nsRangeStore*)mArray.ElementAt(i);
      delete item;
      mArray.RemoveElementAt(i);
    }
  }
  
  
  nsresult res = NS_OK;
  for (i=0; i<rangeCount; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_UNEXPECTED;
    nsCOMPtr<nsIDOMRange> range;
    res = aSel->GetRangeAt(i, getter_AddRefs(range));
    item->StoreRange(range);
  }
  
  return res;
}

nsresult  
nsSelectionState::RestoreSelection(nsISelection *aSel)
{
  if (!aSel) return NS_ERROR_NULL_POINTER;
  nsresult res;
  PRInt32 i, arrayCount = mArray.Count();
  nsRangeStore *item;

  
  aSel->RemoveAllRanges();
  
  
  for (i=0; i<arrayCount; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_UNEXPECTED;
    nsCOMPtr<nsIDOMRange> range;
    item->GetRange(address_of(range));
    if (!range) return NS_ERROR_UNEXPECTED;
   
    res = aSel->AddRange(range);
    if(NS_FAILED(res)) return res;

  }
  return NS_OK;
}

PRBool
nsSelectionState::IsCollapsed()
{
  if (1 != mArray.Count()) return PR_FALSE;
  nsRangeStore *item;
  item = (nsRangeStore*)mArray.ElementAt(0);
  if (!item) return PR_FALSE;
  nsCOMPtr<nsIDOMRange> range;
  item->GetRange(address_of(range));
  if (!range) return PR_FALSE;
  PRBool bIsCollapsed;
  range->GetCollapsed(&bIsCollapsed);
  return bIsCollapsed;
}

PRBool
nsSelectionState::IsEqual(nsSelectionState *aSelState)
{
  if (!aSelState) return NS_ERROR_NULL_POINTER;
  PRInt32 i, myCount = mArray.Count(), itsCount = aSelState->mArray.Count();
  if (myCount != itsCount) return PR_FALSE;
  if (myCount < 1) return PR_FALSE;

  nsRangeStore *myItem, *itsItem;
  
  for (i=0; i<myCount; i++)
  {
    myItem = (nsRangeStore*)mArray.ElementAt(i);
    itsItem = (nsRangeStore*)(aSelState->mArray.ElementAt(i));
    if (!myItem || !itsItem) return PR_FALSE;
    
    nsCOMPtr<nsIDOMRange> myRange, itsRange;
    myItem->GetRange(address_of(myRange));
    itsItem->GetRange(address_of(itsRange));
    if (!myRange || !itsRange) return PR_FALSE;
  
    PRInt16 compResult;
    myRange->CompareBoundaryPoints(nsIDOMRange::START_TO_START, itsRange, &compResult);
    if (compResult) return PR_FALSE;
    myRange->CompareBoundaryPoints(nsIDOMRange::END_TO_END, itsRange, &compResult);
    if (compResult) return PR_FALSE;
  }
  
  return PR_TRUE;
}

void     
nsSelectionState::MakeEmpty()
{
  
  nsRangeStore *item;
  for (PRInt32 i = mArray.Count()-1; i >= 0; --i)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    delete item;
  }
  mArray.Clear();
}

PRBool   
nsSelectionState::IsEmpty()
{
  return (mArray.Count() == 0);
}





nsRangeUpdater::nsRangeUpdater() : mArray(), mLock(PR_FALSE) {}

nsRangeUpdater::~nsRangeUpdater()
{
  
}
  
void 
nsRangeUpdater::RegisterRangeItem(nsRangeStore *aRangeItem)
{
  if (!aRangeItem) return;
  if (mArray.IndexOf(aRangeItem) != -1)
  {
    NS_ERROR("tried to register an already registered range");
    return;  
  }
  mArray.AppendElement(aRangeItem);
}

void 
nsRangeUpdater::DropRangeItem(nsRangeStore *aRangeItem)
{
  if (!aRangeItem) return;
  mArray.RemoveElement(aRangeItem);
}

nsresult 
nsRangeUpdater::RegisterSelectionState(nsSelectionState &aSelState)
{
  PRInt32 i, theCount = aSelState.mArray.Count();
  if (theCount < 1) return NS_ERROR_FAILURE;

  nsRangeStore *item;
  
  for (i=0; i<theCount; i++)
  {
    item = (nsRangeStore*)aSelState.mArray.ElementAt(i);
    RegisterRangeItem(item);
  }

  return NS_OK;
}

nsresult 
nsRangeUpdater::DropSelectionState(nsSelectionState &aSelState)
{
  PRInt32 i, theCount = aSelState.mArray.Count();
  if (theCount < 1) return NS_ERROR_FAILURE;

  nsRangeStore *item;
  
  for (i=0; i<theCount; i++)
  {
    item = (nsRangeStore*)aSelState.mArray.ElementAt(i);
    DropRangeItem(item);
  }

  return NS_OK;
}



nsresult
nsRangeUpdater::SelAdjCreateNode(nsIDOMNode *aParent, PRInt32 aPosition)
{
  if (mLock) return NS_OK;  
  if (!aParent) return NS_ERROR_NULL_POINTER;
  PRInt32 i, count = mArray.Count();
  if (!count) return NS_OK;

  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    if ((item->startNode.get() == aParent) && (item->startOffset > aPosition))
      item->startOffset++;
    if ((item->endNode.get() == aParent) && (item->endOffset > aPosition))
      item->endOffset++;
  }
  return NS_OK;
}

nsresult
nsRangeUpdater::SelAdjInsertNode(nsIDOMNode *aParent, PRInt32 aPosition)
{
  return SelAdjCreateNode(aParent, aPosition);
}


nsresult
nsRangeUpdater::SelAdjDeleteNode(nsIDOMNode *aNode)
{
  if (mLock) return NS_OK;  
  if (!aNode) return NS_ERROR_NULL_POINTER;
  PRInt32 i, count = mArray.Count();
  if (!count) return NS_OK;

  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset = 0;
  
  nsresult res = nsEditor::GetNodeLocation(aNode, address_of(parent), &offset);
  NS_ENSURE_SUCCESS(res, res);
  
  
  nsRangeStore *item;
  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    if ((item->startNode.get() == parent) && (item->startOffset > offset))
      item->startOffset--;
    if ((item->endNode.get() == parent) && (item->endOffset > offset))
      item->endOffset--;
      
    
    if (item->startNode == aNode)
    {
      item->startNode   = parent;
      item->startOffset = offset;
    }
    if (item->endNode == aNode)
    {
      item->endNode   = parent;
      item->endOffset = offset;
    }

    
    nsCOMPtr<nsIDOMNode> oldStart;
    if (nsEditorUtils::IsDescendantOf(item->startNode, aNode))
    {
      oldStart = item->startNode;  
      item->startNode   = parent;
      item->startOffset = offset;
    }

    
    if ((item->endNode == oldStart) || nsEditorUtils::IsDescendantOf(item->endNode, aNode))
    {
      item->endNode   = parent;
      item->endOffset = offset;
    }
  }
  return NS_OK;
}


nsresult
nsRangeUpdater::SelAdjSplitNode(nsIDOMNode *aOldRightNode, PRInt32 aOffset, nsIDOMNode *aNewLeftNode)
{
  if (mLock) return NS_OK;  
  if (!aOldRightNode || !aNewLeftNode) return NS_ERROR_NULL_POINTER;
  PRInt32 i, count = mArray.Count();
  if (!count) return NS_OK;

  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  nsresult result = nsEditor::GetNodeLocation(aOldRightNode, address_of(parent), &offset);
  if (NS_FAILED(result)) return result;
  
  
  result = SelAdjInsertNode(parent,offset-1);
  if (NS_FAILED(result)) return result;

  
  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    if (item->startNode.get() == aOldRightNode)
    {
      if (item->startOffset > aOffset)
      {
        item->startOffset -= aOffset;
      }
      else
      {
        item->startNode = aNewLeftNode;
      }
    }
    if (item->endNode.get() == aOldRightNode)
    {
      if (item->endOffset > aOffset)
      {
        item->endOffset -= aOffset;
      }
      else
      {
        item->endNode = aNewLeftNode;
      }
    }
  }
  return NS_OK;
}


nsresult
nsRangeUpdater::SelAdjJoinNodes(nsIDOMNode *aLeftNode, 
                                  nsIDOMNode *aRightNode, 
                                  nsIDOMNode *aParent, 
                                  PRInt32 aOffset,
                                  PRInt32 aOldLeftNodeLength)
{
  if (mLock) return NS_OK;  
  if (!aLeftNode || !aRightNode || !aParent) return NS_ERROR_NULL_POINTER;
  PRInt32 i, count = mArray.Count();
  if (!count) return NS_OK;

  nsRangeStore *item;

  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    if (item->startNode.get() == aParent)
    {
      
      if (item->startOffset > aOffset)
      {
        item->startOffset--;
      }
      else if (item->startOffset == aOffset)
      {
        
        item->startNode = aRightNode;
        item->startOffset = aOldLeftNodeLength;
      }
    }
    else if (item->startNode.get() == aRightNode)
    {
      
      item->startOffset += aOldLeftNodeLength;
    }
    else if (item->startNode.get() == aLeftNode)
    {
      
      item->startNode = aRightNode;
    }

    if (item->endNode.get() == aParent)
    {
      
      if (item->endOffset > aOffset)
      {
        item->endOffset--;
      }
      else if (item->endOffset == aOffset)
      {
        
        item->endNode = aRightNode;
        item->endOffset = aOldLeftNodeLength;
      }
    }
    else if (item->endNode.get() == aRightNode)
    {
      
       item->endOffset += aOldLeftNodeLength;
    }
    else if (item->endNode.get() == aLeftNode)
    {
      
      item->endNode = aRightNode;
    }
  }
  
  return NS_OK;
}


nsresult
nsRangeUpdater::SelAdjInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsAString &aString)
{
  if (mLock) return NS_OK;  

  PRInt32 count = mArray.Count();
  if (!count) return NS_OK;
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aTextNode));
  if (!node) return NS_ERROR_NULL_POINTER;
  
  PRInt32 len=aString.Length(), i;
  nsRangeStore *item;
  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    if ((item->startNode.get() == node) && (item->startOffset > aOffset))
      item->startOffset += len;
    if ((item->endNode.get() == node) && (item->endOffset > aOffset))
      item->endOffset += len;
  }
  return NS_OK;
}


nsresult
nsRangeUpdater::SelAdjDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength)
{
  if (mLock) return NS_OK;  

  PRInt32 i, count = mArray.Count();
  if (!count) return NS_OK;
  nsRangeStore *item;
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aTextNode));
  if (!node) return NS_ERROR_NULL_POINTER;
  
  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    if ((item->startNode.get() == node) && (item->startOffset > aOffset))
    {
      item->startOffset -= aLength;
      if (item->startOffset < 0) item->startOffset = 0;
    }
    if ((item->endNode.get() == node) && (item->endOffset > aOffset))
    {
      item->endOffset -= aLength;
      if (item->endOffset < 0) item->endOffset = 0;
    }
  }
  return NS_OK;
}


nsresult
nsRangeUpdater::WillReplaceContainer()
{
  if (mLock) return NS_ERROR_UNEXPECTED;  
  mLock = PR_TRUE;
  return NS_OK;
}


nsresult
nsRangeUpdater::DidReplaceContainer(nsIDOMNode *aOriginalNode, nsIDOMNode *aNewNode)
{
  if (!mLock) return NS_ERROR_UNEXPECTED;  
  mLock = PR_FALSE;

  if (!aOriginalNode || !aNewNode) return NS_ERROR_NULL_POINTER;
  PRInt32 i, count = mArray.Count();
  if (!count) return NS_OK;

  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    if (item->startNode.get() == aOriginalNode)
      item->startNode = aNewNode;
    if (item->endNode.get() == aOriginalNode)
      item->endNode = aNewNode;
  }
  return NS_OK;
}


nsresult
nsRangeUpdater::WillRemoveContainer()
{
  if (mLock) return NS_ERROR_UNEXPECTED;  
  mLock = PR_TRUE;
  return NS_OK;
}


nsresult
nsRangeUpdater::DidRemoveContainer(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aOffset, PRUint32 aNodeOrigLen)
{
  if (!mLock) return NS_ERROR_UNEXPECTED;  
  mLock = PR_FALSE;

  if (!aNode || !aParent) return NS_ERROR_NULL_POINTER;
  PRInt32 i, count = mArray.Count();
  if (!count) return NS_OK;

  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    if (item->startNode.get() == aNode)
    {
      item->startNode = aParent;
      item->startOffset += aOffset;
    }
    else if ((item->startNode.get() == aParent) && (item->startOffset > aOffset))
      item->startOffset += (PRInt32)aNodeOrigLen-1;
      
    if (item->endNode.get() == aNode)
    {
      item->endNode = aParent;
      item->endOffset += aOffset;
    }
    else if ((item->endNode.get() == aParent) && (item->endOffset > aOffset))
      item->endOffset += (PRInt32)aNodeOrigLen-1;
  }
  return NS_OK;
}


nsresult
nsRangeUpdater::WillInsertContainer()
{
  if (mLock) return NS_ERROR_UNEXPECTED;  
  mLock = PR_TRUE;
  return NS_OK;
}


nsresult
nsRangeUpdater::DidInsertContainer()
{
  if (!mLock) return NS_ERROR_UNEXPECTED;  
  mLock = PR_FALSE;
  return NS_OK;
}


nsresult
nsRangeUpdater::WillMoveNode()
{
  if (mLock) return NS_ERROR_UNEXPECTED;  
  mLock = PR_TRUE;
  return NS_OK;
}


nsresult
nsRangeUpdater::DidMoveNode(nsIDOMNode *aOldParent, PRInt32 aOldOffset, nsIDOMNode *aNewParent, PRInt32 aNewOffset)
{
  if (!mLock) return NS_ERROR_UNEXPECTED;  
  mLock = PR_FALSE;

  if (!aOldParent || !aNewParent) return NS_ERROR_NULL_POINTER;
  PRInt32 i, count = mArray.Count();
  if (!count) return NS_OK;

  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = (nsRangeStore*)mArray.ElementAt(i);
    if (!item) return NS_ERROR_NULL_POINTER;
    
    
    if ((item->startNode.get() == aOldParent) && (item->startOffset > aOldOffset))
      item->startOffset--;
    if ((item->endNode.get() == aOldParent) && (item->endOffset > aOldOffset))
      item->endOffset--;
      
    
    if ((item->startNode.get() == aNewParent) && (item->startOffset > aNewOffset))
      item->startOffset++;
    if ((item->endNode.get() == aNewParent) && (item->endOffset > aNewOffset))
      item->endOffset++;
  }
  return NS_OK;
}







  

nsRangeStore::nsRangeStore() 
{ 
  
}
nsRangeStore::~nsRangeStore()
{
  
}

nsresult nsRangeStore::StoreRange(nsIDOMRange *aRange)
{
  if (!aRange) return NS_ERROR_NULL_POINTER;
  aRange->GetStartContainer(getter_AddRefs(startNode));
  aRange->GetEndContainer(getter_AddRefs(endNode));
  aRange->GetStartOffset(&startOffset);
  aRange->GetEndOffset(&endOffset);
  return NS_OK;
}

nsresult nsRangeStore::GetRange(nsCOMPtr<nsIDOMRange> *outRange)
{
  if (!outRange) return NS_ERROR_NULL_POINTER;
  nsresult res;
  *outRange = do_CreateInstance("@mozilla.org/content/range;1", &res);
  if(NS_FAILED(res)) return res;

  res = (*outRange)->SetStart(startNode, startOffset);
  if(NS_FAILED(res)) return res;

  res = (*outRange)->SetEnd(endNode, endOffset);
  return res;
}
