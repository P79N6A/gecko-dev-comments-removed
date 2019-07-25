






































#ifndef mozilla_dom_indexeddb_savepoint_h__
#define mozilla_dom_indexeddb_savepoint_h__


#include "IDBTransaction.h"

BEGIN_INDEXEDDB_NAMESPACE

NS_STACK_CLASS
class Savepoint
{
public:
  Savepoint(IDBTransaction* aTransaction)
  : mTransaction(aTransaction)
  , mHasSavepoint(false)
  {
    NS_ASSERTION(mTransaction, "Null pointer!");

    mHasSavepoint = mTransaction->StartSavepoint();
    NS_WARN_IF_FALSE(mHasSavepoint, "Failed to make savepoint!");
  }

  ~Savepoint()
  {
    if (mHasSavepoint) {
      mTransaction->RollbackSavepoint();
    }
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
  IDBTransaction* mTransaction;
  bool mHasSavepoint;
};

END_INDEXEDDB_NAMESPACE

#endif 
