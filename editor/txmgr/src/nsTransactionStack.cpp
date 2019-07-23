




































#include "nsITransaction.h"
#include "nsTransactionItem.h"
#include "nsTransactionStack.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"

nsTransactionStack::nsTransactionStack()
  : mQue(0)
{
} 

nsTransactionStack::~nsTransactionStack()
{
  Clear();
}

nsresult
nsTransactionStack::Push(nsTransactionItem *aTransaction)
{
  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  


  NS_ADDREF(aTransaction);
  mQue.Push(aTransaction);

  return NS_OK;
}

nsresult
nsTransactionStack::Pop(nsTransactionItem **aTransaction)
{
  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  


  *aTransaction = (nsTransactionItem *)mQue.Pop();

  return NS_OK;
}

nsresult
nsTransactionStack::PopBottom(nsTransactionItem **aTransaction)
{
  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  


  *aTransaction = (nsTransactionItem *)mQue.PopFront();

  return NS_OK;
}

nsresult
nsTransactionStack::Peek(nsTransactionItem **aTransaction)
{
  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  if (!mQue.GetSize()) {
    *aTransaction = 0;
    return NS_OK;
  }

  NS_IF_ADDREF(*aTransaction = static_cast<nsTransactionItem*>(mQue.Last()));

  return NS_OK;
}

nsresult
nsTransactionStack::GetItem(PRInt32 aIndex, nsTransactionItem **aTransaction)
{
  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  if (aIndex < 0 || aIndex >= mQue.GetSize())
    return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aTransaction =
               static_cast<nsTransactionItem*>(mQue.ObjectAt(aIndex)));

  return NS_OK;
}

nsresult
nsTransactionStack::Clear(void)
{
  nsRefPtr<nsTransactionItem> tx;
  nsresult result    = NS_OK;

  

  result = Pop(getter_AddRefs(tx));

  if (NS_FAILED(result))
    return result;

  while (tx) {
    result = Pop(getter_AddRefs(tx));

    if (NS_FAILED(result))
      return result;
  }

  return NS_OK;
}

nsresult
nsTransactionStack::GetSize(PRInt32 *aStackSize)
{
  if (!aStackSize)
    return NS_ERROR_NULL_POINTER;

  *aStackSize = mQue.GetSize();

  return NS_OK;
}

void
nsTransactionStack::DoTraverse(nsCycleCollectionTraversalCallback &cb)
{
  for (PRInt32 i = 0, qcount = mQue.GetSize(); i < qcount; ++i) {
    nsTransactionItem *item =
      static_cast<nsTransactionItem*>(mQue.ObjectAt(i));
    if (item) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "transaction stack mQue[i]");
      cb.NoteNativeChild(item, &NS_CYCLE_COLLECTION_NAME(nsTransactionItem));
    }
  }
}

nsTransactionRedoStack::~nsTransactionRedoStack()
{
  Clear();
}

nsresult
nsTransactionRedoStack::Clear(void)
{
  nsRefPtr<nsTransactionItem> tx;
  nsresult result       = NS_OK;

  



  result = PopBottom(getter_AddRefs(tx));

  if (NS_FAILED(result))
    return result;

  while (tx) {
    result = PopBottom(getter_AddRefs(tx));

    if (NS_FAILED(result))
      return result;
  }

  return NS_OK;
}

