






































#ifndef mozStorageStatementData_h
#define mozStorageStatementData_h

#include "sqlite3.h"

#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIEventTarget.h"
#include "mozilla/Util.h"
#include "nsThreadUtils.h"

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

  



  inline void finalize()
  {
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
  }

  






  inline bool hasParametersToBeBound() const { return !!mParamsArray; }
  









  inline PRUint32 needsTransaction()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    
    
    
    sqlite3_stmt *stmt;
    int rc = getSqliteStatement(&stmt);
    if (SQLITE_OK != rc || ::sqlite3_stmt_readonly(stmt)) {
      return 0;
    }
    return mParamsArray ? mParamsArray->length() : 1;
  }

private:
  sqlite3_stmt *mStatement;
  nsRefPtr<BindingParamsArray> mParamsArray;

  



  nsCOMPtr<StorageBaseStatementInternal> mStatementOwner;
};

} 
} 

#endif 
