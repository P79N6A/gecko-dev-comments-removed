





































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
    mStatement->internalAsyncFinalize();
    (void)::NS_ProxyRelease(mConnection->threadOpenedOn, mStatement);
    return NS_OK;
  }
private:
  
  nsCOMPtr<StorageBaseStatementInternal> mStatement;
  nsRefPtr<Connection> mConnection;
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
    
    
    
    internalAsyncFinalize();
  }
  else {
    nsCOMPtr<nsIRunnable> event =
      new AsyncStatementFinalizer(this, mDBConnection);

    
    if (!event ||
        NS_FAILED(target->Dispatch(event, NS_DISPATCH_NORMAL))) {
      internalAsyncFinalize();
    }
  }
}

void
StorageBaseStatementInternal::internalAsyncFinalize()
{
  if (mAsyncStatement) {
    (void)::sqlite3_finalize(mAsyncStatement);
    mAsyncStatement = nsnull;
  }
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

  for (PRUint32 i = 0; i < aValue.Length(); i++) {
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
