




#include "mozilla/Assertions.h"         
#include "mozilla/Selection.h"          
#include "nsAString.h"                  
#include "nsAutoPtr.h"                  
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsEditorUtils.h"              
#include "nsError.h"                    
#include "nsIDOMCharacterData.h"        
#include "nsIDOMNode.h"                 
#include "nsIDOMRange.h"                
#include "nsISelection.h"               
#include "nsISupportsImpl.h"            
#include "nsRange.h"                    
#include "nsSelectionState.h"

using namespace mozilla;






nsSelectionState::nsSelectionState() : mArray(){}

nsSelectionState::~nsSelectionState() 
{
  MakeEmpty();
}

void
nsSelectionState::DoTraverse(nsCycleCollectionTraversalCallback &cb)
{
  for (uint32_t i = 0, iEnd = mArray.Length(); i < iEnd; ++i)
  {
    nsRangeStore* item = mArray[i];
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb,
                                       "selection state mArray[i].startNode");
    cb.NoteXPCOMChild(item->startNode);
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb,
                                       "selection state mArray[i].endNode");
    cb.NoteXPCOMChild(item->endNode);
  }
}

void
nsSelectionState::SaveSelection(Selection* aSel)
{
  MOZ_ASSERT(aSel);
  int32_t arrayCount = mArray.Length();
  int32_t rangeCount = aSel->GetRangeCount();

  
  if (arrayCount < rangeCount) {
    for (int32_t i = arrayCount; i < rangeCount; i++) {
      mArray.AppendElement();
      mArray[i] = new nsRangeStore();
    }
  } else if (arrayCount > rangeCount) {
    
    for (int32_t i = arrayCount - 1; i >= rangeCount; i--) {
      mArray.RemoveElementAt(i);
    }
  }

  
  for (int32_t i = 0; i < rangeCount; i++) {
    mArray[i]->StoreRange(aSel->GetRangeAt(i));
  }
}

nsresult  
nsSelectionState::RestoreSelection(nsISelection *aSel)
{
  NS_ENSURE_TRUE(aSel, NS_ERROR_NULL_POINTER);
  nsresult res;
  uint32_t i, arrayCount = mArray.Length();

  
  aSel->RemoveAllRanges();
  
  
  for (i=0; i<arrayCount; i++)
  {
    nsRefPtr<nsRange> range;
    mArray[i]->GetRange(getter_AddRefs(range));
    NS_ENSURE_TRUE(range, NS_ERROR_UNEXPECTED);
   
    res = aSel->AddRange(range);
    if(NS_FAILED(res)) return res;

  }
  return NS_OK;
}

bool
nsSelectionState::IsCollapsed()
{
  if (1 != mArray.Length()) return false;
  nsRefPtr<nsRange> range;
  mArray[0]->GetRange(getter_AddRefs(range));
  NS_ENSURE_TRUE(range, false);
  bool bIsCollapsed = false;
  range->GetCollapsed(&bIsCollapsed);
  return bIsCollapsed;
}

bool
nsSelectionState::IsEqual(nsSelectionState *aSelState)
{
  NS_ENSURE_TRUE(aSelState, false);
  uint32_t i, myCount = mArray.Length(), itsCount = aSelState->mArray.Length();
  if (myCount != itsCount) return false;
  if (myCount < 1) return false;

  for (i=0; i<myCount; i++)
  {
    nsRefPtr<nsRange> myRange, itsRange;
    mArray[i]->GetRange(getter_AddRefs(myRange));
    aSelState->mArray[i]->GetRange(getter_AddRefs(itsRange));
    NS_ENSURE_TRUE(myRange && itsRange, false);
  
    int16_t compResult;
    nsresult rv;
    rv = myRange->CompareBoundaryPoints(nsIDOMRange::START_TO_START, itsRange, &compResult);
    if (NS_FAILED(rv) || compResult) return false;
    rv = myRange->CompareBoundaryPoints(nsIDOMRange::END_TO_END, itsRange, &compResult);
    if (NS_FAILED(rv) || compResult) return false;
  }
  
  return true;
}

void     
nsSelectionState::MakeEmpty()
{
  
  mArray.Clear();
}

bool     
nsSelectionState::IsEmpty()
{
  return mArray.IsEmpty();
}





nsRangeUpdater::nsRangeUpdater() : mArray(), mLock(false) {}

nsRangeUpdater::~nsRangeUpdater()
{
  
}
  
void 
nsRangeUpdater::RegisterRangeItem(nsRangeStore *aRangeItem)
{
  if (!aRangeItem) return;
  if (mArray.Contains(aRangeItem))
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
  uint32_t i, theCount = aSelState.mArray.Length();
  if (theCount < 1) return NS_ERROR_FAILURE;

  for (i=0; i<theCount; i++)
  {
    RegisterRangeItem(aSelState.mArray[i]);
  }

  return NS_OK;
}

nsresult 
nsRangeUpdater::DropSelectionState(nsSelectionState &aSelState)
{
  uint32_t i, theCount = aSelState.mArray.Length();
  if (theCount < 1) return NS_ERROR_FAILURE;

  for (i=0; i<theCount; i++)
  {
    DropRangeItem(aSelState.mArray[i]);
  }

  return NS_OK;
}



nsresult
nsRangeUpdater::SelAdjCreateNode(nsIDOMNode *aParent, int32_t aPosition)
{
  if (mLock) return NS_OK;  
  NS_ENSURE_TRUE(aParent, NS_ERROR_NULL_POINTER);
  uint32_t i, count = mArray.Length();
  if (!count) {
    return NS_OK;
  }

  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = mArray[i];
    NS_ENSURE_TRUE(item, NS_ERROR_NULL_POINTER);
    
    if ((item->startNode.get() == aParent) && (item->startOffset > aPosition))
      item->startOffset++;
    if ((item->endNode.get() == aParent) && (item->endOffset > aPosition))
      item->endOffset++;
  }
  return NS_OK;
}

nsresult
nsRangeUpdater::SelAdjInsertNode(nsIDOMNode *aParent, int32_t aPosition)
{
  return SelAdjCreateNode(aParent, aPosition);
}

void
nsRangeUpdater::SelAdjDeleteNode(nsIDOMNode *aNode)
{
  if (mLock) {
    
    return;
  }
  MOZ_ASSERT(aNode);
  uint32_t i, count = mArray.Length();
  if (!count) {
    return;
  }

  int32_t offset = 0;
  nsCOMPtr<nsIDOMNode> parent = nsEditor::GetNodeLocation(aNode, &offset);
  
  
  nsRangeStore *item;
  for (i=0; i<count; i++)
  {
    item = mArray[i];
    MOZ_ASSERT(item);
    
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
}


nsresult
nsRangeUpdater::SelAdjSplitNode(nsIDOMNode *aOldRightNode, int32_t aOffset, nsIDOMNode *aNewLeftNode)
{
  if (mLock) return NS_OK;  
  NS_ENSURE_TRUE(aOldRightNode && aNewLeftNode, NS_ERROR_NULL_POINTER);
  uint32_t i, count = mArray.Length();
  if (!count) {
    return NS_OK;
  }

  int32_t offset;
  nsCOMPtr<nsIDOMNode> parent = nsEditor::GetNodeLocation(aOldRightNode, &offset);
  
  
  nsresult result = SelAdjInsertNode(parent,offset-1);
  NS_ENSURE_SUCCESS(result, result);

  
  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = mArray[i];
    NS_ENSURE_TRUE(item, NS_ERROR_NULL_POINTER);
    
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
                                  int32_t aOffset,
                                  int32_t aOldLeftNodeLength)
{
  if (mLock) return NS_OK;  
  NS_ENSURE_TRUE(aLeftNode && aRightNode && aParent, NS_ERROR_NULL_POINTER);
  uint32_t i, count = mArray.Length();
  if (!count) {
    return NS_OK;
  }

  nsRangeStore *item;

  for (i=0; i<count; i++)
  {
    item = mArray[i];
    NS_ENSURE_TRUE(item, NS_ERROR_NULL_POINTER);
    
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
nsRangeUpdater::SelAdjInsertText(nsIDOMCharacterData *aTextNode, int32_t aOffset, const nsAString &aString)
{
  if (mLock) return NS_OK;  

  uint32_t count = mArray.Length();
  if (!count) {
    return NS_OK;
  }
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aTextNode));
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);
  
  uint32_t len=aString.Length(), i;
  nsRangeStore *item;
  for (i=0; i<count; i++)
  {
    item = mArray[i];
    NS_ENSURE_TRUE(item, NS_ERROR_NULL_POINTER);
    
    if ((item->startNode.get() == node) && (item->startOffset > aOffset))
      item->startOffset += len;
    if ((item->endNode.get() == node) && (item->endOffset > aOffset))
      item->endOffset += len;
  }
  return NS_OK;
}


nsresult
nsRangeUpdater::SelAdjDeleteText(nsIDOMCharacterData *aTextNode, int32_t aOffset, int32_t aLength)
{
  if (mLock) return NS_OK;  

  uint32_t i, count = mArray.Length();
  if (!count) {
    return NS_OK;
  }
  nsRangeStore *item;
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aTextNode));
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);
  
  for (i=0; i<count; i++)
  {
    item = mArray[i];
    NS_ENSURE_TRUE(item, NS_ERROR_NULL_POINTER);
    
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
  mLock = true;
  return NS_OK;
}


nsresult
nsRangeUpdater::DidReplaceContainer(nsIDOMNode *aOriginalNode, nsIDOMNode *aNewNode)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_UNEXPECTED);  
  mLock = false;

  NS_ENSURE_TRUE(aOriginalNode && aNewNode, NS_ERROR_NULL_POINTER);
  uint32_t i, count = mArray.Length();
  if (!count) {
    return NS_OK;
  }

  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = mArray[i];
    NS_ENSURE_TRUE(item, NS_ERROR_NULL_POINTER);
    
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
  mLock = true;
  return NS_OK;
}


nsresult
nsRangeUpdater::DidRemoveContainer(nsIDOMNode *aNode, nsIDOMNode *aParent, int32_t aOffset, uint32_t aNodeOrigLen)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_UNEXPECTED);  
  mLock = false;

  NS_ENSURE_TRUE(aNode && aParent, NS_ERROR_NULL_POINTER);
  uint32_t i, count = mArray.Length();
  if (!count) {
    return NS_OK;
  }

  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = mArray[i];
    NS_ENSURE_TRUE(item, NS_ERROR_NULL_POINTER);
    
    if (item->startNode.get() == aNode)
    {
      item->startNode = aParent;
      item->startOffset += aOffset;
    }
    else if ((item->startNode.get() == aParent) && (item->startOffset > aOffset))
      item->startOffset += (int32_t)aNodeOrigLen-1;
      
    if (item->endNode.get() == aNode)
    {
      item->endNode = aParent;
      item->endOffset += aOffset;
    }
    else if ((item->endNode.get() == aParent) && (item->endOffset > aOffset))
      item->endOffset += (int32_t)aNodeOrigLen-1;
  }
  return NS_OK;
}


nsresult
nsRangeUpdater::WillInsertContainer()
{
  if (mLock) return NS_ERROR_UNEXPECTED;  
  mLock = true;
  return NS_OK;
}


nsresult
nsRangeUpdater::DidInsertContainer()
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_UNEXPECTED);  
  mLock = false;
  return NS_OK;
}


nsresult
nsRangeUpdater::WillMoveNode()
{
  if (mLock) return NS_ERROR_UNEXPECTED;  
  mLock = true;
  return NS_OK;
}


nsresult
nsRangeUpdater::DidMoveNode(nsIDOMNode *aOldParent, int32_t aOldOffset, nsIDOMNode *aNewParent, int32_t aNewOffset)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_UNEXPECTED);  
  mLock = false;

  NS_ENSURE_TRUE(aOldParent && aNewParent, NS_ERROR_NULL_POINTER);
  uint32_t i, count = mArray.Length();
  if (!count) {
    return NS_OK;
  }

  nsRangeStore *item;
  
  for (i=0; i<count; i++)
  {
    item = mArray[i];
    NS_ENSURE_TRUE(item, NS_ERROR_NULL_POINTER);
    
    
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
  NS_ENSURE_TRUE(aRange, NS_ERROR_NULL_POINTER);
  aRange->GetStartContainer(getter_AddRefs(startNode));
  aRange->GetEndContainer(getter_AddRefs(endNode));
  aRange->GetStartOffset(&startOffset);
  aRange->GetEndOffset(&endOffset);
  return NS_OK;
}

nsresult nsRangeStore::GetRange(nsRange** outRange)
{
  return nsRange::CreateRange(startNode, startOffset, endNode, endOffset,
                              outRange);
}
