




#ifndef nsTransactionManager_h__
#define nsTransactionManager_h__

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTransactionStack.h"
#include "nsISupportsImpl.h"
#include "nsITransactionManager.h"
#include "nsTransactionStack.h"
#include "nsWeakReference.h"
#include "nscore.h"

class nsITransaction;
class nsITransactionListener;




class nsTransactionManager final : public nsITransactionManager
                                 , public nsSupportsWeakReference
{
private:

  int32_t                mMaxTransactionCount;
  nsTransactionStack     mDoStack;
  nsTransactionStack     mUndoStack;
  nsTransactionStack     mRedoStack;
  nsCOMArray<nsITransactionListener> mListeners;

  

  virtual ~nsTransactionManager();

public:

  

  explicit nsTransactionManager(int32_t aMaxTransactionCount=-1);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsTransactionManager,
                                           nsITransactionManager)

  
  NS_DECL_NSITRANSACTIONMANAGER

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

  
  virtual nsresult BeginTransaction(nsITransaction *aTransaction,
                                    nsISupports *aData);
  virtual nsresult EndTransaction(bool aAllowEmpty);
};

#endif 
