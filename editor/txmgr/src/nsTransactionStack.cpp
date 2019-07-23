




































#include "nsITransaction.h"
#include "nsTransactionItem.h"
#include "nsTransactionStack.h"
#include "nsCOMPtr.h"

nsTransactionStack::nsTransactionStack()
  : mQue(0)
{
  nsTransactionReleaseFunctor* theFunctor=new nsTransactionReleaseFunctor();
  mQue.SetDeallocator(theFunctor);
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

  *aTransaction = (nsTransactionItem *)(mQue.Last());

  return NS_OK;
}

nsresult
nsTransactionStack::GetItem(PRInt32 aIndex, nsTransactionItem **aTransaction)
{
  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  if (aIndex < 0 || aIndex >= mQue.GetSize())
    return NS_ERROR_FAILURE;

  *aTransaction = (nsTransactionItem *)(mQue.ObjectAt(aIndex));

  return NS_OK;
}

nsresult
nsTransactionStack::Clear(void)
{
  nsTransactionItem *tx = 0;
  nsresult result    = NS_OK;

  

  result = Pop(&tx);

  if (NS_FAILED(result))
    return result;

  while (tx) {
    delete tx;

    result = Pop(&tx);

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

nsTransactionRedoStack::~nsTransactionRedoStack()
{
  Clear();
}

nsresult
nsTransactionRedoStack::Clear(void)
{
  nsTransactionItem *tx = 0;
  nsresult result       = NS_OK;

  



  result = PopBottom(&tx);

  if (NS_FAILED(result))
    return result;

  while (tx) {
    delete tx;

    result = PopBottom(&tx);

    if (NS_FAILED(result))
      return result;
  }

  return NS_OK;
}

void *
nsTransactionReleaseFunctor::operator()(void *aObject)
{
  nsTransactionItem *item = (nsTransactionItem *)aObject;
  delete item;
  return 0;
}
