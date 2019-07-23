




































#include "nsITransactionManager.h"
#include "nsTransactionItem.h"
#include "nsTransactionList.h"
#include "nsTransactionStack.h"

NS_IMPL_ISUPPORTS1(nsTransactionList, nsITransactionList)

nsTransactionList::nsTransactionList(nsITransactionManager *aTxnMgr, nsTransactionStack *aTxnStack)
  : mTxnStack(aTxnStack)
  , mTxnItem(0)
{
  if (aTxnMgr)
    mTxnMgr = do_GetWeakReference(aTxnMgr);
}

nsTransactionList::nsTransactionList(nsITransactionManager *aTxnMgr, nsTransactionItem *aTxnItem)
  : mTxnStack(0)
  , mTxnItem(aTxnItem)
{
  if (aTxnMgr)
    mTxnMgr = do_GetWeakReference(aTxnMgr);
}

nsTransactionList::~nsTransactionList()
{
  mTxnStack = 0;
  mTxnItem  = 0;
}


NS_IMETHODIMP nsTransactionList::GetNumItems(PRInt32 *aNumItems)
{
  if (!aNumItems)
    return NS_ERROR_NULL_POINTER;

  *aNumItems = 0;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  if (!txMgr)
    return NS_ERROR_FAILURE;

  nsresult result = NS_ERROR_FAILURE;

  if (mTxnStack)
    result = mTxnStack->GetSize(aNumItems);
  else if (mTxnItem)
    result = mTxnItem->GetNumberOfChildren(aNumItems);

  return result;
}


NS_IMETHODIMP nsTransactionList::ItemIsBatch(PRInt32 aIndex, PRBool *aIsBatch)
{
  if (!aIsBatch)
    return NS_ERROR_NULL_POINTER;

  *aIsBatch = PR_FALSE;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  if (!txMgr)
    return NS_ERROR_FAILURE;

  nsTransactionItem *item = 0;

  nsresult result = NS_ERROR_FAILURE;

  if (mTxnStack)
    result = mTxnStack->GetItem(aIndex, &item);
  else if (mTxnItem)
    result = mTxnItem->GetChild(aIndex, &item);

  if (NS_FAILED(result))
    return result;

  if (!item)
    return NS_ERROR_FAILURE;

  return item->GetIsBatch(aIsBatch);
}


NS_IMETHODIMP nsTransactionList::GetItem(PRInt32 aIndex, nsITransaction **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;

  *aItem = 0;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  if (!txMgr)
    return NS_ERROR_FAILURE;

  nsTransactionItem *item = 0;

  nsresult result = NS_ERROR_FAILURE;

  if (mTxnStack)
    result = mTxnStack->GetItem(aIndex, &item);
  else if (mTxnItem)
    result = mTxnItem->GetChild(aIndex, &item);

  if (NS_FAILED(result))
    return result;

  if (!item)
    return NS_ERROR_FAILURE;

  result = item->GetTransaction(aItem);

  if (NS_FAILED(result))
    return result;

  NS_IF_ADDREF(*aItem);

  return NS_OK;
}


NS_IMETHODIMP nsTransactionList::GetNumChildrenForItem(PRInt32 aIndex, PRInt32 *aNumChildren)
{
  if (!aNumChildren)
    return NS_ERROR_NULL_POINTER;

  *aNumChildren = 0;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  if (!txMgr)
    return NS_ERROR_FAILURE;

  nsTransactionItem *item = 0;

  nsresult result = NS_ERROR_FAILURE;

  if (mTxnStack)
    result = mTxnStack->GetItem(aIndex, &item);
  else if (mTxnItem)
    result = mTxnItem->GetChild(aIndex, &item);

  if (NS_FAILED(result))
    return result;

  if (!item)
    return NS_ERROR_FAILURE;

  return item->GetNumberOfChildren(aNumChildren);
}


NS_IMETHODIMP nsTransactionList::GetChildListForItem(PRInt32 aIndex, nsITransactionList **aTxnList)
{
  if (!aTxnList)
    return NS_ERROR_NULL_POINTER;

  *aTxnList = 0;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  if (!txMgr)
    return NS_ERROR_FAILURE;

  nsTransactionItem *item = 0;

  nsresult result = NS_ERROR_FAILURE;

  if (mTxnStack)
    result = mTxnStack->GetItem(aIndex, &item);
  else if (mTxnItem)
    result = mTxnItem->GetChild(aIndex, &item);

  if (NS_FAILED(result))
    return result;

  if (!item)
    return NS_ERROR_FAILURE;

  *aTxnList = (nsITransactionList *)new nsTransactionList(txMgr, item);

  if (!*aTxnList)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aTxnList);

  return NS_OK;
}

