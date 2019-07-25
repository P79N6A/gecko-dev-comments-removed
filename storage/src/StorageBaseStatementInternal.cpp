





#include "StorageBaseStatementInternal.h"

#include "nsProxyRelease.h"

#include "mozStorageBindingParamsArray.h"
#include "mozStorageStatementData.h"
#include "mozStorageAsyncStatementExecution.h"

namespace mozilla {
namespace storage {







class AsyncStatementFinalizer : public nsRunnable
{
public:
  










  AsyncStatementFinalizer(StorageBaseStatementInternal *aStatement,
                          Connection *aConnection)
  : mStatement(aStatement)
  , mConnection(aConnection)
  {
  }

  NS_IMETHOD Run()
  {
    if (mStatement->mAsyncStatement) {
      (void)::sqlite3_finalize(mStatement->mAsyncStatement);
      mStatement->mAsyncStatement = nullptr;
    }
    (void)::NS_ProxyRelease(mConnection->threadOpenedOn, mStatement);
    return NS_OK;
  }
private:
  nsRefPtr<StorageBaseStatementInternal> mStatement;
  nsRefPtr<Connection> mConnection;
};





class LastDitchSqliteStatementFinalizer : public nsRunnable
{
public:
  














  LastDitchSqliteStatementFinalizer(nsRefPtr<Connection> &aConnection,
                                    sqlite3_stmt *aStatement)
  : mConnection(aConnection)
  , mAsyncStatement(aStatement)
  {
    NS_PRECONDITION(aConnection, "You must provide a Connection");
  }

  NS_IMETHOD Run()
  {
    (void)::sqlite3_finalize(mAsyncStatement);
    mAsyncStatement = nullptr;

    
    
    Connection *rawConnection = nullptr;
    mConnection.swap(rawConnection);
    (void)::NS_ProxyRelease(
      rawConnection->threadOpenedOn,
      NS_ISUPPORTS_CAST(mozIStorageConnection *, rawConnection));
    return NS_OK;
  }
private:
  nsRefPtr<Connection> mConnection;
  sqlite3_stmt *mAsyncStatement;
};




StorageBaseStatementInternal::StorageBaseStatementInternal()
: mAsyncStatement(NULL)
{
}

void
StorageBaseStatementInternal::asyncFinalize()
{
  nsIEventTarget *target = mDBConnection->getAsyncExecutionTarget();
  if (!target) {
    
    
    
    destructorAsyncFinalize();
  }
  else {
    nsCOMPtr<nsIRunnable> event =
      new AsyncStatementFinalizer(this, mDBConnection);

    
    if (NS_FAILED(target->Dispatch(event, NS_DISPATCH_NORMAL))) {
      destructorAsyncFinalize();
    }
  }
}

void
StorageBaseStatementInternal::destructorAsyncFinalize()
{
  if (!mAsyncStatement)
    return;

  nsIEventTarget *target = mDBConnection->getAsyncExecutionTarget();
  if (target) {
    nsCOMPtr<nsIRunnable> event =
      new LastDitchSqliteStatementFinalizer(mDBConnection, mAsyncStatement);
    if (NS_SUCCEEDED(target->Dispatch(event, NS_DISPATCH_NORMAL))) {
      mAsyncStatement = nullptr;
      return;
    }
  }
  
  (void)::sqlite3_finalize(mAsyncStatement);
  mAsyncStatement = nullptr;
}

NS_IMETHODIMP
StorageBaseStatementInternal::NewBindingParamsArray(
  mozIStorageBindingParamsArray **_array
)
{
  nsCOMPtr<mozIStorageBindingParamsArray> array = new BindingParamsArray(this);
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  array.forget(_array);
  return NS_OK;
}

NS_IMETHODIMP
StorageBaseStatementInternal::ExecuteAsync(
  mozIStorageStatementCallback *aCallback,
  mozIStoragePendingStatement **_stmt
)
{
  
  
  
  
  
  nsTArray<StatementData> stmts(1);
  StatementData data;
  nsresult rv = getAsynchronousStatementData(data);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(stmts.AppendElement(data), NS_ERROR_OUT_OF_MEMORY);

  
  return AsyncExecuteStatements::execute(stmts, mDBConnection, aCallback,
                                         _stmt);
}

NS_IMETHODIMP
StorageBaseStatementInternal::EscapeStringForLIKE(
  const nsAString &aValue,
  const PRUnichar aEscapeChar,
  nsAString &_escapedString
)
{
  const PRUnichar MATCH_ALL('%');
  const PRUnichar MATCH_ONE('_');

  _escapedString.Truncate(0);

  for (uint32_t i = 0; i < aValue.Length(); i++) {
    if (aValue[i] == aEscapeChar || aValue[i] == MATCH_ALL ||
        aValue[i] == MATCH_ONE) {
      _escapedString += aEscapeChar;
    }
    _escapedString += aValue[i];
  }
  return NS_OK;
}

} 
} 
