




#include "mozilla/mozalloc.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsID.h"
#include "nsISupportsUtils.h"
#include "nsITransactionManager.h"
#include "nsTransactionItem.h"
#include "nsTransactionList.h"
#include "nsTransactionStack.h"
#include "nscore.h"
#include "prtypes.h"

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
  NS_ENSURE_TRUE(aNumItems, NS_ERROR_NULL_POINTER);

  *aNumItems = 0;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  NS_ENSURE_TRUE(txMgr, NS_ERROR_FAILURE);

  nsresult result = NS_OK;

  if (mTxnStack)
    *aNumItems = mTxnStack->GetSize();
  else if (mTxnItem)
    result = mTxnItem->GetNumberOfChildren(aNumItems);

  return result;
}


NS_IMETHODIMP nsTransactionList::ItemIsBatch(PRInt32 aIndex, bool *aIsBatch)
{
  NS_ENSURE_TRUE(aIsBatch, NS_ERROR_NULL_POINTER);

  *aIsBatch = false;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  NS_ENSURE_TRUE(txMgr, NS_ERROR_FAILURE);

  nsRefPtr<nsTransactionItem> item;

  nsresult result = NS_OK;

  if (mTxnStack)
    item = mTxnStack->GetItem(aIndex);
  else if (mTxnItem)
    result = mTxnItem->GetChild(aIndex, getter_AddRefs(item));

  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);

  return item->GetIsBatch(aIsBatch);
}


NS_IMETHODIMP nsTransactionList::GetItem(PRInt32 aIndex, nsITransaction **aItem)
{
  NS_ENSURE_TRUE(aItem, NS_ERROR_NULL_POINTER);

  *aItem = 0;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  NS_ENSURE_TRUE(txMgr, NS_ERROR_FAILURE);

  nsRefPtr<nsTransactionItem> item;

  nsresult result = NS_OK;

  if (mTxnStack)
    item = mTxnStack->GetItem(aIndex);
  else if (mTxnItem)
    result = mTxnItem->GetChild(aIndex, getter_AddRefs(item));

  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);

  *aItem = item->GetTransaction().get();

  return NS_OK;
}


NS_IMETHODIMP nsTransactionList::GetNumChildrenForItem(PRInt32 aIndex, PRInt32 *aNumChildren)
{
  NS_ENSURE_TRUE(aNumChildren, NS_ERROR_NULL_POINTER);

  *aNumChildren = 0;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  NS_ENSURE_TRUE(txMgr, NS_ERROR_FAILURE);

  nsRefPtr<nsTransactionItem> item;

  nsresult result = NS_OK;

  if (mTxnStack)
    item = mTxnStack->GetItem(aIndex);
  else if (mTxnItem)
    result = mTxnItem->GetChild(aIndex, getter_AddRefs(item));

  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);

  return item->GetNumberOfChildren(aNumChildren);
}


NS_IMETHODIMP nsTransactionList::GetChildListForItem(PRInt32 aIndex, nsITransactionList **aTxnList)
{
  NS_ENSURE_TRUE(aTxnList, NS_ERROR_NULL_POINTER);

  *aTxnList = 0;

  nsCOMPtr<nsITransactionManager> txMgr = do_QueryReferent(mTxnMgr);

  NS_ENSURE_TRUE(txMgr, NS_ERROR_FAILURE);

  nsRefPtr<nsTransactionItem> item;

  nsresult result = NS_OK;

  if (mTxnStack)
    item = mTxnStack->GetItem(aIndex);
  else if (mTxnItem)
    result = mTxnItem->GetChild(aIndex, getter_AddRefs(item));

  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);

  *aTxnList = (nsITransactionList *)new nsTransactionList(txMgr, item);

  NS_ENSURE_TRUE(*aTxnList, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aTxnList);

  return NS_OK;
}

