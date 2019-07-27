





#include "mozilla/dom/cache/Connection.h"

namespace mozilla {
namespace dom {
namespace cache {

NS_IMPL_ISUPPORTS(cache::Connection, mozIStorageAsyncConnection,
                                     mozIStorageConnection);

Connection::Connection(mozIStorageConnection* aBase)
  : mBase(aBase)
{
  MOZ_ASSERT(mBase);
}

Connection::~Connection()
{
}








NS_IMETHODIMP
Connection::AsyncClose(mozIStorageCompletionCallback*)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Connection::AsyncClone(bool, mozIStorageCompletionCallback*)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Connection::GetDatabaseFile(nsIFile** aFileOut)
{
  return mBase->GetDatabaseFile(aFileOut);
}

NS_IMETHODIMP
Connection::CreateAsyncStatement(const nsACString&, mozIStorageAsyncStatement**)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Connection::ExecuteAsync(mozIStorageBaseStatement**, uint32_t,
                         mozIStorageStatementCallback*,
                         mozIStoragePendingStatement**)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Connection::ExecuteSimpleSQLAsync(const nsACString&,
                                  mozIStorageStatementCallback*,
                                  mozIStoragePendingStatement**)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Connection::CreateFunction(const nsACString& aFunctionName,
                           int32_t aNumArguments,
                           mozIStorageFunction* aFunction)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
Connection::CreateAggregateFunction(const nsACString& aFunctionName,
                                    int32_t aNumArguments,
                                    mozIStorageAggregateFunction* aFunction)
{
  return mBase->CreateAggregateFunction(aFunctionName, aNumArguments,
                                        aFunction);
}

NS_IMETHODIMP
Connection::RemoveFunction(const nsACString& aFunctionName)
{
  return mBase->RemoveFunction(aFunctionName);
}

NS_IMETHODIMP
Connection::SetProgressHandler(int32_t aGranularity,
                               mozIStorageProgressHandler* aHandler,
                               mozIStorageProgressHandler** aHandlerOut)
{
  return mBase->SetProgressHandler(aGranularity, aHandler, aHandlerOut);
}

NS_IMETHODIMP
Connection::RemoveProgressHandler(mozIStorageProgressHandler** aHandlerOut)
{
  return mBase->RemoveProgressHandler(aHandlerOut);
}



NS_IMETHODIMP
Connection::Close()
{
  return mBase->Close();
}

NS_IMETHODIMP
Connection::Clone(bool aReadOnly, mozIStorageConnection** aConnectionOut)
{
  nsCOMPtr<mozIStorageConnection> conn;
  nsresult rv = mBase->Clone(aReadOnly, getter_AddRefs(conn));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsCOMPtr<mozIStorageConnection> wrapped = new Connection(conn);
  wrapped.forget(aConnectionOut);

  return rv;
}

NS_IMETHODIMP
Connection::GetDefaultPageSize(int32_t* aSizeOut)
{
  return mBase->GetDefaultPageSize(aSizeOut);
}

NS_IMETHODIMP
Connection::GetConnectionReady(bool* aReadyOut)
{
  return mBase->GetConnectionReady(aReadyOut);
}

NS_IMETHODIMP
Connection::GetLastInsertRowID(int64_t* aRowIdOut)
{
  return mBase->GetLastInsertRowID(aRowIdOut);
}

NS_IMETHODIMP
Connection::GetAffectedRows(int32_t* aCountOut)
{
  return mBase->GetAffectedRows(aCountOut);
}

NS_IMETHODIMP
Connection::GetLastError(int32_t* aErrorOut)
{
  return mBase->GetLastError(aErrorOut);
}

NS_IMETHODIMP
Connection::GetLastErrorString(nsACString& aErrorOut)
{
  return mBase->GetLastErrorString(aErrorOut);
}

NS_IMETHODIMP
Connection::GetSchemaVersion(int32_t* aVersionOut)
{
  return mBase->GetSchemaVersion(aVersionOut);
}

NS_IMETHODIMP
Connection::SetSchemaVersion(int32_t aVersion)
{
  return mBase->SetSchemaVersion(aVersion);
}

NS_IMETHODIMP
Connection::CreateStatement(const nsACString& aQuery,
                            mozIStorageStatement** aStatementOut)
{
  return mBase->CreateStatement(aQuery, aStatementOut);
}

NS_IMETHODIMP
Connection::ExecuteSimpleSQL(const nsACString& aQuery)
{
  return mBase->ExecuteSimpleSQL(aQuery);
}

NS_IMETHODIMP
Connection::TableExists(const nsACString& aTableName, bool* aExistsOut)
{
  return mBase->TableExists(aTableName, aExistsOut);
}

NS_IMETHODIMP
Connection::IndexExists(const nsACString& aIndexName, bool* aExistsOut)
{
  return mBase->IndexExists(aIndexName, aExistsOut);
}

NS_IMETHODIMP
Connection::GetTransactionInProgress(bool* aResultOut)
{
  return mBase->GetTransactionInProgress(aResultOut);
}

NS_IMETHODIMP
Connection::BeginTransaction()
{
  return mBase->BeginTransaction();
}

NS_IMETHODIMP
Connection::BeginTransactionAs(int32_t aType)
{
  return mBase->BeginTransactionAs(aType);
}

NS_IMETHODIMP
Connection::CommitTransaction()
{
  return mBase->CommitTransaction();
}

NS_IMETHODIMP
Connection::RollbackTransaction()
{
  return mBase->RollbackTransaction();
}

NS_IMETHODIMP
Connection::CreateTable(const char* aTable, const char* aSchema)
{
  return mBase->CreateTable(aTable, aSchema);
}

NS_IMETHODIMP
Connection::SetGrowthIncrement(int32_t aIncrement, const nsACString& aDatabase)
{
  return mBase->SetGrowthIncrement(aIncrement, aDatabase);
}

NS_IMETHODIMP
Connection::EnableModule(const nsACString& aModule)
{
  return mBase->EnableModule(aModule);
}

} 
} 
} 
