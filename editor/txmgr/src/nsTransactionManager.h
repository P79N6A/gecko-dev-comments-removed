




#ifndef nsTransactionManager_h__
#define nsTransactionManager_h__

#include "nsWeakReference.h"
#include "nsITransactionManager.h"
#include "nsCOMArray.h"
#include "nsITransactionListener.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTransactionStack.h"

class nsITransaction;
class nsITransactionListener;
class nsTransactionItem;




class nsTransactionManager : public nsITransactionManager
                           , public nsSupportsWeakReference
{
private:

  PRInt32                mMaxTransactionCount;
  nsTransactionStack     mDoStack;
  nsTransactionStack     mUndoStack;
  nsTransactionStack     mRedoStack;
  nsCOMArray<nsITransactionListener> mListeners;

public:

  

  nsTransactionManager(PRInt32 aMaxTransactionCount=-1);

  

  virtual ~nsTransactionManager();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsTransactionManager,
                                           nsITransactionManager)

  
  NS_DECL_NSITRANSACTIONMANAGER

  
  virtual nsresult ClearUndoStack(void);
  virtual nsresult ClearRedoStack(void);
  already_AddRefed<nsITransaction> PeekUndoStack();
  already_AddRefed<nsITransaction> PeekRedoStack();

  virtual nsresult WillDoNotify(nsITransaction *aTransaction, bool *aInterrupt);
  virtual nsresult DidDoNotify(nsITransaction *aTransaction, nsresult aExecuteResult);
  virtual nsresult WillUndoNotify(nsITransaction *aTransaction, bool *aInterrupt);
  virtual nsresult DidUndoNotify(nsITransaction *aTransaction, nsresult aUndoResult);
  virtual nsresult WillRedoNotify(nsITransaction *aTransaction, bool *aInterrupt);
  virtual nsresult DidRedoNotify(nsITransaction *aTransaction, nsresult aRedoResult);
  virtual nsresult WillBeginBatchNotify(bool *aInterrupt);
  virtual nsresult DidBeginBatchNotify(nsresult aResult);
  virtual nsresult WillEndBatchNotify(bool *aInterrupt);
  virtual nsresult DidEndBatchNotify(nsresult aResult);
  virtual nsresult WillMergeNotify(nsITransaction *aTop,
                                   nsITransaction *aTransaction,
                                   bool *aInterrupt);
  virtual nsresult DidMergeNotify(nsITransaction *aTop,
                                  nsITransaction *aTransaction,
                                  bool aDidMerge,
                                  nsresult aMergeResult);

private:

  
  virtual nsresult BeginTransaction(nsITransaction *aTransaction);
  virtual nsresult EndTransaction(void);
};

#endif 
