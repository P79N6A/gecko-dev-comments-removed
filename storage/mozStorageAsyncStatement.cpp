





#include <limits.h>
#include <stdio.h>

#include "nsError.h"
#include "nsMemory.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsIClassInfoImpl.h"
#include "Variant.h"

#include "mozIStorageError.h"

#include "mozStorageBindingParams.h"
#include "mozStorageConnection.h"
#include "mozStorageAsyncStatementJSHelper.h"
#include "mozStorageAsyncStatementParams.h"
#include "mozStoragePrivateHelpers.h"
#include "mozStorageStatementRow.h"
#include "mozStorageStatement.h"
#include "nsDOMClassInfo.h"

#include "mozilla/Logging.h"

extern PRLogModuleInfo *gStorageLog;

namespace mozilla {
namespace storage {




NS_IMPL_CI_INTERFACE_GETTER(AsyncStatement,
                            mozIStorageAsyncStatement,
                            mozIStorageBaseStatement,
                            mozIStorageBindingParams,
                            mozilla::storage::StorageBaseStatementInternal)

class AsyncStatementClassInfo : public nsIClassInfo
{
public:
  MOZ_CONSTEXPR AsyncStatementClassInfo() {}

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHODIMP
  GetInterfaces(uint32_t *_count, nsIID ***_array) override
  {
    return NS_CI_INTERFACE_GETTER_NAME(AsyncStatement)(_count, _array);
  }

  NS_IMETHODIMP
  GetScriptableHelper(nsIXPCScriptable **_helper) override
  {
    static AsyncStatementJSHelper sJSHelper;
    *_helper = &sJSHelper;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetContractID(char **_contractID) override
  {
    *_contractID = nullptr;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetClassDescription(char **_desc) override
  {
    *_desc = nullptr;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetClassID(nsCID **_id) override
  {
    *_id = nullptr;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetFlags(uint32_t *_flags) override
  {
    *_flags = 0;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetClassIDNoAlloc(nsCID *_cid) override
  {
    return NS_ERROR_NOT_AVAILABLE;
  }
};

NS_IMETHODIMP_(MozExternalRefCountType) AsyncStatementClassInfo::AddRef() { return 2; }
NS_IMETHODIMP_(MozExternalRefCountType) AsyncStatementClassInfo::Release() { return 1; }
NS_IMPL_QUERY_INTERFACE(AsyncStatementClassInfo, nsIClassInfo)

static AsyncStatementClassInfo sAsyncStatementClassInfo;




AsyncStatement::AsyncStatement()
: StorageBaseStatementInternal()
, mFinalized(false)
{
}

nsresult
AsyncStatement::initialize(Connection *aDBConnection,
                           sqlite3 *aNativeConnection,
                           const nsACString &aSQLStatement)
{
  MOZ_ASSERT(aDBConnection, "No database connection given!");
  MOZ_ASSERT(!aDBConnection->isClosed(), "Database connection should be valid");
  MOZ_ASSERT(aNativeConnection, "No native connection given!");

  mDBConnection = aDBConnection;
  mNativeConnection = aNativeConnection;
  mSQLString = aSQLStatement;

  MOZ_LOG(gStorageLog, LogLevel::Debug, ("Inited async statement '%s' (0x%p)",
                                      mSQLString.get()));

#ifdef DEBUG
  
  
  
  
  const nsCaseInsensitiveCStringComparator c;
  nsACString::const_iterator start, end, e;
  aSQLStatement.BeginReading(start);
  aSQLStatement.EndReading(end);
  e = end;
  while (::FindInReadable(NS_LITERAL_CSTRING(" LIKE"), start, e, c)) {
    
    
    
    nsACString::const_iterator s1, s2, s3;
    s1 = s2 = s3 = start;

    if (!(::FindInReadable(NS_LITERAL_CSTRING(" LIKE ?"), s1, end, c) ||
          ::FindInReadable(NS_LITERAL_CSTRING(" LIKE :"), s2, end, c) ||
          ::FindInReadable(NS_LITERAL_CSTRING(" LIKE @"), s3, end, c))) {
      
      
      
      NS_WARNING("Unsafe use of LIKE detected!  Please ensure that you "
                 "are using mozIStorageAsyncStatement::escapeStringForLIKE "
                 "and that you are binding that result to the statement "
                 "to prevent SQL injection attacks.");
    }

    
    start = e;
    e = end;
  }
#endif

  return NS_OK;
}

mozIStorageBindingParams *
AsyncStatement::getParams()
{
  nsresult rv;

  
  if (!mParamsArray) {
    nsCOMPtr<mozIStorageBindingParamsArray> array;
    rv = NewBindingParamsArray(getter_AddRefs(array));
    NS_ENSURE_SUCCESS(rv, nullptr);

    mParamsArray = static_cast<BindingParamsArray *>(array.get());
  }

  
  if (mParamsArray->length() == 0) {
    nsRefPtr<AsyncBindingParams> params(new AsyncBindingParams(mParamsArray));
    NS_ENSURE_TRUE(params, nullptr);

    rv = mParamsArray->AddParams(params);
    NS_ENSURE_SUCCESS(rv, nullptr);

    
    
    params->unlock(nullptr);

    
    
    mParamsArray->lock();
  }

  return *mParamsArray->begin();
}










AsyncStatement::~AsyncStatement()
{
  destructorAsyncFinalize();

  
  
  bool onCallingThread = false;
  (void)mDBConnection->threadOpenedOn->IsOnCurrentThread(&onCallingThread);
  if (!onCallingThread) {
    
    
    Connection *forgottenConn = nullptr;
    mDBConnection.swap(forgottenConn);
    (void)::NS_ProxyRelease(forgottenConn->threadOpenedOn,
                            static_cast<mozIStorageConnection *>(forgottenConn));
  }
}




NS_IMPL_ADDREF(AsyncStatement)
NS_IMPL_RELEASE(AsyncStatement)

NS_INTERFACE_MAP_BEGIN(AsyncStatement)
  NS_INTERFACE_MAP_ENTRY(mozIStorageAsyncStatement)
  NS_INTERFACE_MAP_ENTRY(mozIStorageBaseStatement)
  NS_INTERFACE_MAP_ENTRY(mozIStorageBindingParams)
  NS_INTERFACE_MAP_ENTRY(mozilla::storage::StorageBaseStatementInternal)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    foundInterface = static_cast<nsIClassInfo *>(&sAsyncStatementClassInfo);
  }
  else
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozIStorageAsyncStatement)
NS_INTERFACE_MAP_END





Connection *
AsyncStatement::getOwner()
{
  return mDBConnection;
}

int
AsyncStatement::getAsyncStatement(sqlite3_stmt **_stmt)
{
#ifdef DEBUG
  
  bool onOpenedThread = false;
  (void)mDBConnection->threadOpenedOn->IsOnCurrentThread(&onOpenedThread);
  NS_ASSERTION(!onOpenedThread,
               "We should only be called on the async thread!");
#endif

  if (!mAsyncStatement) {
    int rc = mDBConnection->prepareStatement(mNativeConnection, mSQLString,
                                             &mAsyncStatement);
    if (rc != SQLITE_OK) {
      MOZ_LOG(gStorageLog, LogLevel::Error,
             ("Sqlite statement prepare error: %d '%s'", rc,
              ::sqlite3_errmsg(mNativeConnection)));
      MOZ_LOG(gStorageLog, LogLevel::Error,
             ("Statement was: '%s'", mSQLString.get()));
      *_stmt = nullptr;
      return rc;
    }
    MOZ_LOG(gStorageLog, LogLevel::Debug, ("Initialized statement '%s' (0x%p)",
                                        mSQLString.get(),
                                        mAsyncStatement));
  }

  *_stmt = mAsyncStatement;
  return SQLITE_OK;
}

nsresult
AsyncStatement::getAsynchronousStatementData(StatementData &_data)
{
  if (mFinalized)
    return NS_ERROR_UNEXPECTED;

  
  
  _data = StatementData(nullptr, bindingParamsArray(), this);

  return NS_OK;
}

already_AddRefed<mozIStorageBindingParams>
AsyncStatement::newBindingParams(mozIStorageBindingParamsArray *aOwner)
{
  if (mFinalized)
    return nullptr;

  nsCOMPtr<mozIStorageBindingParams> params(new AsyncBindingParams(aOwner));
  return params.forget();
}











MIXIN_IMPL_STORAGEBASESTATEMENTINTERNAL(
  AsyncStatement,
  if (mFinalized) return NS_ERROR_UNEXPECTED;)

NS_IMETHODIMP
AsyncStatement::Finalize()
{
  if (mFinalized)
    return NS_OK;

  mFinalized = true;

  MOZ_LOG(gStorageLog, LogLevel::Debug, ("Finalizing statement '%s'",
                                      mSQLString.get()));

  asyncFinalize();

  
  mStatementParamsHolder = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
AsyncStatement::BindParameters(mozIStorageBindingParamsArray *aParameters)
{
  if (mFinalized)
    return NS_ERROR_UNEXPECTED;

  BindingParamsArray *array = static_cast<BindingParamsArray *>(aParameters);
  if (array->getOwner() != this)
    return NS_ERROR_UNEXPECTED;

  if (array->length() == 0)
    return NS_ERROR_UNEXPECTED;

  mParamsArray = array;
  mParamsArray->lock();

  return NS_OK;
}

NS_IMETHODIMP
AsyncStatement::GetState(int32_t *_state)
{
  if (mFinalized)
    *_state = MOZ_STORAGE_STATEMENT_INVALID;
  else
    *_state = MOZ_STORAGE_STATEMENT_READY;

  return NS_OK;
}




BOILERPLATE_BIND_PROXIES(
  AsyncStatement, 
  if (mFinalized) return NS_ERROR_UNEXPECTED;
)

} 
} 
