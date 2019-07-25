




































#include "nsITransaction.h"
#include "nsITransactionListener.h"

#include "nsTransactionItem.h"
#include "nsTransactionStack.h"
#include "nsVoidArray.h"
#include "nsTransactionManager.h"
#include "nsTransactionList.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"

#define LOCK_TX_MANAGER(mgr)    (mgr)->Lock()
#define UNLOCK_TX_MANAGER(mgr)  (mgr)->Unlock()


nsTransactionManager::nsTransactionManager(PRInt32 aMaxTransactionCount)
  : mMaxTransactionCount(aMaxTransactionCount)
{
  mMonitor = ::PR_NewMonitor();
}

nsTransactionManager::~nsTransactionManager()
{
  if (mMonitor)
  {
    ::PR_DestroyMonitor(mMonitor);
    mMonitor = 0;
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsTransactionManager)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsTransactionManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mListeners)
  tmp->mDoStack.DoUnlink();
  tmp->mUndoStack.DoUnlink();
  tmp->mRedoStack.DoUnlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsTransactionManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mListeners)
  tmp->mDoStack.DoTraverse(cb);
  tmp->mUndoStack.DoTraverse(cb);
  tmp->mRedoStack.DoTraverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsTransactionManager)
  NS_INTERFACE_MAP_ENTRY(nsITransactionManager)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsITransactionManager)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTransactionManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTransactionManager)

NS_IMETHODIMP
nsTransactionManager::DoTransaction(nsITransaction *aTransaction)
{
  nsresult result;

  NS_ENSURE_TRUE(aTransaction, NS_ERROR_NULL_POINTER);

  LOCK_TX_MANAGER(this);

  PRBool doInterrupt = PR_FALSE;

  result = WillDoNotify(aTransaction, &doInterrupt);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (doInterrupt) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  result = BeginTransaction(aTransaction);

  if (NS_FAILED(result)) {
    DidDoNotify(aTransaction, result);
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  result = EndTransaction();

  nsresult result2 = DidDoNotify(aTransaction, result);

  if (NS_SUCCEEDED(result))
    result = result2;

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::UndoTransaction()
{
  nsresult result       = NS_OK;
  nsRefPtr<nsTransactionItem> tx;

  LOCK_TX_MANAGER(this);

  
  
  

  result = mDoStack.Peek(getter_AddRefs(tx));

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_ERROR_FAILURE;
  }

  
  
  result = mUndoStack.Peek(getter_AddRefs(tx));

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  
  if (!tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  nsCOMPtr<nsITransaction> t;

  result = tx->GetTransaction(getter_AddRefs(t));

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  PRBool doInterrupt = PR_FALSE;

  result = WillUndoNotify(t, &doInterrupt);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (doInterrupt) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  result = tx->UndoTransaction(this);

  if (NS_SUCCEEDED(result)) {
    result = mUndoStack.Pop(getter_AddRefs(tx));

    if (NS_SUCCEEDED(result))
      result = mRedoStack.Push(tx);
  }

  nsresult result2 = DidUndoNotify(t, result);

  if (NS_SUCCEEDED(result))
    result = result2;

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::RedoTransaction()
{
  nsresult result       = NS_OK;
  nsRefPtr<nsTransactionItem> tx;

  LOCK_TX_MANAGER(this);

  
  
  

  result = mDoStack.Peek(getter_AddRefs(tx));

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_ERROR_FAILURE;
  }

  
  
  result = mRedoStack.Peek(getter_AddRefs(tx));

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  
  if (!tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  nsCOMPtr<nsITransaction> t;

  result = tx->GetTransaction(getter_AddRefs(t));

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  PRBool doInterrupt = PR_FALSE;

  result = WillRedoNotify(t, &doInterrupt);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (doInterrupt) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  result = tx->RedoTransaction(this);

  if (NS_SUCCEEDED(result)) {
    result = mRedoStack.Pop(getter_AddRefs(tx));

    if (NS_SUCCEEDED(result))
      result = mUndoStack.Push(tx);
  }

  nsresult result2 = DidRedoNotify(t, result);

  if (NS_SUCCEEDED(result))
    result = result2;

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::Clear()
{
  nsresult result;

  LOCK_TX_MANAGER(this);

  result = ClearRedoStack();

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  result = ClearUndoStack();

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::BeginBatch()
{
  nsresult result;

  
  
  
  

  LOCK_TX_MANAGER(this);

  PRBool doInterrupt = PR_FALSE;

  result = WillBeginBatchNotify(&doInterrupt);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (doInterrupt) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  result = BeginTransaction(0);
  
  nsresult result2 = DidBeginBatchNotify(result);

  if (NS_SUCCEEDED(result))
    result = result2;

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::EndBatch()
{
  nsRefPtr<nsTransactionItem> tx;
  nsCOMPtr<nsITransaction> ti;
  nsresult result;

  LOCK_TX_MANAGER(this);

  
  
  
  
  
  
  
  
  
  

  result = mDoStack.Peek(getter_AddRefs(tx));

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (tx)
    tx->GetTransaction(getter_AddRefs(ti));

  if (!tx || ti) {
    UNLOCK_TX_MANAGER(this);
    return NS_ERROR_FAILURE;
  }

  PRBool doInterrupt = PR_FALSE;

  result = WillEndBatchNotify(&doInterrupt);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (doInterrupt) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  result = EndTransaction();

  nsresult result2 = DidEndBatchNotify(result);

  if (NS_SUCCEEDED(result))
    result = result2;

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::GetNumberOfUndoItems(PRInt32 *aNumItems)
{
  nsresult result;

  LOCK_TX_MANAGER(this);
  result = mUndoStack.GetSize(aNumItems);
  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::GetNumberOfRedoItems(PRInt32 *aNumItems)
{
  nsresult result;

  LOCK_TX_MANAGER(this);
  result = mRedoStack.GetSize(aNumItems);
  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::GetMaxTransactionCount(PRInt32 *aMaxCount)
{
  NS_ENSURE_TRUE(aMaxCount, NS_ERROR_NULL_POINTER);

  LOCK_TX_MANAGER(this);
  *aMaxCount = mMaxTransactionCount;
  UNLOCK_TX_MANAGER(this);

  return NS_OK;
}

NS_IMETHODIMP
nsTransactionManager::SetMaxTransactionCount(PRInt32 aMaxCount)
{
  PRInt32 numUndoItems  = 0, numRedoItems = 0, total = 0;
  nsRefPtr<nsTransactionItem> tx;
  nsresult result;

  LOCK_TX_MANAGER(this);

  
  
  
  
  

  result = mDoStack.Peek(getter_AddRefs(tx));

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_ERROR_FAILURE;
  }

  
  

  if (aMaxCount < 0) {
    mMaxTransactionCount = -1;
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  result = mUndoStack.GetSize(&numUndoItems);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  result = mRedoStack.GetSize(&numRedoItems);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  total = numUndoItems + numRedoItems;

  
  
  

  if (aMaxCount > total ) {
    mMaxTransactionCount = aMaxCount;
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  
  

  while (numUndoItems > 0 && (numRedoItems + numUndoItems) > aMaxCount) {
    result = mUndoStack.PopBottom(getter_AddRefs(tx));

    if (NS_FAILED(result) || !tx) {
      UNLOCK_TX_MANAGER(this);
      return result;
    }

    --numUndoItems;
  }

  
  

  while (numRedoItems > 0 && (numRedoItems + numUndoItems) > aMaxCount) {
    result = mRedoStack.PopBottom(getter_AddRefs(tx));

    if (NS_FAILED(result) || !tx) {
      UNLOCK_TX_MANAGER(this);
      return result;
    }

    --numRedoItems;
  }

  mMaxTransactionCount = aMaxCount;

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::PeekUndoStack(nsITransaction **aTransaction)
{
  nsRefPtr<nsTransactionItem> tx;
  nsresult result;

  NS_ENSURE_TRUE(aTransaction, NS_ERROR_NULL_POINTER);

  *aTransaction = 0;

  LOCK_TX_MANAGER(this);

  result = mUndoStack.Peek(getter_AddRefs(tx));

  if (NS_FAILED(result) || !tx) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  result = tx->GetTransaction(aTransaction);

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::PeekRedoStack(nsITransaction **aTransaction)
{
  nsRefPtr<nsTransactionItem> tx;
  nsresult result;

  NS_ENSURE_TRUE(aTransaction, NS_ERROR_NULL_POINTER);

  *aTransaction = 0;

  LOCK_TX_MANAGER(this);

  result = mRedoStack.Peek(getter_AddRefs(tx));

  if (NS_FAILED(result) || !tx) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  result = tx->GetTransaction(aTransaction);

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::GetUndoList(nsITransactionList **aTransactionList)
{
  NS_ENSURE_TRUE(aTransactionList, NS_ERROR_NULL_POINTER);

  *aTransactionList = (nsITransactionList *)new nsTransactionList(this, &mUndoStack);

  NS_IF_ADDREF(*aTransactionList);

  return (! *aTransactionList) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

NS_IMETHODIMP
nsTransactionManager::GetRedoList(nsITransactionList **aTransactionList)
{
  NS_ENSURE_TRUE(aTransactionList, NS_ERROR_NULL_POINTER);

  *aTransactionList = (nsITransactionList *)new nsTransactionList(this, &mRedoStack);

  NS_IF_ADDREF(*aTransactionList);

  return (! *aTransactionList) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

NS_IMETHODIMP
nsTransactionManager::AddListener(nsITransactionListener *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  LOCK_TX_MANAGER(this);

  nsresult rv = mListeners.AppendObject(aListener) ? NS_OK : NS_ERROR_FAILURE;

  UNLOCK_TX_MANAGER(this);

  return rv;
}

NS_IMETHODIMP
nsTransactionManager::RemoveListener(nsITransactionListener *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  LOCK_TX_MANAGER(this);

  nsresult rv = mListeners.RemoveObject(aListener) ? NS_OK : NS_ERROR_FAILURE;

  UNLOCK_TX_MANAGER(this);

  return rv;
}

nsresult
nsTransactionManager::ClearUndoStack()
{
  nsresult result;

  LOCK_TX_MANAGER(this);
  result = mUndoStack.Clear();
  UNLOCK_TX_MANAGER(this);

  return result;
}

nsresult
nsTransactionManager::ClearRedoStack()
{
  nsresult result;

  LOCK_TX_MANAGER(this);
  result = mRedoStack.Clear();
  UNLOCK_TX_MANAGER(this);

  return result;
}

nsresult
nsTransactionManager::WillDoNotify(nsITransaction *aTransaction, PRBool *aInterrupt)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->WillDo(this, aTransaction, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidDoNotify(nsITransaction *aTransaction, nsresult aDoResult)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->DidDo(this, aTransaction, aDoResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillUndoNotify(nsITransaction *aTransaction, PRBool *aInterrupt)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->WillUndo(this, aTransaction, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidUndoNotify(nsITransaction *aTransaction, nsresult aUndoResult)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->DidUndo(this, aTransaction, aUndoResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillRedoNotify(nsITransaction *aTransaction, PRBool *aInterrupt)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->WillRedo(this, aTransaction, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidRedoNotify(nsITransaction *aTransaction, nsresult aRedoResult)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->DidRedo(this, aTransaction, aRedoResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillBeginBatchNotify(PRBool *aInterrupt)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->WillBeginBatch(this, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidBeginBatchNotify(nsresult aResult)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->DidBeginBatch(this, aResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillEndBatchNotify(PRBool *aInterrupt)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->WillEndBatch(this, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidEndBatchNotify(nsresult aResult)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->DidEndBatch(this, aResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillMergeNotify(nsITransaction *aTop, nsITransaction *aTransaction, PRBool *aInterrupt)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->WillMerge(this, aTop, aTransaction, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidMergeNotify(nsITransaction *aTop,
                                     nsITransaction *aTransaction,
                                     PRBool aDidMerge,
                                     nsresult aMergeResult)
{
  nsresult result = NS_OK;
  for (PRInt32 i = 0, lcount = mListeners.Count(); i < lcount; i++)
  {
    nsITransactionListener *listener = mListeners[i];

    NS_ENSURE_TRUE(listener, NS_ERROR_FAILURE);

    result = listener->DidMerge(this, aTop, aTransaction, aDidMerge, aMergeResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::BeginTransaction(nsITransaction *aTransaction)
{
  nsresult result = NS_OK;

  
  

  
  
  nsRefPtr<nsTransactionItem> tx = new nsTransactionItem(aTransaction);

  if (!tx) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  result = mDoStack.Push(tx);

  if (NS_FAILED(result)) {
    return result;
  }

  result = tx->DoTransaction();

  if (NS_FAILED(result)) {
    mDoStack.Pop(getter_AddRefs(tx));
    return result;
  }

  return NS_OK;
}

nsresult
nsTransactionManager::EndTransaction()
{
  nsCOMPtr<nsITransaction> tint;
  nsRefPtr<nsTransactionItem> tx;
  nsresult result              = NS_OK;

  
  

  result = mDoStack.Pop(getter_AddRefs(tx));

  if (NS_FAILED(result) || !tx)
    return result;

  result = tx->GetTransaction(getter_AddRefs(tint));

  if (NS_FAILED(result)) {
    
    return result;
  }

  if (!tint) {
    PRInt32 nc = 0;

    
    

    tx->GetNumberOfChildren(&nc);

    if (!nc) {
      return result;
    }
  }

  
  

  PRBool isTransient = PR_FALSE;

  if (tint)
    result = tint->GetIsTransient(&isTransient);

  if (NS_FAILED(result) || isTransient || !mMaxTransactionCount) {
    
    
    return result;
  }

  nsRefPtr<nsTransactionItem> top;

  
  
  

  result = mDoStack.Peek(getter_AddRefs(top));
  if (top) {
    result = top->AddChild(tx);

    

    return result;
  }

  

  result = ClearRedoStack();

  if (NS_FAILED(result)) {
    
  }

  
  

  top = 0;
  result = mUndoStack.Peek(getter_AddRefs(top));

  if (tint && top) {
    PRBool didMerge = PR_FALSE;
    nsCOMPtr<nsITransaction> topTransaction;

    result = top->GetTransaction(getter_AddRefs(topTransaction));

    if (topTransaction) {

      PRBool doInterrupt = PR_FALSE;

      result = WillMergeNotify(topTransaction, tint, &doInterrupt);

      NS_ENSURE_SUCCESS(result, result);

      if (!doInterrupt) {
        result = topTransaction->Merge(tint, &didMerge);

        nsresult result2 = DidMergeNotify(topTransaction, tint, didMerge, result);

        if (NS_SUCCEEDED(result))
          result = result2;

        if (NS_FAILED(result)) {
          
        }

        if (didMerge) {
          return result;
        }
      }
    }
  }

  
  

  PRInt32 sz = 0;

  result = mUndoStack.GetSize(&sz);

  if (mMaxTransactionCount > 0 && sz >= mMaxTransactionCount) {
    nsRefPtr<nsTransactionItem> overflow;

    result = mUndoStack.PopBottom(getter_AddRefs(overflow));

    
  }

  

  result = mUndoStack.Push(tx);

  if (NS_FAILED(result)) {
    
    
  }

  return result;
}

nsresult
nsTransactionManager::Lock()
{
  if (mMonitor)
    PR_EnterMonitor(mMonitor);

  return NS_OK;
}

nsresult
nsTransactionManager::Unlock()
{
  if (mMonitor)
    PR_ExitMonitor(mMonitor);

  return NS_OK;
}

