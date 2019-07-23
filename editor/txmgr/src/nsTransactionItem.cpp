




































#include "nsITransaction.h"
#include "nsTransactionStack.h"
#include "nsTransactionManager.h"
#include "nsTransactionItem.h"
#include "nsCOMPtr.h"

nsTransactionItem::nsTransactionItem(nsITransaction *aTransaction)
    : mTransaction(aTransaction), mUndoStack(0), mRedoStack(0)
{
}

nsTransactionItem::~nsTransactionItem()
{
  if (mRedoStack)
    delete mRedoStack;

  if (mUndoStack)
    delete mUndoStack;

  NS_IF_RELEASE(mTransaction);
}

nsresult
nsTransactionItem::AddChild(nsTransactionItem *aTransactionItem)
{
  if (!aTransactionItem)
    return NS_ERROR_NULL_POINTER;

  if (!mUndoStack) {
    mUndoStack = new nsTransactionStack();
    if (!mUndoStack)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  mUndoStack->Push(aTransactionItem);

  return NS_OK;
}

nsresult
nsTransactionItem::GetTransaction(nsITransaction **aTransaction)
{
  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  *aTransaction = mTransaction;

  return NS_OK;
}

nsresult
nsTransactionItem::GetIsBatch(PRBool *aIsBatch)
{
  if (!aIsBatch)
    return NS_ERROR_NULL_POINTER;

  *aIsBatch = !mTransaction;

  return NS_OK;
}

nsresult
nsTransactionItem::GetNumberOfChildren(PRInt32 *aNumChildren)
{
  nsresult result;

  if (!aNumChildren)
    return NS_ERROR_NULL_POINTER;

  *aNumChildren = 0;

  PRInt32 ui = 0;
  PRInt32 ri = 0;

  result = GetNumberOfUndoItems(&ui);

  if (NS_FAILED(result))
    return result;

  result = GetNumberOfRedoItems(&ri);

  if (NS_FAILED(result))
    return result;

  *aNumChildren = ui + ri;

  return NS_OK;
}

nsresult
nsTransactionItem::GetChild(PRInt32 aIndex, nsTransactionItem **aChild)
{
  if (!aChild)
    return NS_ERROR_NULL_POINTER;

  *aChild = 0;

  PRInt32 numItems = 0;
  nsresult result = GetNumberOfChildren(&numItems);

  if (NS_FAILED(result))
    return result;

  if (aIndex < 0 || aIndex >= numItems)
    return NS_ERROR_FAILURE;

  
  
  
  

  result = GetNumberOfUndoItems(&numItems);

  if (NS_FAILED(result))
    return result;

  if (numItems > 0 && aIndex < numItems) {
    if (!mUndoStack)
      return NS_ERROR_FAILURE;

    return mUndoStack->GetItem(aIndex, aChild);
  }

  

  aIndex -=  numItems;

  result = GetNumberOfRedoItems(&numItems);

  if (NS_FAILED(result))
    return result;

  if (!mRedoStack || numItems == 0 || aIndex >= numItems)
      return NS_ERROR_FAILURE;

  return mRedoStack->GetItem(numItems - aIndex - 1, aChild);
}

nsresult
nsTransactionItem::DoTransaction()
{
  if (mTransaction)
    return mTransaction->DoTransaction();
  return NS_OK;
}

nsresult
nsTransactionItem::UndoTransaction(nsTransactionManager *aTxMgr)
{
  nsresult result = UndoChildren(aTxMgr);

  if (NS_FAILED(result)) {
    RecoverFromUndoError(aTxMgr);
    return result;
  }

  if (!mTransaction)
    return NS_OK;

  result = mTransaction->UndoTransaction();

  if (NS_FAILED(result)) {
    RecoverFromUndoError(aTxMgr);
    return result;
  }

  return NS_OK;
}

nsresult
nsTransactionItem::UndoChildren(nsTransactionManager *aTxMgr)
{
  nsTransactionItem *item;
  nsresult result = NS_OK;
  PRInt32 sz = 0;

  if (mUndoStack) {
    if (!mRedoStack && mUndoStack) {
      mRedoStack = new nsTransactionRedoStack();
      if (!mRedoStack)
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    result = mUndoStack->GetSize(&sz);

    if (NS_FAILED(result))
      return result;

    while (sz-- > 0) {
      result = mUndoStack->Peek(&item);

      if (NS_FAILED(result)) {
        return result;
      }

      nsITransaction *t = 0;

      result = item->GetTransaction(&t);

      if (NS_FAILED(result)) {
        return result;
      }

      PRBool doInterrupt = PR_FALSE;

      result = aTxMgr->WillUndoNotify(t, &doInterrupt);

      if (NS_FAILED(result)) {
        return result;
      }

      if (doInterrupt) {
        return NS_OK;
      }

      result = item->UndoTransaction(aTxMgr);

      if (NS_SUCCEEDED(result)) {
        result = mUndoStack->Pop(&item);

        if (NS_SUCCEEDED(result)) {
          result = mRedoStack->Push(item);

          


        }
      }

      nsresult result2 = aTxMgr->DidUndoNotify(t, result);

      if (NS_SUCCEEDED(result)) {
        result = result2;
      }
    }
  }

  return result;
}

nsresult
nsTransactionItem::RedoTransaction(nsTransactionManager *aTxMgr)
{
  nsresult result;

  if (mTransaction) {
    result = mTransaction->RedoTransaction();

    if (NS_FAILED(result))
      return result;
  }

  result = RedoChildren(aTxMgr);

  if (NS_FAILED(result)) {
    RecoverFromRedoError(aTxMgr);
    return result;
  }

  return NS_OK;
}

nsresult
nsTransactionItem::RedoChildren(nsTransactionManager *aTxMgr)
{
  nsTransactionItem *item;
  nsresult result = NS_OK;
  PRInt32 sz = 0;

  if (!mRedoStack)
    return NS_OK;

  
  result = mRedoStack->GetSize(&sz);

  if (NS_FAILED(result))
    return result;


  while (sz-- > 0) {
    result = mRedoStack->Peek(&item);

    if (NS_FAILED(result)) {
      return result;
    }

    nsITransaction *t = 0;

    result = item->GetTransaction(&t);

    if (NS_FAILED(result)) {
      return result;
    }

    PRBool doInterrupt = PR_FALSE;

    result = aTxMgr->WillRedoNotify(t, &doInterrupt);

    if (NS_FAILED(result)) {
      return result;
    }

    if (doInterrupt) {
      return NS_OK;
    }

    result = item->RedoTransaction(aTxMgr);

    if (NS_SUCCEEDED(result)) {
      result = mRedoStack->Pop(&item);

      if (NS_SUCCEEDED(result)) {
        result = mUndoStack->Push(item);

        
        
      }
    }

    nsresult result2 = aTxMgr->DidUndoNotify(t, result);

    if (NS_SUCCEEDED(result)) {
      result = result2;
    }
  }

  return result;
}

nsresult
nsTransactionItem::GetNumberOfUndoItems(PRInt32 *aNumItems)
{
  if (!aNumItems)
    return NS_ERROR_NULL_POINTER;

  if (!mUndoStack) {
    *aNumItems = 0;
    return NS_OK;
  }

  return mUndoStack->GetSize(aNumItems);
}

nsresult
nsTransactionItem::GetNumberOfRedoItems(PRInt32 *aNumItems)
{
  if (!aNumItems)
    return NS_ERROR_NULL_POINTER;

  if (!mRedoStack) {
    *aNumItems = 0;
    return NS_OK;
  }

  return mRedoStack->GetSize(aNumItems);
}

nsresult
nsTransactionItem::RecoverFromUndoError(nsTransactionManager *aTxMgr)
{
  
  
  
  
  
  return RedoChildren(aTxMgr);
}

nsresult
nsTransactionItem::RecoverFromRedoError(nsTransactionManager *aTxMgr)
{
  
  
  
  
  
  

  nsresult result;

  result = UndoChildren(aTxMgr);

  if (NS_FAILED(result)) {
    return result;
  }

  if (!mTransaction)
    return NS_OK;

  return mTransaction->UndoTransaction();
}

