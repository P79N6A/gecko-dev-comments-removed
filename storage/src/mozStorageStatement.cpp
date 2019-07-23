








































#include <stdio.h>

#include "nsError.h"
#include "nsMemory.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"

#include "mozIStorageError.h"

#include "mozStorageBindingParams.h"
#include "mozStorageConnection.h"
#include "mozStorageStatementJSHelper.h"
#include "mozStoragePrivateHelpers.h"
#include "mozStorageStatementParams.h"
#include "mozStorageStatementRow.h"
#include "mozStorageStatement.h"

#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gStorageLog;
#endif

namespace mozilla {
namespace storage {




namespace {




class AsyncStatementFinalizer : public nsRunnable
{
public:
  








  AsyncStatementFinalizer(sqlite3_stmt *aStatement,
                          nsISupports *aParent)
  : mStatement(aStatement)
  , mParent(aParent)
  , mCallingThread(do_GetCurrentThread())
  {
  }

  NS_IMETHOD Run()
  {
    (void)::sqlite3_finalize(mStatement);
    (void)::NS_ProxyRelease(mCallingThread, mParent);
    return NS_OK;
  }
private:
  sqlite3_stmt *mStatement;
  nsCOMPtr<nsISupports> mParent;
  nsCOMPtr<nsIThread> mCallingThread;
};

} 




NS_IMPL_CI_INTERFACE_GETTER2(
  Statement,
  mozIStorageStatement,
  mozIStorageValueArray
)

class StatementClassInfo : public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHODIMP
  GetInterfaces(PRUint32 *_count, nsIID ***_array)
  {
    return NS_CI_INTERFACE_GETTER_NAME(Statement)(_count, _array);
  }

  NS_IMETHODIMP
  GetHelperForLanguage(PRUint32 aLanguage, nsISupports **_helper)
  {
    if (aLanguage == nsIProgrammingLanguage::JAVASCRIPT) {
      static StatementJSHelper sJSHelper;
      *_helper = &sJSHelper;
      return NS_OK;
    }

    *_helper = nsnull;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetContractID(char **_contractID)
  {
    *_contractID = nsnull;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetClassDescription(char **_desc)
  {
    *_desc = nsnull;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetClassID(nsCID **_id)
  {
    *_id = nsnull;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetImplementationLanguage(PRUint32 *_language)
  {
    *_language = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetFlags(PRUint32 *_flags)
  {
    *_flags = nsnull;
    return NS_OK;
  }

  NS_IMETHODIMP
  GetClassIDNoAlloc(nsCID *_cid)
  {
    return NS_ERROR_NOT_AVAILABLE;
  }
};

NS_IMETHODIMP_(nsrefcnt) StatementClassInfo::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) StatementClassInfo::Release() { return 1; }
NS_IMPL_QUERY_INTERFACE1(StatementClassInfo, nsIClassInfo)

static StatementClassInfo sStatementClassInfo;




Statement::Statement()
: mDBConnection(nsnull)
, mDBStatement(NULL)
, mColumnNames()
, mExecuting(false)
, mCachedAsyncStatement(NULL)
{
}

nsresult
Statement::initialize(Connection *aDBConnection,
                      const nsACString &aSQLStatement)
{
  NS_ASSERTION(aDBConnection, "No database connection given!");
  NS_ASSERTION(!mDBStatement, "Statement already initialized!");

  sqlite3 *db = aDBConnection->GetNativeConnection();
  NS_ASSERTION(db, "We should never be called with a null sqlite3 database!");

  int srv = ::sqlite3_prepare_v2(db, PromiseFlatCString(aSQLStatement).get(),
                                 -1, &mDBStatement, NULL);
  if (srv != SQLITE_OK) {
#ifdef PR_LOGGING
      PR_LOG(gStorageLog, PR_LOG_ERROR,
             ("Sqlite statement prepare error: %d '%s'", srv,
              ::sqlite3_errmsg(db)));
      PR_LOG(gStorageLog, PR_LOG_ERROR,
             ("Statement was: '%s'", PromiseFlatCString(aSQLStatement).get()));
#endif
      return NS_ERROR_FAILURE;
    }

#ifdef PR_LOGGING
  PR_LOG(gStorageLog, PR_LOG_NOTICE, ("Initialized statement '%s' (0x%p)",
                                      PromiseFlatCString(aSQLStatement).get(),
                                      mDBStatement));
#endif

  mDBConnection = aDBConnection;
  mParamCount = ::sqlite3_bind_parameter_count(mDBStatement);
  mResultColumnCount = ::sqlite3_column_count(mDBStatement);
  mColumnNames.Clear();

  for (PRUint32 i = 0; i < mResultColumnCount; i++) {
      const char *name = ::sqlite3_column_name(mDBStatement, i);
      (void)mColumnNames.AppendElement(nsDependentCString(name));
  }

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
                 "are using mozIStorageStatement::escapeStringForLIKE "
                 "and that you are binding that result to the statement "
                 "to prevent SQL injection attacks.");
    }

    
    start = e;
    e = end;
  }
#endif

  return NS_OK;
}

nsresult
Statement::getAsynchronousStatementData(StatementData &_data)
{
  if (!mDBStatement)
    return NS_ERROR_UNEXPECTED;

  sqlite3_stmt *stmt;
  int rc = getAsyncStatement(&stmt);
  if (rc != SQLITE_OK)
    return convertResultCode(rc);

  _data = StatementData(stmt, bindingParamsArray(), this);

  return NS_OK;
}

int
Statement::getAsyncStatement(sqlite3_stmt **_stmt)
{
  
  NS_ASSERTION(mDBStatement != NULL, "We have no statement to clone!");

  
  if (!mCachedAsyncStatement) {
    int rc = ::sqlite3_prepare_v2(mDBConnection->GetNativeConnection(),
                                  ::sqlite3_sql(mDBStatement), -1,
                                  &mCachedAsyncStatement, NULL);
    if (rc != SQLITE_OK)
      return rc;

#ifdef PR_LOGGING
    PR_LOG(gStorageLog, PR_LOG_NOTICE,
           ("Cloned statement 0x%p to 0x%p", mDBStatement,
            mCachedAsyncStatement));
#endif
  }

  *_stmt = mCachedAsyncStatement;
  return SQLITE_OK;
}

BindingParams *
Statement::getParams()
{
  nsresult rv;

  
  if (!mParamsArray) {
    nsCOMPtr<mozIStorageBindingParamsArray> array;
    rv = NewBindingParamsArray(getter_AddRefs(array));
    NS_ENSURE_SUCCESS(rv, nsnull);

    mParamsArray = static_cast<BindingParamsArray *>(array.get());
  }

  
  if (mParamsArray->length() == 0) {
    nsRefPtr<BindingParams> params(new BindingParams(mParamsArray, this));
    NS_ENSURE_TRUE(params, nsnull);

    rv = mParamsArray->AddParams(params);
    NS_ENSURE_SUCCESS(rv, nsnull);

    
    
    params->unlock();

    
    
    
    mParamsArray->lock();
  }

  return *mParamsArray->begin();
}

Statement::~Statement()
{
  (void)Finalize();
}

NS_IMPL_THREADSAFE_ADDREF(Statement)
NS_IMPL_THREADSAFE_RELEASE(Statement)

NS_INTERFACE_MAP_BEGIN(Statement)
  NS_INTERFACE_MAP_ENTRY(mozIStorageStatement)
  NS_INTERFACE_MAP_ENTRY(mozIStorageValueArray)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    foundInterface = static_cast<nsIClassInfo *>(&sStatementClassInfo);
  }
  else
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
Statement::Clone(mozIStorageStatement **_statement)
{
  nsRefPtr<Statement> statement(new Statement());
  NS_ENSURE_TRUE(statement, NS_ERROR_OUT_OF_MEMORY);

  nsCAutoString sql(::sqlite3_sql(mDBStatement));
  nsresult rv = statement->initialize(mDBConnection, sql);
  NS_ENSURE_SUCCESS(rv, rv);

  statement.forget(_statement);
  return NS_OK;
}

NS_IMETHODIMP
Statement::Finalize()
{
  if (!mDBStatement)
    return NS_OK;

#ifdef PR_LOGGING
  PR_LOG(gStorageLog, PR_LOG_NOTICE, ("Finalizing statement '%s'",
                                      ::sqlite3_sql(mDBStatement)));
#endif

  int srv = ::sqlite3_finalize(mDBStatement);
  mDBStatement = NULL;

  
  
  
  if (mCachedAsyncStatement) {
    nsCOMPtr<nsIEventTarget> target = mDBConnection->getAsyncExecutionTarget();
    if (!target) {
      
      
      
      (void)::sqlite3_finalize(mCachedAsyncStatement);
    }
    else {
      nsCOMPtr<nsIRunnable> event =
        new AsyncStatementFinalizer(mCachedAsyncStatement, this);
      NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

      nsresult rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, rv);
      mCachedAsyncStatement = NULL;
    }
  }

  
  
  if (mStatementParamsHolder) {
    nsCOMPtr<nsIXPConnectWrappedNative> wrapper =
        do_QueryInterface(mStatementParamsHolder);
    nsCOMPtr<mozIStorageStatementParams> iParams =
        do_QueryWrappedNative(wrapper);
    StatementParams *params = static_cast<StatementParams *>(iParams.get());
    params->mStatement = nsnull;
    mStatementParamsHolder = nsnull;
  }

  if (mStatementRowHolder) {
    nsCOMPtr<nsIXPConnectWrappedNative> wrapper =
        do_QueryInterface(mStatementRowHolder);
    nsCOMPtr<mozIStorageStatementRow> iRow =
        do_QueryWrappedNative(wrapper);
    StatementRow *row = static_cast<StatementRow *>(iRow.get());
    row->mStatement = nsnull;
    mStatementRowHolder = nsnull;
  }

  return convertResultCode(srv);
}

NS_IMETHODIMP
Statement::GetParameterCount(PRUint32 *_parameterCount)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  *_parameterCount = mParamCount;
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetParameterName(PRUint32 aParamIndex,
                            nsACString &_name)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;
  ENSURE_INDEX_VALUE(aParamIndex, mParamCount);

  const char *name = ::sqlite3_bind_parameter_name(mDBStatement,
                                                   aParamIndex + 1);
  if (name == NULL) {
    
    nsCAutoString name(":");
    name.AppendInt(aParamIndex);
    _name.Assign(name);
  }
  else {
    _name.Assign(nsDependentCString(name));
  }

  return NS_OK;
}

NS_IMETHODIMP
Statement::GetParameterIndex(const nsACString &aName,
                             PRUint32 *_index)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  
  
  nsCAutoString name(":");
  name.Append(aName);
  int ind = ::sqlite3_bind_parameter_index(mDBStatement,
                                           PromiseFlatCString(name).get());
  if (ind  == 0) 
    return NS_ERROR_INVALID_ARG;

  *_index = ind - 1; 

  return NS_OK;
}

NS_IMETHODIMP
Statement::GetColumnCount(PRUint32 *_columnCount)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  *_columnCount = mResultColumnCount;
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetColumnName(PRUint32 aColumnIndex,
                         nsACString &_name)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;
  ENSURE_INDEX_VALUE(aColumnIndex, mResultColumnCount);

  const char *cname = ::sqlite3_column_name(mDBStatement, aColumnIndex);
  _name.Assign(nsDependentCString(cname));

  return NS_OK;
}

NS_IMETHODIMP
Statement::GetColumnIndex(const nsACString &aName,
                          PRUint32 *_index)
{
  if (!mDBStatement)
      return NS_ERROR_NOT_INITIALIZED;

  
  
  for (PRUint32 i = 0; i < mResultColumnCount; i++) {
    if (mColumnNames[i].Equals(aName)) {
      *_index = i;
      return NS_OK;
    }
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
Statement::Reset()
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

#ifdef DEBUG
  PR_LOG(gStorageLog, PR_LOG_DEBUG, ("Resetting statement: '%s'",
                                     ::sqlite3_sql(mDBStatement)));

  checkAndLogStatementPerformance(mDBStatement);
#endif

  mParamsArray = nsnull;
  (void)sqlite3_reset(mDBStatement);
  (void)sqlite3_clear_bindings(mDBStatement);

  mExecuting = false;

  return NS_OK;
}

NS_IMETHODIMP
Statement::BindUTF8StringParameter(PRUint32 aParamIndex,
                                   const nsACString &aValue)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  BindingParams *params = getParams();
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  return params->BindUTF8StringByIndex(aParamIndex, aValue);
}

NS_IMETHODIMP
Statement::BindStringParameter(PRUint32 aParamIndex,
                               const nsAString &aValue)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  BindingParams *params = getParams();
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  return params->BindStringByIndex(aParamIndex, aValue);
}

NS_IMETHODIMP
Statement::BindDoubleParameter(PRUint32 aParamIndex,
                               double aValue)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  BindingParams *params = getParams();
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  return params->BindDoubleByIndex(aParamIndex, aValue);
}

NS_IMETHODIMP
Statement::BindInt32Parameter(PRUint32 aParamIndex,
                              PRInt32 aValue)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  BindingParams *params = getParams();
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  return params->BindInt32ByIndex(aParamIndex, aValue);
}

NS_IMETHODIMP
Statement::BindInt64Parameter(PRUint32 aParamIndex,
                              PRInt64 aValue)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  BindingParams *params = getParams();
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  return params->BindInt64ByIndex(aParamIndex, aValue);
}

NS_IMETHODIMP
Statement::BindNullParameter(PRUint32 aParamIndex)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  BindingParams *params = getParams();
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  return params->BindNullByIndex(aParamIndex);
}

NS_IMETHODIMP
Statement::BindBlobParameter(PRUint32 aParamIndex,
                             const PRUint8 *aValue,
                             PRUint32 aValueSize)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  BindingParams *params = getParams();
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  return params->BindBlobByIndex(aParamIndex, aValue, aValueSize);
}

NS_IMETHODIMP
Statement::BindParameters(mozIStorageBindingParamsArray *aParameters)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  BindingParamsArray *array = static_cast<BindingParamsArray *>(aParameters);
  if (array->getOwner() != this)
    return NS_ERROR_UNEXPECTED;

  mParamsArray = array;
  mParamsArray->lock();

  return NS_OK;
}

NS_IMETHODIMP
Statement::NewBindingParamsArray(mozIStorageBindingParamsArray **_array)
{
  nsCOMPtr<mozIStorageBindingParamsArray> array =
    new BindingParamsArray(this);
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  array.forget(_array);
  return NS_OK;
}

NS_IMETHODIMP
Statement::Execute()
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  PRBool ret;
  nsresult rv = ExecuteStep(&ret);
  NS_ENSURE_SUCCESS(rv, rv);

  return Reset();
}

NS_IMETHODIMP
Statement::ExecuteStep(PRBool *_moreResults)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  
  if (mParamsArray) {
    
    
    if (mParamsArray->length() != 1)
      return NS_ERROR_UNEXPECTED;

    BindingParamsArray::iterator row = mParamsArray->begin();
    nsCOMPtr<mozIStorageError> error;
    error = (*row)->bind(mDBStatement);
    if (error) {
      PRInt32 srv;
      (void)error->GetResult(&srv);
      return convertResultCode(srv);
    }

    
    mParamsArray = nsnull;
  }
  int srv = ::sqlite3_step(mDBStatement);

#ifdef PR_LOGGING
  if (srv != SQLITE_ROW && srv != SQLITE_DONE) {
      nsCAutoString errStr;
      (void)mDBConnection->GetLastErrorString(errStr);
      PR_LOG(gStorageLog, PR_LOG_DEBUG,
             ("Statement::ExecuteStep error: %s", errStr.get()));
  }
#endif

  
  if (srv == SQLITE_ROW) {
    
    mExecuting = true;
    *_moreResults = PR_TRUE;
    return NS_OK;
  }
  else if (srv == SQLITE_DONE) {
    
    mExecuting = false;
    *_moreResults = PR_FALSE;
    return NS_OK;
  }
  else if (srv == SQLITE_BUSY || srv == SQLITE_MISUSE) {
    mExecuting = PR_FALSE;
  }
  else if (mExecuting) {
#ifdef PR_LOGGING
    PR_LOG(gStorageLog, PR_LOG_ERROR,
           ("SQLite error after mExecuting was true!"));
#endif
    mExecuting = PR_FALSE;
  }

  return convertResultCode(srv);
}

nsresult
Statement::ExecuteAsync(mozIStorageStatementCallback *aCallback,
                        mozIStoragePendingStatement **_stmt)
{
  mozIStorageStatement *stmts[1] = {this};
  return mDBConnection->ExecuteAsync(stmts, 1, aCallback, _stmt);
}

NS_IMETHODIMP
Statement::GetState(PRInt32 *_state)
{
  if (!mDBStatement)
    *_state = MOZ_STORAGE_STATEMENT_INVALID;
  else if (mExecuting)
    *_state = MOZ_STORAGE_STATEMENT_EXECUTING;
  else
    *_state = MOZ_STORAGE_STATEMENT_READY;

  return NS_OK;
}

NS_IMETHODIMP
Statement::EscapeStringForLIKE(const nsAString &aValue,
                               const PRUnichar aEscapeChar,
                               nsAString &_escapedString)
{
  const PRUnichar MATCH_ALL('%');
  const PRUnichar MATCH_ONE('_');

  _escapedString.Truncate(0);

  for (PRUint32 i = 0; i < aValue.Length(); i++) {
    if (aValue[i] == aEscapeChar || aValue[i] == MATCH_ALL ||
        aValue[i] == MATCH_ONE)
      _escapedString += aEscapeChar;
    _escapedString += aValue[i];
  }
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetColumnDecltype(PRUint32 aParamIndex,
                             nsACString &_declType)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  ENSURE_INDEX_VALUE(aParamIndex, mResultColumnCount);

  _declType.Assign(::sqlite3_column_decltype(mDBStatement, aParamIndex));
  return NS_OK;
}




NS_IMETHODIMP
Statement::GetNumEntries(PRUint32 *_length)
{
  *_length = mResultColumnCount;
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetTypeOfIndex(PRUint32 aIndex,
                          PRInt32 *_type)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  ENSURE_INDEX_VALUE(aIndex, mResultColumnCount);

  if (!mExecuting)
    return NS_ERROR_UNEXPECTED;

  int t = ::sqlite3_column_type(mDBStatement, aIndex);
  switch (t) {
    case SQLITE_INTEGER:
      *_type = VALUE_TYPE_INTEGER;
      break;
    case SQLITE_FLOAT:
      *_type = VALUE_TYPE_FLOAT;
      break;
    case SQLITE_TEXT:
      *_type = VALUE_TYPE_TEXT;
      break;
    case SQLITE_BLOB:
      *_type = VALUE_TYPE_BLOB;
      break;
    case SQLITE_NULL:
      *_type = VALUE_TYPE_NULL;
      break;
    default:
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
Statement::GetInt32(PRUint32 aIndex,
                    PRInt32 *_value)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  ENSURE_INDEX_VALUE(aIndex, mResultColumnCount);

  if (!mExecuting)
    return NS_ERROR_UNEXPECTED;

  *_value = ::sqlite3_column_int(mDBStatement, aIndex);
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetInt64(PRUint32 aIndex,
                    PRInt64 *_value)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  ENSURE_INDEX_VALUE(aIndex, mResultColumnCount);

  if (!mExecuting)
    return NS_ERROR_UNEXPECTED;

  *_value = ::sqlite3_column_int64(mDBStatement, aIndex);

  return NS_OK;
}

NS_IMETHODIMP
Statement::GetDouble(PRUint32 aIndex,
                     double *_value)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  ENSURE_INDEX_VALUE(aIndex, mResultColumnCount);

  if (!mExecuting)
    return NS_ERROR_UNEXPECTED;

  *_value = ::sqlite3_column_double(mDBStatement, aIndex);

  return NS_OK;
}

NS_IMETHODIMP
Statement::GetUTF8String(PRUint32 aIndex,
                         nsACString &_value)
{
  
  PRInt32 type;
  nsresult rv = GetTypeOfIndex(aIndex, &type);
  NS_ENSURE_SUCCESS(rv, rv);
  if (type == VALUE_TYPE_NULL) {
    
    
    _value.Truncate(0);
    _value.SetIsVoid(PR_TRUE);
  }
  else {
    const char *value =
      reinterpret_cast<const char *>(::sqlite3_column_text(mDBStatement,
                                                           aIndex));
    _value.Assign(value, ::sqlite3_column_bytes(mDBStatement, aIndex));
  }
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetString(PRUint32 aIndex,
                     nsAString &_value)
{
  
  PRInt32 type;
  nsresult rv = GetTypeOfIndex(aIndex, &type);
  NS_ENSURE_SUCCESS(rv, rv);
  if (type == VALUE_TYPE_NULL) {
    
    
    _value.Truncate(0);
    _value.SetIsVoid(PR_TRUE);
  } else {
    const PRUnichar *value =
      static_cast<const PRUnichar *>(::sqlite3_column_text16(mDBStatement,
                                                             aIndex));
    _value.Assign(value, ::sqlite3_column_bytes16(mDBStatement, aIndex) / 2);
  }
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetBlob(PRUint32 aIndex,
                   PRUint32 *_size,
                   PRUint8 **_blob)
{
  if (!mDBStatement)
    return NS_ERROR_NOT_INITIALIZED;

  ENSURE_INDEX_VALUE(aIndex, mResultColumnCount);

  if (!mExecuting)
     return NS_ERROR_UNEXPECTED;

  int size = ::sqlite3_column_bytes(mDBStatement, aIndex);
  void *blob = nsnull;
  if (size) {
    blob = nsMemory::Clone(::sqlite3_column_blob(mDBStatement, aIndex), size);
    NS_ENSURE_TRUE(blob, NS_ERROR_OUT_OF_MEMORY);
  }

  *_blob = static_cast<PRUint8 *>(blob);
  *_size = size;
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetSharedUTF8String(PRUint32 aIndex,
                               PRUint32 *_length,
                               const char **_value)
{
  if (_length)
    *_length = ::sqlite3_column_bytes(mDBStatement, aIndex);

  *_value = reinterpret_cast<const char *>(::sqlite3_column_text(mDBStatement,
                                                                 aIndex));
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetSharedString(PRUint32 aIndex,
                           PRUint32 *_length,
                           const PRUnichar **_value)
{
  if (_length)
    *_length = ::sqlite3_column_bytes16(mDBStatement, aIndex);

  *_value = static_cast<const PRUnichar *>(::sqlite3_column_text16(mDBStatement,
                                                                   aIndex));
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetSharedBlob(PRUint32 aIndex,
                         PRUint32 *_size,
                         const PRUint8 **_blob)
{
  *_size = ::sqlite3_column_bytes(mDBStatement, aIndex);
  *_blob = static_cast<const PRUint8 *>(::sqlite3_column_blob(mDBStatement,
                                                              aIndex));
  return NS_OK;
}

NS_IMETHODIMP
Statement::GetIsNull(PRUint32 aIndex,
                     PRBool *_isNull)
{
  
  PRInt32 type;
  nsresult rv = GetTypeOfIndex(aIndex, &type);
  NS_ENSURE_SUCCESS(rv, rv);
  *_isNull = (type == VALUE_TYPE_NULL);
  return NS_OK;
}

} 
} 
