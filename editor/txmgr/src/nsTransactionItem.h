




#ifndef nsTransactionItem_h__
#define nsTransactionItem_h__

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"
#include "nscore.h"
#include "prtypes.h"

class nsITransaction;
class nsTransactionManager;
class nsTransactionStack;

class nsTransactionItem
{
  nsCOMPtr<nsITransaction> mTransaction;
  nsTransactionStack      *mUndoStack;
  nsTransactionStack      *mRedoStack;
  nsAutoRefCnt             mRefCnt;

public:

  nsTransactionItem(nsITransaction *aTransaction);
  virtual ~nsTransactionItem();
  nsrefcnt AddRef();
  nsrefcnt Release();

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsTransactionItem)

  virtual nsresult AddChild(nsTransactionItem *aTransactionItem);
  already_AddRefed<nsITransaction> GetTransaction();
  virtual nsresult GetIsBatch(bool *aIsBatch);
  virtual nsresult GetNumberOfChildren(int32_t *aNumChildren);
  virtual nsresult GetChild(int32_t aIndex, nsTransactionItem **aChild);

  virtual nsresult DoTransaction(void);
  virtual nsresult UndoTransaction(nsTransactionManager *aTxMgr);
  virtual nsresult RedoTransaction(nsTransactionManager *aTxMgr);

private:

  virtual nsresult UndoChildren(nsTransactionManager *aTxMgr);
  virtual nsresult RedoChildren(nsTransactionManager *aTxMgr);

  virtual nsresult RecoverFromUndoError(nsTransactionManager *aTxMgr);
  virtual nsresult RecoverFromRedoError(nsTransactionManager *aTxMgr);

  virtual nsresult GetNumberOfUndoItems(int32_t *aNumItems);
  virtual nsresult GetNumberOfRedoItems(int32_t *aNumItems);
};

#endif 
