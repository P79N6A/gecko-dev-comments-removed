






































#ifndef mozilla_dom_indexeddb_savepoint_h__
#define mozilla_dom_indexeddb_savepoint_h__


#include "IDBTransactionRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

NS_STACK_CLASS
class Savepoint
{
public:
  Savepoint(IDBTransactionRequest* aTransaction)
  : mTransaction(aTransaction)
  , mHasSavepoint(false)
  {
    mHasSavepoint = mTransaction->StartSavepoint();
    NS_WARN_IF_FALSE(mHasSavepoint, "Failed to make savepoint!");
  }

  ~Savepoint()
  {
    Release();
  }

  nsresult Release()
  {
    nsresult rv = NS_OK;
    if (mHasSavepoint) {
      rv = mTransaction->ReleaseSavepoint();
      mHasSavepoint = false;
    }
    return rv;
  }

private:
  IDBTransactionRequest* mTransaction;
  bool mHasSavepoint;
};

END_INDEXEDDB_NAMESPACE

#endif 
