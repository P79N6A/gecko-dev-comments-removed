




































#ifndef nsTransactionList_h__
#define nsTransactionList_h__

#include "nsWeakReference.h"
#include "nsITransactionList.h"

class nsITransaction;
class nsITransactionManager;
class nsTransactionItem;
class nsTransactionStack;
class nsTransactionRedoStack;




class nsTransactionList : public nsITransactionList
{
private:

  nsWeakPtr           mTxnMgr;
  nsTransactionStack *mTxnStack;
  nsTransactionItem  *mTxnItem;

public:

  nsTransactionList(nsITransactionManager *aTxnMgr, nsTransactionStack *aTxnStack);
  nsTransactionList(nsITransactionManager *aTxnMgr, nsTransactionItem *aTxnItem);

  virtual ~nsTransactionList();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITRANSACTIONLIST

  
};

#endif
