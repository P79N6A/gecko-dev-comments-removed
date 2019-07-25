






































#ifndef mozStorageStatementData_h
#define mozStorageStatementData_h

#include "sqlite3.h"

#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIEventTarget.h"
#include "nsProxyRelease.h"

#include "mozStorageBindingParamsArray.h"
#include "mozIStorageBaseStatement.h"
#include "mozStorageConnection.h"
#include "StorageBaseStatementInternal.h"

struct sqlite3_stmt;

namespace mozilla {
namespace storage {

class StatementData
{
public:
  StatementData(sqlite3_stmt *aStatement,
                already_AddRefed<BindingParamsArray> aParamsArray,
                StorageBaseStatementInternal *aStatementOwner)
  : mStatement(aStatement)
  , mParamsArray(aParamsArray)
  , mStatementOwner(aStatementOwner)
  {
    NS_PRECONDITION(mStatementOwner, "Must have a statement owner!");
  }
  StatementData(const StatementData &aSource)
  : mStatement(aSource.mStatement)
  , mParamsArray(aSource.mParamsArray)
  , mStatementOwner(aSource.mStatementOwner)
  {
    NS_PRECONDITION(mStatementOwner, "Must have a statement owner!");
  }
  StatementData()
  {
  }

  



  inline int getSqliteStatement(sqlite3_stmt **_stmt)
  {
    if (!mStatement) {
      int rc = mStatementOwner->getAsyncStatement(&mStatement);
      NS_ENSURE_TRUE(rc == SQLITE_OK, rc);
    }
    *_stmt = mStatement;
    return SQLITE_OK;
  }

  operator BindingParamsArray *() const { return mParamsArray; }

  



  operator sqlite3 *() const
  {
    return mStatementOwner->getOwner()->GetNativeConnection();
  }

  







  inline void finalize(nsIEventTarget *aReleaseThread)
  {
    NS_PRECONDITION(aReleaseThread, "Must have a non-NULL release thread!");
    NS_PRECONDITION(mStatementOwner, "Must have a statement owner!");
#ifdef DEBUG
    {
      nsCOMPtr<nsIEventTarget> asyncThread =
        mStatementOwner->getOwner()->getAsyncExecutionTarget();
      
      
      if (asyncThread) {
        PRBool onAsyncThread;
        NS_ASSERTION(NS_SUCCEEDED(asyncThread->IsOnCurrentThread(&onAsyncThread)) && onAsyncThread,
                     "This should only be running on the async thread!");
      }
    }
#endif
    
    
    
    if (mStatement) {
      (void)::sqlite3_reset(mStatement);
      (void)::sqlite3_clear_bindings(mStatement);
      mStatement = NULL;
    }
    (void)NS_ProxyRelease(aReleaseThread, mStatementOwner, PR_TRUE);
    if (mParamsArray) {
      (void)NS_ProxyRelease(aReleaseThread, mParamsArray, PR_TRUE);
    }
  }

  






  inline bool hasParametersToBeBound() const { return !!mParamsArray; }
  




  inline bool needsTransaction() const
  {
    return mParamsArray != nsnull && mParamsArray->length() > 1;
  }

private:
  sqlite3_stmt *mStatement;
  nsRefPtr<BindingParamsArray> mParamsArray;

  



  nsCOMPtr<StorageBaseStatementInternal> mStatementOwner;
};

} 
} 

#endif 
