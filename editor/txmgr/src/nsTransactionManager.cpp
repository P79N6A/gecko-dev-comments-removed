




































#include "nsITransaction.h"
#include "nsITransactionListener.h"

#include "nsTransactionItem.h"
#include "nsTransactionStack.h"
#include "nsVoidArray.h"
#include "nsTransactionManager.h"
#include "nsTransactionList.h"

#include "nsCOMPtr.h"

#define LOCK_TX_MANAGER(mgr)    (mgr)->Lock()
#define UNLOCK_TX_MANAGER(mgr)  (mgr)->Unlock()


nsTransactionManager::nsTransactionManager(PRInt32 aMaxTransactionCount)
  : mMaxTransactionCount(aMaxTransactionCount), mListeners(0)
{
  mMonitor = ::PR_NewMonitor();
}

nsTransactionManager::~nsTransactionManager()
{
  if (mListeners)
  {
    PRInt32 i;
    nsITransactionListener *listener;

    for (i = 0; i < mListeners->Count(); i++)
    {
      listener = (nsITransactionListener *)mListeners->ElementAt(i);
      NS_IF_RELEASE(listener);
    }

    delete mListeners;
    mListeners = 0;
  }

  if (mMonitor)
  {
    ::PR_DestroyMonitor(mMonitor);
    mMonitor = 0;
  }
}

#ifdef DEBUG_TXMGR_REFCNT

nsrefcnt nsTransactionManager::AddRef(void)
{
  return ++mRefCnt;
}

nsrefcnt nsTransactionManager::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  if (--mRefCnt == 0) {
    NS_DELETEXPCOM(this);
    return 0;
  }
  return mRefCnt;
}

NS_IMPL_QUERY_INTERFACE2(nsTransactionManager, nsITransactionManager, nsISupportsWeakReference)

#else

NS_IMPL_ISUPPORTS2(nsTransactionManager, nsITransactionManager, nsISupportsWeakReference)

#endif

NS_IMETHODIMP
nsTransactionManager::DoTransaction(nsITransaction *aTransaction)
{
  nsresult result;

  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

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
  nsTransactionItem *tx = 0;

  LOCK_TX_MANAGER(this);

  
  
  

  result = mDoStack.Peek(&tx);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_ERROR_FAILURE;
  }

  
  
  result = mUndoStack.Peek(&tx);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  
  if (!tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  nsITransaction *t = 0;

  result = tx->GetTransaction(&t);

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
    result = mUndoStack.Pop(&tx);

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
  nsTransactionItem *tx = 0;

  LOCK_TX_MANAGER(this);

  
  
  

  result = mDoStack.Peek(&tx);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_ERROR_FAILURE;
  }

  
  
  result = mRedoStack.Peek(&tx);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  
  if (!tx) {
    UNLOCK_TX_MANAGER(this);
    return NS_OK;
  }

  nsITransaction *t = 0;

  result = tx->GetTransaction(&t);

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
    result = mRedoStack.Pop(&tx);

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
  nsTransactionItem *tx = 0;
  nsITransaction *ti    = 0;
  nsresult result;

  LOCK_TX_MANAGER(this);

  
  
  
  
  
  
  
  
  
  

  result = mDoStack.Peek(&tx);

  if (NS_FAILED(result)) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  if (tx)
    tx->GetTransaction(&ti);

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
  if (!aMaxCount)
    return NS_ERROR_NULL_POINTER;

  LOCK_TX_MANAGER(this);
  *aMaxCount = mMaxTransactionCount;
  UNLOCK_TX_MANAGER(this);

  return NS_OK;
}

NS_IMETHODIMP
nsTransactionManager::SetMaxTransactionCount(PRInt32 aMaxCount)
{
  PRInt32 numUndoItems  = 0, numRedoItems = 0, total = 0;
  nsTransactionItem *tx = 0;
  nsresult result;

  LOCK_TX_MANAGER(this);

  
  
  
  
  

  result = mDoStack.Peek(&tx);

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
    tx = 0;
    result = mUndoStack.PopBottom(&tx);

    if (NS_FAILED(result) || !tx) {
      UNLOCK_TX_MANAGER(this);
      return result;
    }

    delete tx;

    --numUndoItems;
  }

  
  

  while (numRedoItems > 0 && (numRedoItems + numUndoItems) > aMaxCount) {
    tx = 0;
    result = mRedoStack.PopBottom(&tx);

    if (NS_FAILED(result) || !tx) {
      UNLOCK_TX_MANAGER(this);
      return result;
    }

    delete tx;

    --numRedoItems;
  }

  mMaxTransactionCount = aMaxCount;

  UNLOCK_TX_MANAGER(this);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::PeekUndoStack(nsITransaction **aTransaction)
{
  nsTransactionItem *tx = 0;
  nsresult result;

  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  *aTransaction = 0;

  LOCK_TX_MANAGER(this);

  result = mUndoStack.Peek(&tx);

  if (NS_FAILED(result) || !tx) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  result = tx->GetTransaction(aTransaction);

  UNLOCK_TX_MANAGER(this);

  NS_IF_ADDREF(*aTransaction);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::PeekRedoStack(nsITransaction **aTransaction)
{
  nsTransactionItem *tx = 0;
  nsresult result;

  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  *aTransaction = 0;

  LOCK_TX_MANAGER(this);

  result = mRedoStack.Peek(&tx);

  if (NS_FAILED(result) || !tx) {
    UNLOCK_TX_MANAGER(this);
    return result;
  }

  result = tx->GetTransaction(aTransaction);

  UNLOCK_TX_MANAGER(this);

  NS_IF_ADDREF(*aTransaction);

  return result;
}

NS_IMETHODIMP
nsTransactionManager::GetUndoList(nsITransactionList **aTransactionList)
{
  if (!aTransactionList)
    return NS_ERROR_NULL_POINTER;

  *aTransactionList = (nsITransactionList *)new nsTransactionList(this, &mUndoStack);

  NS_IF_ADDREF(*aTransactionList);

  return (! *aTransactionList) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

NS_IMETHODIMP
nsTransactionManager::GetRedoList(nsITransactionList **aTransactionList)
{
  if (!aTransactionList)
    return NS_ERROR_NULL_POINTER;

  *aTransactionList = (nsITransactionList *)new nsTransactionList(this, &mRedoStack);

  NS_IF_ADDREF(*aTransactionList);

  return (! *aTransactionList) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

NS_IMETHODIMP
nsTransactionManager::AddListener(nsITransactionListener *aListener)
{
  if (!aListener)
    return NS_ERROR_NULL_POINTER;

  LOCK_TX_MANAGER(this);

  if (!mListeners) {
    mListeners = new nsAutoVoidArray();

    if (!mListeners) {
      UNLOCK_TX_MANAGER(this);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (!mListeners->AppendElement((void *)aListener)) {
    UNLOCK_TX_MANAGER(this);
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(aListener);

  UNLOCK_TX_MANAGER(this);

  return NS_OK;
}

NS_IMETHODIMP
nsTransactionManager::RemoveListener(nsITransactionListener *aListener)
{
  if (!aListener)
    return NS_ERROR_NULL_POINTER;

  if (!mListeners)
    return NS_ERROR_FAILURE;

  LOCK_TX_MANAGER(this);

  if (!mListeners->RemoveElement((void *)aListener))
  {
    UNLOCK_TX_MANAGER(this);
    return NS_ERROR_FAILURE;
  }

  NS_IF_RELEASE(aListener);

  if (mListeners->Count() < 1)
  {
    delete mListeners;
    mListeners = 0;
  }

  UNLOCK_TX_MANAGER(this);

  return NS_OK;
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
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->WillDo(this, aTransaction, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidDoNotify(nsITransaction *aTransaction, nsresult aDoResult)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->DidDo(this, aTransaction, aDoResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillUndoNotify(nsITransaction *aTransaction, PRBool *aInterrupt)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->WillUndo(this, aTransaction, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidUndoNotify(nsITransaction *aTransaction, nsresult aUndoResult)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->DidUndo(this, aTransaction, aUndoResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillRedoNotify(nsITransaction *aTransaction, PRBool *aInterrupt)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->WillRedo(this, aTransaction, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidRedoNotify(nsITransaction *aTransaction, nsresult aRedoResult)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->DidRedo(this, aTransaction, aRedoResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillBeginBatchNotify(PRBool *aInterrupt)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->WillBeginBatch(this, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidBeginBatchNotify(nsresult aResult)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->DidBeginBatch(this, aResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillEndBatchNotify(PRBool *aInterrupt)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->WillEndBatch(this, aInterrupt);
    
    if (NS_FAILED(result) || *aInterrupt)
      break;
  }

  return result;
}

nsresult
nsTransactionManager::DidEndBatchNotify(nsresult aResult)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->DidEndBatch(this, aResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::WillMergeNotify(nsITransaction *aTop, nsITransaction *aTransaction, PRBool *aInterrupt)
{
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

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
  if (!mListeners)
    return NS_OK;

  nsresult result = NS_OK;
  PRInt32 i, lcount = mListeners->Count();

  for (i = 0; i < lcount; i++)
  {
    nsITransactionListener *listener = (nsITransactionListener *)mListeners->ElementAt(i);

    if (!listener)
      return NS_ERROR_FAILURE;

    result = listener->DidMerge(this, aTop, aTransaction, aDidMerge, aMergeResult);
    
    if (NS_FAILED(result))
      break;
  }

  return result;
}

nsresult
nsTransactionManager::BeginTransaction(nsITransaction *aTransaction)
{
  nsTransactionItem *tx;
  nsresult result = NS_OK;

  
  

  NS_IF_ADDREF(aTransaction);

  
  
  tx = new nsTransactionItem(aTransaction);

  if (!tx) {
    NS_IF_RELEASE(aTransaction);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  result = mDoStack.Push(tx);

  if (NS_FAILED(result)) {
    delete tx;
    return result;
  }

  result = tx->DoTransaction();

  if (NS_FAILED(result)) {
    mDoStack.Pop(&tx);
    delete tx;
    return result;
  }

  return NS_OK;
}

nsresult
nsTransactionManager::EndTransaction()
{
  nsITransaction *tint = 0;
  nsTransactionItem *tx        = 0;
  nsresult result              = NS_OK;

  
  

  result = mDoStack.Pop(&tx);

  if (NS_FAILED(result) || !tx)
    return result;

  result = tx->GetTransaction(&tint);

  if (NS_FAILED(result)) {
    
    return result;
  }

  if (!tint) {
    PRInt32 nc = 0;

    
    

    tx->GetNumberOfChildren(&nc);

    if (!nc) {
      delete tx;
      return result;
    }
  }

  
  

  PRBool isTransient = PR_FALSE;

  if (tint)
    result = tint->GetIsTransient(&isTransient);

  if (NS_FAILED(result) || isTransient || !mMaxTransactionCount) {
    
    
    delete tx;
    return result;
  }

  nsTransactionItem *top = 0;

  
  
  

  result = mDoStack.Peek(&top);
  if (top) {
    result = top->AddChild(tx);

    

    return result;
  }

  

  result = ClearRedoStack();

  if (NS_FAILED(result)) {
    
  }

  
  

  top = 0;
  result = mUndoStack.Peek(&top);

  if (tint && top) {
    PRBool didMerge = PR_FALSE;
    nsITransaction *topTransaction = 0;

    result = top->GetTransaction(&topTransaction);

    if (topTransaction) {

      PRBool doInterrupt = PR_FALSE;

      result = WillMergeNotify(topTransaction, tint, &doInterrupt);

      if (NS_FAILED(result))
        return result;

      if (!doInterrupt) {
        result = topTransaction->Merge(tint, &didMerge);

        nsresult result2 = DidMergeNotify(topTransaction, tint, didMerge, result);

        if (NS_SUCCEEDED(result))
          result = result2;

        if (NS_FAILED(result)) {
          
        }

        if (didMerge) {
          delete tx;
          return result;
        }
      }
    }
  }

  
  

  PRInt32 sz = 0;

  result = mUndoStack.GetSize(&sz);

  if (mMaxTransactionCount > 0 && sz >= mMaxTransactionCount) {
    nsTransactionItem *overflow = 0;

    result = mUndoStack.PopBottom(&overflow);

    

    if (overflow)
      delete overflow;
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

