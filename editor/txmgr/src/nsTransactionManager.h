




































#ifndef nsTransactionManager_h__
#define nsTransactionManager_h__

#include "prmon.h"
#include "nsWeakReference.h"
#include "nsITransactionManager.h"

class nsITransaction;
class nsITransactionListener;
class nsTransactionItem;
class nsTransactionStack;
class nsTransactionRedoStack;
class nsVoidArray;




class nsTransactionManager : public nsITransactionManager
                           , public nsSupportsWeakReference
{
private:

  PRInt32                mMaxTransactionCount;
  nsTransactionStack     mDoStack;
  nsTransactionStack     mUndoStack;
  nsTransactionRedoStack mRedoStack;
  nsVoidArray            *mListeners;

  PRMonitor              *mMonitor;

public:

  

  nsTransactionManager(PRInt32 aMaxTransactionCount=-1);

  

  virtual ~nsTransactionManager();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITRANSACTIONMANAGER

  
  virtual nsresult ClearUndoStack(void);
  virtual nsresult ClearRedoStack(void);

  virtual nsresult WillDoNotify(nsITransaction *aTransaction, PRBool *aInterrupt);
  virtual nsresult DidDoNotify(nsITransaction *aTransaction, nsresult aExecuteResult);
  virtual nsresult WillUndoNotify(nsITransaction *aTransaction, PRBool *aInterrupt);
  virtual nsresult DidUndoNotify(nsITransaction *aTransaction, nsresult aUndoResult);
  virtual nsresult WillRedoNotify(nsITransaction *aTransaction, PRBool *aInterrupt);
  virtual nsresult DidRedoNotify(nsITransaction *aTransaction, nsresult aRedoResult);
  virtual nsresult WillBeginBatchNotify(PRBool *aInterrupt);
  virtual nsresult DidBeginBatchNotify(nsresult aResult);
  virtual nsresult WillEndBatchNotify(PRBool *aInterrupt);
  virtual nsresult DidEndBatchNotify(nsresult aResult);
  virtual nsresult WillMergeNotify(nsITransaction *aTop,
                                   nsITransaction *aTransaction,
                                   PRBool *aInterrupt);
  virtual nsresult DidMergeNotify(nsITransaction *aTop,
                                  nsITransaction *aTransaction,
                                  PRBool aDidMerge,
                                  nsresult aMergeResult);

private:

  
  virtual nsresult BeginTransaction(nsITransaction *aTransaction);
  virtual nsresult EndTransaction(void);
  virtual nsresult Lock(void);
  virtual nsresult Unlock(void);
};

#endif 
