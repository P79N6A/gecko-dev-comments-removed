




#ifndef nsTransactionList_h__
#define nsTransactionList_h__

#include "nsAutoPtr.h"
#include "nsISupportsImpl.h"
#include "nsITransactionList.h"
#include "nsIWeakReferenceUtils.h"

class nsITransaction;
class nsITransactionManager;
class nsTransactionItem;
class nsTransactionStack;




class nsTransactionList : public nsITransactionList
{
private:

  nsWeakPtr                   mTxnMgr;
  nsTransactionStack         *mTxnStack;
  nsRefPtr<nsTransactionItem> mTxnItem;

protected:
  virtual ~nsTransactionList();

public:

  nsTransactionList(nsITransactionManager *aTxnMgr, nsTransactionStack *aTxnStack);
  nsTransactionList(nsITransactionManager *aTxnMgr, nsTransactionItem *aTxnItem);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITRANSACTIONLIST

  
};

#endif
