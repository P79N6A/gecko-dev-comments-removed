




































#ifndef nsTransactionList_h__
#define nsTransactionList_h__

#include "nsWeakReference.h"
#include "nsITransactionList.h"
#include "nsTransactionItem.h"
#include "nsAutoPtr.h"

class nsITransaction;
class nsITransactionManager;
class nsTransactionStack;
class nsTransactionRedoStack;




class nsTransactionList : public nsITransactionList
{
private:

  nsWeakPtr                   mTxnMgr;
  nsTransactionStack         *mTxnStack;
  nsRefPtr<nsTransactionItem> mTxnItem;

public:

  nsTransactionList(nsITransactionManager *aTxnMgr, nsTransactionStack *aTxnStack);
  nsTransactionList(nsITransactionManager *aTxnMgr, nsTransactionItem *aTxnItem);

  virtual ~nsTransactionList();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITRANSACTIONLIST

  
};

#endif
