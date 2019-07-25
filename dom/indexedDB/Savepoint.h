






































#ifndef mozilla_dom_indexeddb_savepoint_h__
#define mozilla_dom_indexeddb_savepoint_h__

#include "mozilla/dom/indexedDB/IDBTransactionRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

NS_STACK_CLASS
class Savepoint
{
public:
  Savepoint(IDBTransactionRequest* aTransaction)
  : mTransaction(aTransaction)
  , mHasSavepoint(false)
  {
    mName.AppendInt(mTransaction->GetUniqueNumberForName());
    hasSavepoint = mTransaction->StartSavepoint(mName);
  }

  void Rollback()
  {
    if (mHasSavepoint) {
      mTransaction->RevertToSavepoint(mName);
    }
  }
private:
  nsRefPtr<IDBTransactionRequest> mTransaction;
  nsCString mName;
  bool mHasSavepoint;
};

END_INDEXEDDB_NAMESPACE

#endif 
