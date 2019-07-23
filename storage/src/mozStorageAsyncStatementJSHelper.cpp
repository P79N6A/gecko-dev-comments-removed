






































#include "nsIXPConnect.h"
#include "mozStorageAsyncStatement.h"
#include "mozStorageService.h"

#include "nsMemory.h"
#include "nsString.h"
#include "nsServiceManagerUtils.h"

#include "mozStorageAsyncStatementJSHelper.h"

#include "mozStorageAsyncStatementParams.h"

#include "jsapi.h"

namespace mozilla {
namespace storage {




nsresult
AsyncStatementJSHelper::getParams(AsyncStatement *aStatement,
                                  JSContext *aCtx,
                                  JSObject *aScopeObj,
                                  jsval *_params)
{
  nsresult rv;

#ifdef DEBUG
  PRInt32 state;
  (void)aStatement->GetState(&state);
  NS_ASSERTION(state == mozIStorageAsyncStatement::MOZ_STORAGE_STATEMENT_READY,
               "Invalid state to get the params object - all calls will fail!");
#endif

  if (!aStatement->mStatementParamsHolder) {
    nsCOMPtr<mozIStorageStatementParams> params =
      new AsyncStatementParams(aStatement);
    NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIXPConnect> xpc(Service::getXPConnect());
    rv = xpc->WrapNative(
      aCtx,
      ::JS_GetGlobalForObject(aCtx, aScopeObj),
      params,
      NS_GET_IID(mozIStorageStatementParams),
      getter_AddRefs(aStatement->mStatementParamsHolder)
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  JSObject *obj = nsnull;
  rv = aStatement->mStatementParamsHolder->GetJSObject(&obj);
  NS_ENSURE_SUCCESS(rv, rv);

  *_params = OBJECT_TO_JSVAL(obj);
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) AsyncStatementJSHelper::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) AsyncStatementJSHelper::Release() { return 1; }
NS_INTERFACE_MAP_BEGIN(AsyncStatementJSHelper)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




#define XPC_MAP_CLASSNAME AsyncStatementJSHelper
#define XPC_MAP_QUOTED_CLASSNAME "AsyncStatementJSHelper"
#define XPC_MAP_WANT_GETPROPERTY
#define XPC_MAP_FLAGS nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h"

NS_IMETHODIMP
AsyncStatementJSHelper::GetProperty(nsIXPConnectWrappedNative *aWrapper,
                                    JSContext *aCtx,
                                    JSObject *aScopeObj,
                                    jsval aId,
                                    jsval *_result,
                                    PRBool *_retval)
{
  if (!JSVAL_IS_STRING(aId))
    return NS_OK;

  
  mozIStorageAsyncStatement *iAsyncStmt =
    static_cast<mozIStorageAsyncStatement *>(aWrapper->Native());
  AsyncStatement *stmt = static_cast<AsyncStatement *>(iAsyncStmt);

#ifdef DEBUG
  {
    nsISupports *supp = aWrapper->Native();
    nsCOMPtr<mozIStorageAsyncStatement> isStatement(do_QueryInterface(supp));
    NS_ASSERTION(isStatement, "How is this not an async statement?!");
  }
#endif

  const char *propName = ::JS_GetStringBytes(JSVAL_TO_STRING(aId));
  if (::strcmp(propName, "params") == 0)
    return getParams(stmt, aCtx, aScopeObj, _result);

  return NS_OK;
}

} 
} 
