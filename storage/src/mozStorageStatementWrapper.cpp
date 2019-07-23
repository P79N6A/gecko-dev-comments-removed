







































#include "nsString.h"

#include "mozStorageStatementWrapper.h"
#include "mozStorageStatementParams.h"
#include "mozStorageStatementRow.h"

#include "sqlite3.h"

namespace mozilla {
namespace storage {




StatementWrapper::StatementWrapper()
: mStatement(nsnull)
{
}

StatementWrapper::~StatementWrapper()
{
  mStatement = nsnull;
}

NS_IMPL_ISUPPORTS2(
  StatementWrapper,
  mozIStorageStatementWrapper,
  nsIXPCScriptable
)




NS_IMETHODIMP
StatementWrapper::Initialize(mozIStorageStatement *aStatement)
{
  NS_ASSERTION(mStatement == nsnull, "StatementWrapper is already initialized");
  NS_ENSURE_ARG_POINTER(aStatement);

  mStatement = static_cast<Statement *>(aStatement);

  
  (void)mStatement->GetParameterCount(&mParamCount);
  (void)mStatement->GetColumnCount(&mResultColumnCount);

  for (unsigned int i = 0; i < mResultColumnCount; i++) {
    const void *name = ::sqlite3_column_name16(nativeStatement(), i);
    (void)mColumnNames.AppendElement(nsDependentString(static_cast<const PRUnichar*>(name)));
  }

  return NS_OK;
}

NS_IMETHODIMP
StatementWrapper::GetStatement(mozIStorageStatement **_statement)
{
  NS_IF_ADDREF(*_statement = mStatement);
  return NS_OK;
}

NS_IMETHODIMP
StatementWrapper::Reset()
{
  if (!mStatement)
    return NS_ERROR_FAILURE;

  return mStatement->Reset();
}

NS_IMETHODIMP
StatementWrapper::Step(PRBool *_hasMoreResults)
{
  if (!mStatement)
    return NS_ERROR_FAILURE;

  PRBool hasMore = PR_FALSE;
  nsresult rv = mStatement->ExecuteStep(&hasMore);
  if (NS_SUCCEEDED(rv) && !hasMore) {
    *_hasMoreResults = PR_FALSE;
    (void)mStatement->Reset();
    return NS_OK;
  }

  *_hasMoreResults = hasMore;
  return rv;
}

NS_IMETHODIMP
StatementWrapper::Execute()
{
  if (!mStatement)
    return NS_ERROR_FAILURE;

  return mStatement->Execute();
}

NS_IMETHODIMP
StatementWrapper::GetRow(mozIStorageStatementRow **_row)
{
  NS_ENSURE_ARG_POINTER(_row);

  if (!mStatement)
    return NS_ERROR_FAILURE;

  PRInt32 state;
  mStatement->GetState(&state);
  if (state != mozIStorageStatement::MOZ_STORAGE_STATEMENT_EXECUTING)
    return NS_ERROR_FAILURE;

  if (!mStatementRow) {
    mStatementRow = new StatementRow(mStatement);
    NS_ENSURE_TRUE(mStatementRow, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ADDREF(*_row = mStatementRow);
  return NS_OK;
}

NS_IMETHODIMP
StatementWrapper::GetParams(mozIStorageStatementParams **_params)
{
  NS_ENSURE_ARG_POINTER(_params);

  if (!mStatementParams) {
    mStatementParams = new StatementParams(mStatement);
    NS_ENSURE_TRUE(mStatementParams, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ADDREF(*_params = mStatementParams);
  return NS_OK;
}




#define XPC_MAP_CLASSNAME StatementWrapper
#define XPC_MAP_QUOTED_CLASSNAME "StatementWrapper"
#define XPC_MAP_WANT_CALL
#define XPC_MAP_FLAGS nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE | \
                      nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY
#include "xpc_map_end.h"

NS_IMETHODIMP
StatementWrapper::Call(nsIXPConnectWrappedNative *aWrapper,
                       JSContext *aCtx,
                       JSObject *aScopeObj,
                       PRUint32 aArgc,
                       jsval *aArgv,
                       jsval *_vp,
                       PRBool *_retval)
{
  if (!mStatement)
    return NS_ERROR_FAILURE;

  if (aArgc != mParamCount) {
    *_retval = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  
  (void)mStatement->Reset();

  
  for (int i = 0; i < (int)aArgc; i++) {
    if (!JSValStorageStatementBinder(aCtx, mStatement, i, aArgv[i])) {
      *_retval = PR_FALSE;
      return NS_ERROR_INVALID_ARG;
    }
  }

  
  if (mResultColumnCount == 0)
    (void)mStatement->Execute();

  *_vp = JSVAL_TRUE;
  *_retval = PR_TRUE;
  return NS_OK;
}

} 
} 
